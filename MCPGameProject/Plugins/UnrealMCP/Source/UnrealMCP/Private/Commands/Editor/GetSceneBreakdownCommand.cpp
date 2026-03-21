#include "Commands/Editor/GetSceneBreakdownCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "LevelEditorViewport.h"

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
	struct FLODInfo
	{
		int32 Tris = 0;
		float ScreenSize = 0.0f;
	};

	struct FMeshStats
	{
		FString MeshPath;
		FString MeshName;
		int32 LOD0Tris = 0;
		int32 LODCount = 0;
		TArray<FLODInfo> LODs;
		TArray<int32> InstancesPerLOD;  // How many instances render at each LOD
		int32 InstancesCulled = 0;      // Beyond cull distance
		int32 InstanceCount = 0;        // Total ISM/HISM instances
		int32 ComponentCount = 0;
		int32 ShadowCasters = 0;
		int32 WPODisableDistance = 0;
		float SphereRadius = 0.0f;
		bool bHasNanite = false;
		bool bIsISM = false;
	};

	// Get camera position for LOD calculation
	FVector CameraLocation = FVector::ZeroVector;
	bool bHasCamera = false;
	{
		FLevelEditorViewportClient* ViewportClient = nullptr;
		if (GEditor && GEditor->GetActiveViewport())
		{
			ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
		}
		if (ViewportClient)
		{
			CameraLocation = ViewportClient->GetViewLocation();
			bHasCamera = true;
		}
	}

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

				// Collect per-LOD info
				if (RD)
				{
					for (int32 LODIdx = 0; LODIdx < RD->LODResources.Num(); ++LODIdx)
					{
						FLODInfo LODInfo;
						LODInfo.Tris = Mesh->GetNumTriangles(LODIdx);
						if (Mesh->GetNumSourceModels() > LODIdx)
						{
							LODInfo.ScreenSize = Mesh->GetSourceModel(LODIdx).ScreenSize.Default;
						}
						Stats.LODs.Add(LODInfo);
					}
				}
			}

			// Get WPO disable distance from component
			Stats.WPODisableDistance = SMC->WorldPositionOffsetDisableDistance;
			Stats.SphereRadius = Mesh->GetBounds().SphereRadius;

			// ISM/HISM
			if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(SMC))
			{
				int32 Count = ISM->GetInstanceCount();
				Stats.InstanceCount += Count;
				Stats.bIsISM = true;
				Stats.ComponentCount++;
				if (SMC->CastShadow) Stats.ShadowCasters++;

				// Count instances per LOD based on distance from camera
				if (bHasCamera && Stats.LODs.Num() > 0)
				{
					// Ensure InstancesPerLOD array is sized
					while (Stats.InstancesPerLOD.Num() < Stats.LODs.Num())
					{
						Stats.InstancesPerLOD.Add(0);
					}

					float MeshRadius = Stats.SphereRadius;
					int32 CullDist = (int32)SMC->LDMaxDrawDistance;

					for (int32 InstIdx = 0; InstIdx < Count; ++InstIdx)
					{
						FTransform InstanceTransform;
						if (!ISM->GetInstanceTransform(InstIdx, InstanceTransform, true))
							continue;

						float Dist = FVector::Dist(CameraLocation, InstanceTransform.GetLocation());

						// Check cull distance
						if (CullDist > 0 && Dist > (float)CullDist)
						{
							Stats.InstancesCulled++;
							continue;
						}

						// Approximate screen size: radius / distance
						// UE uses a more complex formula but this gives reasonable LOD bucketing
						float ApproxScreenSize = (Dist > 1.0f) ? (MeshRadius / Dist) : 1.0f;

						// Find which LOD this maps to (screen sizes are in descending order)
						int32 LODIndex = 0;
						for (int32 L = 1; L < Stats.LODs.Num(); ++L)
						{
							if (ApproxScreenSize < Stats.LODs[L - 1].ScreenSize && Stats.LODs[L].ScreenSize > 0)
							{
								LODIndex = L;
							}
						}
						Stats.InstancesPerLOD[LODIndex]++;
					}
				}
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

	// Sort by instance count * LOD0 tris (worst case cost indicator)
	TArray<FMeshStats> Sorted;
	for (auto& Pair : MeshMap)
	{
		Sorted.Add(Pair.Value);
	}
	Sorted.Sort([](const FMeshStats& A, const FMeshStats& B)
	{
		return (int64)A.InstanceCount * A.LOD0Tris > (int64)B.InstanceCount * B.LOD0Tris;
	});

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
		Obj->SetNumberField(TEXT("lod_count"), S.LODCount);
		Obj->SetNumberField(TEXT("components"), S.ComponentCount);
		Obj->SetNumberField(TEXT("shadow_casters"), S.ShadowCasters);
		Obj->SetNumberField(TEXT("wpo_disable_distance"), S.WPODisableDistance);
		Obj->SetBoolField(TEXT("nanite"), S.bHasNanite);
		Obj->SetBoolField(TEXT("instanced"), S.bIsISM);

		// Per-LOD breakdown with instance counts
		TArray<TSharedPtr<FJsonValue>> LODArray;
		int64 EstimatedRenderedTris = 0;
		for (int32 L = 0; L < S.LODs.Num(); ++L)
		{
			TSharedPtr<FJsonObject> LODObj = MakeShared<FJsonObject>();
			LODObj->SetNumberField(TEXT("tris"), S.LODs[L].Tris);
			LODObj->SetNumberField(TEXT("screen_size"), S.LODs[L].ScreenSize);
			int32 InstAtLOD = (L < S.InstancesPerLOD.Num()) ? S.InstancesPerLOD[L] : 0;
			LODObj->SetNumberField(TEXT("instances"), InstAtLOD);
			LODObj->SetNumberField(TEXT("total_tris"), (int64)InstAtLOD * S.LODs[L].Tris);
			EstimatedRenderedTris += (int64)InstAtLOD * S.LODs[L].Tris;
			LODArray.Add(MakeShared<FJsonValueObject>(LODObj));
		}
		Obj->SetArrayField(TEXT("lods"), LODArray);
		Obj->SetNumberField(TEXT("culled_instances"), S.InstancesCulled);
		Obj->SetNumberField(TEXT("estimated_rendered_tris"), (double)EstimatedRenderedTris);

		MeshArray.Add(MakeShared<FJsonValueObject>(Obj));

		GrandTotalInstances += S.InstanceCount;
		GrandTotalShadowCasters += S.ShadowCasters;
	}

	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("meshes"), MeshArray);
	Result->SetNumberField(TEXT("unique_meshes"), Sorted.Num());
	Result->SetNumberField(TEXT("total_instances"), GrandTotalInstances);
	Result->SetNumberField(TEXT("total_components"), TotalComponents);
	Result->SetNumberField(TEXT("total_actors"), TotalActors);
	Result->SetNumberField(TEXT("total_shadow_casters"), GrandTotalShadowCasters);

	// Summary
	FString Summary = FString::Printf(
		TEXT("%d unique meshes, %d instances, %d shadow casters"),
		Sorted.Num(), GrandTotalInstances, GrandTotalShadowCasters);
	Result->SetStringField(TEXT("message"), Summary);

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Result.ToSharedRef(), W);
	return Out;
}

FString FGetSceneBreakdownCommand::GetCommandName() const { return TEXT("get_scene_breakdown"); }
bool FGetSceneBreakdownCommand::ValidateParams(const FString& Parameters) const { return true; }
