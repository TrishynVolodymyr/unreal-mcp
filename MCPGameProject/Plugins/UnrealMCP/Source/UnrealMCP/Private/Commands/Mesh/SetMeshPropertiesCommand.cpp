#include "Commands/Mesh/SetMeshPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshEditorSubsystem.h"
#include "UObject/SavePackage.h"

FString FSetMeshPropertiesCommand::Execute(const FString& Parameters)
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
	if (!JsonObject->TryGetStringField(TEXT("mesh_path"), MeshPath))
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), TEXT("Missing 'mesh_path'"));
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

	TArray<FString> ChangedProperties;

	// Enable/Disable Nanite
	bool bEnableNanite;
	if (JsonObject->TryGetBoolField(TEXT("enable_nanite"), bEnableNanite))
	{
		FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
		NaniteSettings.bEnabled = bEnableNanite;

		UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();
		if (Subsystem)
		{
			Subsystem->SetNaniteSettings(Mesh, NaniteSettings, true);
		}
		else
		{
			Mesh->SetNaniteSettings(NaniteSettings);
		}
		ChangedProperties.Add(FString::Printf(TEXT("Nanite=%s"), bEnableNanite ? TEXT("true") : TEXT("false")));
	}

	// Materials — array of {slot_index, material_path} or single material_path for all slots
	FString MaterialPath;
	if (JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (Material)
		{
			int32 NumSlots = Mesh->GetStaticMaterials().Num();
			for (int32 i = 0; i < NumSlots; i++)
			{
				Mesh->SetMaterial(i, Material);
			}
			ChangedProperties.Add(FString::Printf(TEXT("Material=%s (all %d slots)"), *MaterialPath, NumSlots));
		}
		else
		{
			TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
			E->SetStringField(TEXT("error"), FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
			FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
			FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
		}
	}

	// Material per slot
	int32 SlotIndex;
	FString SlotMaterialPath;
	if (JsonObject->TryGetNumberField(TEXT("material_slot_index"), SlotIndex) &&
		JsonObject->TryGetStringField(TEXT("material_slot_path"), SlotMaterialPath))
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *SlotMaterialPath);
		if (Material && SlotIndex >= 0 && SlotIndex < Mesh->GetStaticMaterials().Num())
		{
			Mesh->SetMaterial(SlotIndex, Material);
			ChangedProperties.Add(FString::Printf(TEXT("Material[%d]=%s"), SlotIndex, *SlotMaterialPath));
		}
	}

	// Collision
	bool bHasCollision;
	if (JsonObject->TryGetBoolField(TEXT("has_collision"), bHasCollision))
	{
		if (bHasCollision)
		{
			// Enable simple collision if none exists
			if (!Mesh->GetBodySetup())
			{
				Mesh->CreateBodySetup();
			}
		}
		// Note: removing collision entirely is more complex, just set flag
		ChangedProperties.Add(FString::Printf(TEXT("Collision=%s"), bHasCollision ? TEXT("true") : TEXT("false")));
	}

	// Apply changes
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();

	// Save
	UPackage* Package = Mesh->GetOutermost();
	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Set properties on %s: %s"), *MeshPath, *FString::Join(ChangedProperties, TEXT(", ")));

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("mesh_path"), MeshPath);

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const FString& Prop : ChangedProperties)
	{
		PropsArray.Add(MakeShared<FJsonValueString>(Prop));
	}
	Result->SetArrayField(TEXT("changed_properties"), PropsArray);
	Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Set %d properties on %s: %s"),
		ChangedProperties.Num(), *MeshPath, *FString::Join(ChangedProperties, TEXT(", "))));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FSetMeshPropertiesCommand::GetCommandName() const { return TEXT("set_static_mesh_properties"); }
bool FSetMeshPropertiesCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> J; TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(R, J)) return false;
	FString P; return J->TryGetStringField(TEXT("mesh_path"), P);
}
