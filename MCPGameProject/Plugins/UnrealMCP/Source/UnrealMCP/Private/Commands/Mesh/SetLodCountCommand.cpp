#include "Commands/Mesh/SetLodCountCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "UObject/SavePackage.h"

FString FSetLodCountCommand::Execute(const FString& Parameters)
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
	int32 LodCount = 1;
	JsonObject->TryGetStringField(TEXT("mesh_path"), MeshPath);
	JsonObject->TryGetNumberField(TEXT("lod_count"), LodCount);

	if (MeshPath.IsEmpty() || LodCount < 1 || LodCount > 8)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("Invalid mesh_path or lod_count (1-8)"));
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

	int32 OldCount = Mesh->GetNumSourceModels();
	Mesh->SetNumSourceModels(LodCount);
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();

	// Save
	UPackage* Package = Mesh->GetOutermost();
	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Set LOD count on %s: %d → %d"), *MeshPath, OldCount, LodCount);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("old_lod_count"), OldCount);
	Result->SetNumberField(TEXT("new_lod_count"), LodCount);
	Result->SetStringField(TEXT("message"), FString::Printf(TEXT("LOD count set to %d (was %d)"), LodCount, OldCount));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FSetLodCountCommand::GetCommandName() const { return TEXT("set_lod_count"); }
bool FSetLodCountCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> J; TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(R, J)) return false;
	FString P; return J->TryGetStringField(TEXT("mesh_path"), P);
}
