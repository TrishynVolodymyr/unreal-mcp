#include "Commands/Mesh/GetStaticMeshMetadataCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshEditorSubsystem.h"

FString FGetStaticMeshMetadataCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), TEXT("Invalid JSON"));
		FString Out; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), W); return Out;
	}

	FString MeshPath;
	if (!JsonObject->TryGetStringField(TEXT("mesh_path"), MeshPath))
	{
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), TEXT("Missing 'mesh_path'"));
		FString Out; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), W); return Out;
	}

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
	if (!Mesh)
	{
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), FString::Printf(TEXT("StaticMesh not found: %s"), *MeshPath));
		FString Out; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), W); return Out;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("mesh_path"), MeshPath);

	// LOD info
	int32 NumLODs = Mesh->GetNumLODs();
	Result->SetNumberField(TEXT("lod_count"), NumLODs);

	TArray<TSharedPtr<FJsonValue>> LodArray;
	for (int32 i = 0; i < NumLODs; ++i)
	{
		TSharedPtr<FJsonObject> LodObj = MakeShared<FJsonObject>();
		LodObj->SetNumberField(TEXT("index"), i);
		LodObj->SetNumberField(TEXT("vertices"), Mesh->GetNumVertices(i));
		LodObj->SetNumberField(TEXT("triangles"), Mesh->GetNumTriangles(i));

		// Screen size (fixed array of MAX_STATIC_MESH_LODS=8)
		if (Mesh->GetRenderData() && i < MAX_STATIC_MESH_LODS)
		{
			LodObj->SetNumberField(TEXT("screen_size"), Mesh->GetRenderData()->ScreenSize[i].Default);
		}

		LodArray.Add(MakeShared<FJsonValueObject>(LodObj));
	}
	Result->SetArrayField(TEXT("lods"), LodArray);

	// Source models count
	Result->SetNumberField(TEXT("source_model_count"), Mesh->GetNumSourceModels());

	// Bounds
	FBoxSphereBounds Bounds = Mesh->GetBounds();
	TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();
	BoundsObj->SetNumberField(TEXT("origin_x"), Bounds.Origin.X);
	BoundsObj->SetNumberField(TEXT("origin_y"), Bounds.Origin.Y);
	BoundsObj->SetNumberField(TEXT("origin_z"), Bounds.Origin.Z);
	BoundsObj->SetNumberField(TEXT("extent_x"), Bounds.BoxExtent.X);
	BoundsObj->SetNumberField(TEXT("extent_y"), Bounds.BoxExtent.Y);
	BoundsObj->SetNumberField(TEXT("extent_z"), Bounds.BoxExtent.Z);
	BoundsObj->SetNumberField(TEXT("sphere_radius"), Bounds.SphereRadius);
	Result->SetObjectField(TEXT("bounds"), BoundsObj);

	// Material slots
	TArray<TSharedPtr<FJsonValue>> MatArray;
	for (const FStaticMaterial& Mat : Mesh->GetStaticMaterials())
	{
		TSharedPtr<FJsonObject> MatObj = MakeShared<FJsonObject>();
		MatObj->SetStringField(TEXT("slot_name"), Mat.MaterialSlotName.ToString());
		MatObj->SetStringField(TEXT("material"), Mat.MaterialInterface ? Mat.MaterialInterface->GetPathName() : TEXT("None"));
		MatArray.Add(MakeShared<FJsonValueObject>(MatObj));
	}
	Result->SetArrayField(TEXT("material_slots"), MatArray);

	Result->SetStringField(TEXT("message"), FString::Printf(
		TEXT("StaticMesh %s: %d LODs, LOD0=%d verts/%d tris"),
		*MeshPath, NumLODs, Mesh->GetNumVertices(0), Mesh->GetNumTriangles(0)));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FGetStaticMeshMetadataCommand::GetCommandName() const { return TEXT("get_static_mesh_metadata"); }
bool FGetStaticMeshMetadataCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> J; TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(R, J)) return false;
	FString P; return J->TryGetStringField(TEXT("mesh_path"), P);
}
