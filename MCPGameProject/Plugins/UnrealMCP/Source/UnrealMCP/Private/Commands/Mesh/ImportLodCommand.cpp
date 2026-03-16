#include "Commands/Mesh/ImportLodCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshEditorSubsystem.h"
#include "UObject/SavePackage.h"

FString FImportLodCommand::Execute(const FString& Parameters)
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

	FString MeshPath, SourceFilePath;
	int32 LodIndex = 0;
	JsonObject->TryGetStringField(TEXT("mesh_path"), MeshPath);
	JsonObject->TryGetNumberField(TEXT("lod_index"), LodIndex);
	JsonObject->TryGetStringField(TEXT("source_file_path"), SourceFilePath);

	if (MeshPath.IsEmpty() || SourceFilePath.IsEmpty())
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("Missing mesh_path or source_file_path"));
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

	// Ensure LOD slot exists
	if (LodIndex >= Mesh->GetNumSourceModels())
	{
		Mesh->SetNumSourceModels(LodIndex + 1);
	}

	// Use UStaticMeshEditorSubsystem to import LOD
	UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();
	if (!Subsystem)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("StaticMeshEditorSubsystem not available"));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

	int32 ResultLOD = Subsystem->ImportLOD(Mesh, LodIndex, SourceFilePath);

	if (ResultLOD < 0)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to import LOD %d from %s"), LodIndex, *SourceFilePath));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

	// Save
	Mesh->MarkPackageDirty();
	UPackage* Package = Mesh->GetOutermost();
	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs);

	// Get stats for imported LOD
	int32 Verts = Mesh->GetNumVertices(LodIndex);
	int32 Tris = Mesh->GetNumTriangles(LodIndex);

	UE_LOG(LogTemp, Log, TEXT("Imported LOD%d for %s from %s (%d verts, %d tris)"),
		LodIndex, *MeshPath, *SourceFilePath, Verts, Tris);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("lod_index"), ResultLOD);
	Result->SetNumberField(TEXT("vertices"), Verts);
	Result->SetNumberField(TEXT("triangles"), Tris);
	Result->SetNumberField(TEXT("total_lods"), Mesh->GetNumLODs());
	Result->SetStringField(TEXT("message"), FString::Printf(
		TEXT("Imported LOD%d: %d verts, %d tris. Total LODs: %d"), ResultLOD, Verts, Tris, Mesh->GetNumLODs()));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FImportLodCommand::GetCommandName() const { return TEXT("import_lod"); }
bool FImportLodCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> J; TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(R, J)) return false;
	FString P1, P2; return J->TryGetStringField(TEXT("mesh_path"), P1) && J->TryGetStringField(TEXT("source_file_path"), P2);
}
