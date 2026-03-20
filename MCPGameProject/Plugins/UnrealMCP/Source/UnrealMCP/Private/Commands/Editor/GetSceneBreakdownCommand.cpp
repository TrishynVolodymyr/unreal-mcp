#include "Commands/Editor/GetSceneBreakdownCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"

FString FGetSceneBreakdownCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"), TEXT("No editor world"));
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	// Parse optional filter
	FString MeshFilter;
	int32 MaxResults = 50;
	{
		TSharedPtr<FJsonObject> Params;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
		if (FJsonSerializer::Deserialize(Reader, Params) && Params.IsValid())
		{
			Params->TryGetStringField(TEXT("mesh_filter"), MeshFilter);
			Params->TryGetNumberField(TEXT("max_results"), MaxResults);
		}
	}

	// Per-mesh aggregate
	struct FMeshStats
	{
		FString MeshPath;
		FString MeshName;
		int32 LOD0Tris = 0;
		int32 LODCount = 0;
		int32 InstanceCount = 0;      // ISM/HISM instances
		int32 ComponentCount = 0;     // Number of SMC/ISM components using this mesh
		int32 ShadowCasters = 0;      // Components with CastShadow=true
		int64 TotalTris = 0;          // instances * LOD0Tris
		bool bHasNanite = false;
		bool bIsISM = false;
	};

	TMap<UStaticMesh*, FMeshStats> MeshMap;
	int32 TotalComponents = 0;
	int32 TotalActors = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		bool bActorCounted = false;

		Actor->ForEachComponent<UStaticMeshComponent>(false, [&](UStaticMeshComponent* SMC)
		{
			if (!SMC || !SMC->IsRegistered()) return;

			UStaticMesh* Mesh = SMC->GetStaticMesh();
			if (!Mesh) return;

			if (!bActorCounted) { ++TotalActors; bActorCounted = true; }
			++TotalComponents;

			FMeshStats& Stats = MeshMap.FindOrAdd(Mesh);
			if (Stats.MeshPath.IsEmpty())
			{
				Stats.MeshPath = Mesh->GetPathName();
				Stats.MeshName = Mesh->GetName();
				Stats.LOD0Tris = Mesh->GetNumTriangles(0);
				Stats.LODCount = Mesh->GetNumLODs();

				FStaticMeshRenderData* RD = Mesh->GetRenderData();
				Stats.bHasNanite = RD && RD->HasValidNaniteData();
			}

			// ISM/HISM
			if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(SMC))
			{
				int32 Count = ISM->GetInstanceCount();
				Stats.InstanceCount += Count;
				Stats.bIsISM = true;
				Stats.ComponentCount++;
				if (SMC->CastShadow) Stats.ShadowCasters++;
			}
			else
			{
				// Regular static mesh — counts as 1 instance
				Stats.InstanceCount += 1;
				Stats.ComponentCount++;
				if (SMC->CastShadow) Stats.ShadowCasters++;
			}
		});
	}

	// Calculate total tris and sort
	TArray<FMeshStats> Sorted;
	for (auto& Pair : MeshMap)
	{
		Pair.Value.TotalTris = (int64)Pair.Value.InstanceCount * Pair.Value.LOD0Tris;
		Sorted.Add(Pair.Value);
	}
	Sorted.Sort([](const FMeshStats& A, const FMeshStats& B) { return A.TotalTris > B.TotalTris; });

	// Apply mesh_filter if provided
	if (!MeshFilter.IsEmpty())
	{
		Sorted = Sorted.FilterByPredicate([&](const FMeshStats& S)
		{
			return S.MeshName.Contains(MeshFilter, ESearchCase::IgnoreCase)
				|| S.MeshPath.Contains(MeshFilter, ESearchCase::IgnoreCase);
		});
	}

	// Build JSON
	int64 GrandTotalTris = 0;
	int32 GrandTotalInstances = 0;
	int32 GrandTotalShadowCasters = 0;

	TArray<TSharedPtr<FJsonValue>> MeshArray;
	for (int32 i = 0; i < FMath::Min(Sorted.Num(), MaxResults); ++i)
	{
		const FMeshStats& S = Sorted[i];
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("mesh"), S.MeshName);
		Obj->SetStringField(TEXT("path"), S.MeshPath);
		Obj->SetNumberField(TEXT("instances"), S.InstanceCount);
		Obj->SetNumberField(TEXT("lod0_tris"), S.LOD0Tris);
		Obj->SetNumberField(TEXT("total_tris"), (double)S.TotalTris);
		Obj->SetNumberField(TEXT("lod_count"), S.LODCount);
		Obj->SetNumberField(TEXT("components"), S.ComponentCount);
		Obj->SetNumberField(TEXT("shadow_casters"), S.ShadowCasters);
		Obj->SetBoolField(TEXT("nanite"), S.bHasNanite);
		Obj->SetBoolField(TEXT("instanced"), S.bIsISM);
		MeshArray.Add(MakeShared<FJsonValueObject>(Obj));

		GrandTotalTris += S.TotalTris;
		GrandTotalInstances += S.InstanceCount;
		GrandTotalShadowCasters += S.ShadowCasters;
	}

	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("meshes"), MeshArray);
	Result->SetNumberField(TEXT("unique_meshes"), Sorted.Num());
	Result->SetNumberField(TEXT("total_instances"), GrandTotalInstances);
	Result->SetNumberField(TEXT("total_tris_lod0"), (double)GrandTotalTris);
	Result->SetNumberField(TEXT("total_components"), TotalComponents);
	Result->SetNumberField(TEXT("total_actors"), TotalActors);
	Result->SetNumberField(TEXT("total_shadow_casters"), GrandTotalShadowCasters);

	// Summary
	FString Summary = FString::Printf(
		TEXT("%d unique meshes, %d instances, %.1fM tris (LOD0), %d shadow casters"),
		Sorted.Num(), GrandTotalInstances, GrandTotalTris / 1000000.0, GrandTotalShadowCasters);
	Result->SetStringField(TEXT("message"), Summary);

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Result.ToSharedRef(), W);
	return Out;
}

FString FGetSceneBreakdownCommand::GetCommandName() const { return TEXT("get_scene_breakdown"); }
bool FGetSceneBreakdownCommand::ValidateParams(const FString& Parameters) const { return true; }
