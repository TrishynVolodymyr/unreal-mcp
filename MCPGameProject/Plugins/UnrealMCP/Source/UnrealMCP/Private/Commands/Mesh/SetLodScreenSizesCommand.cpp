#include "Commands/Mesh/SetLodScreenSizesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshEditorSubsystem.h"
#include "UObject/SavePackage.h"

FString FSetLodScreenSizesCommand::Execute(const FString& Parameters)
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
	JsonObject->TryGetStringField(TEXT("mesh_path"), MeshPath);

	const TArray<TSharedPtr<FJsonValue>>* ScreenSizesArray;
	if (MeshPath.IsEmpty() || !JsonObject->TryGetArrayField(TEXT("screen_sizes"), ScreenSizesArray))
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("Missing mesh_path or screen_sizes array"));
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

	// Parse screen sizes
	TArray<float> ScreenSizes;
	for (const auto& Val : *ScreenSizesArray)
	{
		ScreenSizes.Add((float)Val->AsNumber());
	}

	// Disable auto-compute so manual screen sizes stick
	Mesh->SetAutoComputeLODScreenSize(false);

	// Set on source models (persists to disk)
	for (int32 i = 0; i < ScreenSizes.Num() && i < Mesh->GetNumSourceModels(); ++i)
	{
		Mesh->GetSourceModel(i).ScreenSize.Default = ScreenSizes[i];
	}

	// Set on render data for immediate visual effect
	if (Mesh->GetRenderData())
	{
		for (int32 i = 0; i < ScreenSizes.Num() && i < MAX_STATIC_MESH_LODS; ++i)
		{
			Mesh->GetRenderData()->ScreenSize[i].Default = ScreenSizes[i];
		}
	}

	// Do NOT call PostEditChange() — it triggers a full mesh rebuild that
	// destroys and recreates RenderData, corrupting the mesh scale.
	// This follows Epic's own UStaticMeshEditorSubsystem::SetLodScreenSizes pattern.
	Mesh->Modify();
	Mesh->MarkPackageDirty();

	// Save
	UPackage* Package = Mesh->GetOutermost();
	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Set LOD screen sizes on %s: %d values"), *MeshPath, ScreenSizes.Num());

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("lod_count"), ScreenSizes.Num());
	Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Screen sizes set for %d LODs"), ScreenSizes.Num()));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FSetLodScreenSizesCommand::GetCommandName() const { return TEXT("set_lod_screen_sizes"); }
bool FSetLodScreenSizesCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> J; TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(R, J)) return false;
	FString P; const TArray<TSharedPtr<FJsonValue>>* A;
	return J->TryGetStringField(TEXT("mesh_path"), P) && J->TryGetArrayField(TEXT("screen_sizes"), A);
}
