#include "Commands/Mesh/SetMeshPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshEditorSubsystem.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/SavePackage.h"

#if WITH_DEV_AUTOMATION_TESTS
TOptional<bool> FSetMeshPropertiesCommand::SaveResultOverrideForTest;
TOptional<bool> FSetMeshPropertiesCommand::CollisionResultOverrideForTest;
int32 FSetMeshPropertiesCommand::PostEditChangeCallCountForTest = 0;
int32 FSetMeshPropertiesCommand::SaveAttemptCountForTest = 0;
#endif

bool FSetMeshPropertiesCommand::ApplyCollisionSetting(
	UStaticMesh& Mesh,
	bool bHasCollision,
	bool& bOutMeshChanged)
{
	bOutMeshChanged = false;
#if WITH_DEV_AUTOMATION_TESTS
	if (CollisionResultOverrideForTest.IsSet())
	{
		return CollisionResultOverrideForTest.GetValue();
	}
#endif
	UStaticMeshEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>();
	if (!Subsystem)
	{
		return false;
	}

	const UBodySetup* BodySetup = Mesh.GetBodySetup();
	const int32 ExistingCollisionCount = BodySetup ? BodySetup->AggGeom.GetElementCount() : 0;
	if (!bHasCollision)
	{
		if (ExistingCollisionCount <= 0)
		{
			return true;
		}
		const bool bRemoved = Subsystem->RemoveCollisionsWithNotification(&Mesh, false);
		bOutMeshChanged = bRemoved;
		return bRemoved;
	}

	if (ExistingCollisionCount > 0)
	{
		return true;
	}
	bool bCreatedBodySetup = false;
	if (!Mesh.GetBodySetup())
	{
		Mesh.CreateBodySetup();
		if (!Mesh.GetBodySetup())
		{
			return false;
		}
		bCreatedBodySetup = true;
	}
	const int32 NewCollisionIndex = Subsystem->AddSimpleCollisionsWithNotification(
		&Mesh,
		EScriptCollisionShapeType::Box,
		false);
	if (NewCollisionIndex < 0 && bCreatedBodySetup)
	{
		Mesh.SetBodySetup(nullptr);
	}
	bOutMeshChanged = NewCollisionIndex >= 0;
	return bOutMeshChanged;
}

#if WITH_DEV_AUTOMATION_TESTS
bool FSetMeshPropertiesCommand::ApplyCollisionSettingForTest(UStaticMesh& Mesh, bool bHasCollision)
{
	bool bMeshChanged = false;
	return ApplyCollisionSetting(Mesh, bHasCollision, bMeshChanged);
}

void FSetMeshPropertiesCommand::SetSaveResultOverrideForTest(TOptional<bool> ResultOverride)
{
	SaveResultOverrideForTest = ResultOverride;
}

void FSetMeshPropertiesCommand::SetCollisionResultOverrideForTest(TOptional<bool> ResultOverride)
{
	CollisionResultOverrideForTest = ResultOverride;
}

void FSetMeshPropertiesCommand::ResetExecutionCountersForTest()
{
	PostEditChangeCallCountForTest = 0;
	SaveAttemptCountForTest = 0;
}

int32 FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest()
{
	return PostEditChangeCallCountForTest;
}

int32 FSetMeshPropertiesCommand::GetSaveAttemptCountForTest()
{
	return SaveAttemptCountForTest;
}
#endif

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
	bool bNeedsPostEditChange = false;

	// Resolve and validate every referenced asset before mutating the mesh. This keeps a
	// late validation error from leaving an earlier collision/Nanite edit in memory.
	FString MaterialPath;
	UMaterialInterface* AllSlotsMaterial = nullptr;
	if (JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		AllSlotsMaterial = MaterialPath.IsEmpty()
			? nullptr
			: LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (!AllSlotsMaterial)
		{
			TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
			E->SetStringField(TEXT("error"), FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
			FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
			FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
		}
	}

	int32 SlotIndex = INDEX_NONE;
	FString SlotMaterialPath;
	UMaterialInterface* SlotMaterial = nullptr;
	const bool bHasSlotIndex = JsonObject->TryGetNumberField(TEXT("material_slot_index"), SlotIndex);
	const bool bHasSlotPath = JsonObject->TryGetStringField(TEXT("material_slot_path"), SlotMaterialPath);
	if (bHasSlotIndex != bHasSlotPath)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(
			TEXT("error"),
			TEXT("'material_slot_index' and 'material_slot_path' must be provided together"));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}
	if (bHasSlotIndex && bHasSlotPath)
	{
		SlotMaterial = SlotMaterialPath.IsEmpty()
			? nullptr
			: LoadObject<UMaterialInterface>(nullptr, *SlotMaterialPath);
		if (!SlotMaterial || SlotIndex < 0 || SlotIndex >= Mesh->GetStaticMaterials().Num())
		{
			TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
			E->SetStringField(TEXT("error"), FString::Printf(
				TEXT("Invalid material slot assignment: index=%d path=%s"),
				SlotIndex,
				*SlotMaterialPath));
			FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
			FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
		}
	}

	bool bEnableNanite = false;
	const bool bHasNaniteSetting = JsonObject->TryGetBoolField(TEXT("enable_nanite"), bEnableNanite);
	bool bHasCollision = false;
	const bool bHasCollisionSetting = JsonObject->TryGetBoolField(TEXT("has_collision"), bHasCollision);
	const UBodySetup* InitialBodySetup = Mesh->GetBodySetup();
	const int32 InitialCollisionCount = InitialBodySetup ? InitialBodySetup->AggGeom.GetElementCount() : 0;
	const bool bCollisionWillChange = bHasCollisionSetting
		&& ((bHasCollision && InitialCollisionCount == 0) || (!bHasCollision && InitialCollisionCount > 0));
	const bool bNaniteWillChange = bHasNaniteSetting && Mesh->GetNaniteSettings().bEnabled != bEnableNanite;

	// UE's StaticMeshEditorSubsystem closes an open asset editor even when bApplyChanges=false.
	// Own that lifecycle at command level so all requested mutations share one final rebuild and
	// the editor is restored afterwards.
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	const bool bWasMeshEditorOpen = AssetEditorSubsystem
		&& (bCollisionWillChange || bNaniteWillChange)
		&& AssetEditorSubsystem->FindEditorForAsset(Mesh, false) != nullptr;
	if (bWasMeshEditorOpen)
	{
		AssetEditorSubsystem->CloseAllEditorsForAsset(Mesh);
	}

	// Collision can still fail in the editor subsystem. Apply it before the infallible
	// property assignments so a failure cannot leave material/Nanite edits behind.
	if (bHasCollisionSetting)
	{
		bool bCollisionChanged = false;
		if (!ApplyCollisionSetting(*Mesh, bHasCollision, bCollisionChanged))
		{
			if (bWasMeshEditorOpen)
			{
				AssetEditorSubsystem->OpenEditorForAsset(Mesh);
			}
			TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
			E->SetStringField(TEXT("error"), FString::Printf(
				TEXT("Failed to %s simple collision on mesh: %s"),
				bHasCollision ? TEXT("generate") : TEXT("remove"),
				*MeshPath));
			FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
			FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
		}
		bNeedsPostEditChange |= bCollisionChanged;
		ChangedProperties.Add(FString::Printf(TEXT("Collision=%s"), bHasCollision ? TEXT("true") : TEXT("false")));
	}

	// Enable/Disable Nanite
	if (bHasNaniteSetting)
	{
		if (bNaniteWillChange)
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = bEnableNanite;
			Mesh->Modify();
			Mesh->SetNaniteSettings(NaniteSettings);
			bNeedsPostEditChange = true;
		}
		ChangedProperties.Add(FString::Printf(TEXT("Nanite=%s"), bEnableNanite ? TEXT("true") : TEXT("false")));
	}

	// Materials — array of {slot_index, material_path} or single material_path for all slots
	if (AllSlotsMaterial)
	{
		int32 NumSlots = Mesh->GetStaticMaterials().Num();
		for (int32 i = 0; i < NumSlots; i++)
		{
			Mesh->SetMaterial(i, AllSlotsMaterial);
		}
		bNeedsPostEditChange = true;
		ChangedProperties.Add(FString::Printf(TEXT("Material=%s (all %d slots)"), *MaterialPath, NumSlots));
	}

	// Material per slot
	if (SlotMaterial)
	{
		Mesh->SetMaterial(SlotIndex, SlotMaterial);
		bNeedsPostEditChange = true;
		ChangedProperties.Add(FString::Printf(TEXT("Material[%d]=%s"), SlotIndex, *SlotMaterialPath));
	}

	// Apply changes
	if (bNeedsPostEditChange)
	{
#if WITH_DEV_AUTOMATION_TESTS
		++PostEditChangeCallCountForTest;
#endif
		Mesh->PostEditChange();
	}
	if (bWasMeshEditorOpen)
	{
		AssetEditorSubsystem->OpenEditorForAsset(Mesh);
	}
	Mesh->MarkPackageDirty();

	// Save
	UPackage* Package = Mesh->GetOutermost();
	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = false;
#if WITH_DEV_AUTOMATION_TESTS
	++SaveAttemptCountForTest;
#endif
#if WITH_DEV_AUTOMATION_TESTS
	if (SaveResultOverrideForTest.IsSet())
	{
		bSaved = SaveResultOverrideForTest.GetValue();
	}
	else
#endif
	{
		bSaved = UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs);
	}
	if (!bSaved)
	{
		TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>(); E->SetBoolField(TEXT("success"), false);
		E->SetStringField(TEXT("error"), FString::Printf(
			TEXT("Changed mesh in memory but failed to save package: %s"),
			*Filename));
		FString O; TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&O);
		FJsonSerializer::Serialize(E.ToSharedRef(), W); return O;
	}

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
