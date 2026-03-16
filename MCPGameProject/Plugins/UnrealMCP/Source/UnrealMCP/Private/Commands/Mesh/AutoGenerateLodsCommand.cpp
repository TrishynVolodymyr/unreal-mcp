#include "Commands/Mesh/AutoGenerateLodsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshEditorSubsystem.h"
#include "UObject/SavePackage.h"

FString FAutoGenerateLodsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("Invalid JSON"));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

	FString MeshPath;
	int32 TargetLodCount = 3;
	JsonObject->TryGetStringField(TEXT("mesh_path"), MeshPath);
	JsonObject->TryGetNumberField(TEXT("target_lod_count"), TargetLodCount);

	if (MeshPath.IsEmpty())
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("Missing mesh_path"));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
	if (!Mesh)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), FString::Printf(TEXT("Mesh not found: %s"), *MeshPath));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

	// Parse reduction percentages or generate defaults
	TArray<float> ReductionPercentages;
	const TArray<TSharedPtr<FJsonValue>>* RedArray;
	if (JsonObject->TryGetArrayField(TEXT("reduction_percentages"), RedArray))
	{
		for (const auto& Val : *RedArray)
		{
			ReductionPercentages.Add((float)Val->AsNumber());
		}
	}
	else
	{
		// Default: evenly distributed
		for (int32 i = 0; i < TargetLodCount; ++i)
		{
			float Pct = 1.0f - ((float)i / (float)TargetLodCount);
			ReductionPercentages.Add(FMath::Max(0.05f, Pct));
		}
	}

	// Build reduction options
	UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();
	if (!Subsystem)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("StaticMeshEditorSubsystem not available"));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

	// Use SetLods with reduction options
	FStaticMeshReductionOptions Options;
	Options.bAutoComputeLODScreenSize = true;

	for (int32 i = 0; i < TargetLodCount; ++i)
	{
		FStaticMeshReductionSettings Settings;
		Settings.PercentTriangles = (i < ReductionPercentages.Num()) ? ReductionPercentages[i] : 0.5f;
		Settings.ScreenSize = 1.0f - ((float)i / (float)TargetLodCount);
		Options.ReductionSettings.Add(Settings);
	}

	Subsystem->SetLods(Mesh, Options);

	// Save
	Mesh->MarkPackageDirty();
	UPackage* Package = Mesh->GetOutermost();
	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs);

	// Build result with per-LOD stats
	TArray<TSharedPtr<FJsonValue>> LodArray;
	for (int32 i = 0; i < Mesh->GetNumLODs(); ++i)
	{
		TSharedPtr<FJsonObject> LodObj = MakeShared<FJsonObject>();
		LodObj->SetNumberField(TEXT("index"), i);
		LodObj->SetNumberField(TEXT("vertices"), Mesh->GetNumVertices(i));
		LodObj->SetNumberField(TEXT("triangles"), Mesh->GetNumTriangles(i));
		LodArray.Add(MakeShared<FJsonValueObject>(LodObj));
	}

	UE_LOG(LogTemp, Log, TEXT("Auto-generated %d LODs for %s"), Mesh->GetNumLODs(), *MeshPath);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("lod_count"), Mesh->GetNumLODs());
	Result->SetArrayField(TEXT("lods"), LodArray);
	Result->SetStringField(TEXT("message"), FString::Printf(
		TEXT("Generated %d LODs for %s"), Mesh->GetNumLODs(), *MeshPath));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FAutoGenerateLodsCommand::GetCommandName() const { return TEXT("auto_generate_lods"); }
bool FAutoGenerateLodsCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> J; TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(R, J)) return false;
	FString P; return J->TryGetStringField(TEXT("mesh_path"), P);
}
