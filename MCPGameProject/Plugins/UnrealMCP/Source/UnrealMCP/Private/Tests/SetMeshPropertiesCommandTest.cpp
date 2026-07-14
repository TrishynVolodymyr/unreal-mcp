#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Mesh/SetMeshPropertiesCommand.h"

#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "PackageTools.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "PhysicsEngine/ConvexElem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSetMeshPropertiesCollisionTest,
	"UnrealMCP.Mesh.SetStaticMeshProperties.Collision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	void AddValidConvexFixture(UStaticMesh& Mesh)
	{
		FKBoxElem Box;
		Box.X = 20.0;
		Box.Y = 20.0;
		Box.Z = 20.0;
		FKConvexElem Convex;
		Convex.ConvexFromBoxElem(Box);
		Mesh.GetBodySetup()->AggGeom.ConvexElems.Add(MoveTemp(Convex));
	}

	void UnloadPackageIfLoaded(FAutomationTestBase& Test, const FString& PackageName)
	{
		if (UPackage* ExistingPackage = FindPackage(nullptr, *PackageName))
		{
			ExistingPackage->SetDirtyFlag(false);
			TArray<UPackage*> PackagesToUnload{ExistingPackage};
			FText UnloadError;
			Test.TestTrue(
				TEXT("Stale automation package unloads before test"),
				UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
		}
	}
}

bool FSetMeshPropertiesCollisionTest::RunTest(const FString& Parameters)
{
	UStaticMesh* EngineCube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	TestNotNull(TEXT("Engine cube fixture loads"), EngineCube);
	if (!EngineCube)
	{
		return false;
	}

	UStaticMesh* Mesh = DuplicateObject<UStaticMesh>(EngineCube, GetTransientPackage());
	Mesh->GetBodySetup()->RemoveSimpleCollision();
	AddValidConvexFixture(*Mesh);
	TestEqual(TEXT("Fixture begins with one convex collision"), Mesh->GetBodySetup()->AggGeom.ConvexElems.Num(), 1);

	TestTrue(
		TEXT("has_collision=false applies successfully"),
		FSetMeshPropertiesCommand::ApplyCollisionSettingForTest(*Mesh, false));
	TestEqual(
		TEXT("has_collision=false removes the convex primitive omitted by GetSimpleCollisionCount"),
		Mesh->GetBodySetup()->AggGeom.GetElementCount(),
		0);
	TestTrue(
		TEXT("has_collision=false is idempotent"),
		FSetMeshPropertiesCommand::ApplyCollisionSettingForTest(*Mesh, false));

	UStaticMesh* CollisionlessCube = DuplicateObject<UStaticMesh>(EngineCube, GetTransientPackage());
	CollisionlessCube->GetBodySetup()->RemoveSimpleCollision();
	TestTrue(
		TEXT("has_collision=true applies successfully"),
		FSetMeshPropertiesCommand::ApplyCollisionSettingForTest(*CollisionlessCube, true));
	TestTrue(
		TEXT("has_collision=true creates a real simple primitive"),
		CollisionlessCube->GetBodySetup()->AggGeom.GetElementCount() > 0);
	const int32 GeneratedCollisionCount = CollisionlessCube->GetBodySetup()->AggGeom.GetElementCount();
	TestTrue(
		TEXT("has_collision=true is idempotent for an already-collidable mesh"),
		FSetMeshPropertiesCommand::ApplyCollisionSettingForTest(*CollisionlessCube, true));
	TestEqual(
		TEXT("Idempotent has_collision=true does not duplicate simple primitives"),
		CollisionlessCube->GetBodySetup()->AggGeom.GetElementCount(),
		GeneratedCollisionCount);

	UStaticMesh* BodySetupLessCube = DuplicateObject<UStaticMesh>(EngineCube, GetTransientPackage());
	BodySetupLessCube->SetBodySetup(nullptr);
	TestNull(TEXT("BodySetup-less fixture begins without collision storage"), BodySetupLessCube->GetBodySetup());
	TestTrue(
		TEXT("has_collision=true safely creates missing BodySetup before fitting collision"),
		FSetMeshPropertiesCommand::ApplyCollisionSettingForTest(*BodySetupLessCube, true));
	TestNotNull(TEXT("Collision generation creates BodySetup"), BodySetupLessCube->GetBodySetup());
	TestTrue(
		TEXT("BodySetup-less fixture receives a real collision primitive"),
		BodySetupLessCube->GetBodySetup() && BodySetupLessCube->GetBodySetup()->AggGeom.GetElementCount() > 0);

	const FString PackageName = TEXT("/Game/Automation/SM_SetMeshPropertiesCollisionTest");
	const FString AssetName = TEXT("SM_SetMeshPropertiesCollisionTest");
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension());
	UnloadPackageIfLoaded(*this, PackageName);
	IFileManager::Get().Delete(*PackageFilename, false, true, true);

	UPackage* Package = CreatePackage(*PackageName);
	UStaticMesh* CommandMesh = DuplicateObject<UStaticMesh>(EngineCube, Package, *AssetName);
	CommandMesh->SetFlags(RF_Public | RF_Standalone);
	CommandMesh->GetBodySetup()->RemoveSimpleCollision();
	AddValidConvexFixture(*CommandMesh);

	FSetMeshPropertiesCommand Command;
	const int32 PrevalidationCollisionCount = CommandMesh->GetBodySetup()->AggGeom.GetElementCount();
	const bool bPrevalidationNanite = CommandMesh->GetNaniteSettings().bEnabled;
	TArray<TObjectPtr<UMaterialInterface>> PrevalidationMaterials;
	for (const FStaticMaterial& Material : CommandMesh->GetStaticMaterials())
	{
		PrevalidationMaterials.Add(Material.MaterialInterface);
	}
	auto TestPrevalidationState = [this, CommandMesh, PrevalidationCollisionCount, bPrevalidationNanite, &PrevalidationMaterials](
		const TCHAR* Context)
	{
		TestEqual(
			*FString::Printf(TEXT("%s keeps collision unchanged"), Context),
			CommandMesh->GetBodySetup()->AggGeom.GetElementCount(),
			PrevalidationCollisionCount);
		TestEqual(
			*FString::Printf(TEXT("%s keeps Nanite unchanged"), Context),
			CommandMesh->GetNaniteSettings().bEnabled,
			bPrevalidationNanite);
		TArray<TObjectPtr<UMaterialInterface>> CurrentMaterials;
		for (const FStaticMaterial& Material : CommandMesh->GetStaticMaterials())
		{
			CurrentMaterials.Add(Material.MaterialInterface);
		}
		TestEqual(*FString::Printf(TEXT("%s keeps materials unchanged"), Context), CurrentMaterials, PrevalidationMaterials);
	};
	auto TestPrevalidationHasNoExecutionSideEffects = [this, &PackageFilename](const TCHAR* Context)
	{
		TestEqual(
			*FString::Printf(TEXT("%s does not invoke PostEditChange"), Context),
			FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest(),
			0);
		TestEqual(
			*FString::Printf(TEXT("%s does not attempt package save"), Context),
			FSetMeshPropertiesCommand::GetSaveAttemptCountForTest(),
			0);
		TestFalse(
			*FString::Printf(TEXT("%s does not create a package file"), Context),
			IFileManager::Get().FileExists(*PackageFilename));
	};
	TSharedPtr<FJsonObject> InvalidMaterialResponseObject;
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	const FString InvalidMaterialResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false,"enable_nanite":%s,"material_path":""})"),
		*ObjectPath,
		bPrevalidationNanite ? TEXT("false") : TEXT("true")));
	const TSharedRef<TJsonReader<>> InvalidMaterialReader = TJsonReaderFactory<>::Create(InvalidMaterialResponse);
	TestTrue(TEXT("Invalid-material response is valid JSON"), FJsonSerializer::Deserialize(InvalidMaterialReader, InvalidMaterialResponseObject));
	TestFalse(
		TEXT("Invalid material rejects the combined request"),
		InvalidMaterialResponseObject.IsValid() && InvalidMaterialResponseObject->GetBoolField(TEXT("success")));
	TestPrevalidationState(TEXT("Invalid-material prevalidation"));
	TestPrevalidationHasNoExecutionSideEffects(TEXT("Invalid-material prevalidation"));

	TSharedPtr<FJsonObject> InvalidSlotResponseObject;
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	const FString InvalidSlotResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false,"enable_nanite":%s,"material_slot_index":999,"material_slot_path":"/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"})"),
		*ObjectPath,
		bPrevalidationNanite ? TEXT("false") : TEXT("true")));
	const TSharedRef<TJsonReader<>> InvalidSlotReader = TJsonReaderFactory<>::Create(InvalidSlotResponse);
	TestTrue(TEXT("Invalid-slot response is valid JSON"), FJsonSerializer::Deserialize(InvalidSlotReader, InvalidSlotResponseObject));
	TestFalse(
		TEXT("Invalid slot rejects the combined request"),
		InvalidSlotResponseObject.IsValid() && InvalidSlotResponseObject->GetBoolField(TEXT("success")));
	TestPrevalidationState(TEXT("Invalid-slot prevalidation"));
	TestPrevalidationHasNoExecutionSideEffects(TEXT("Invalid-slot prevalidation"));

	TSharedPtr<FJsonObject> MissingSlotPathResponseObject;
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	const FString MissingSlotPathResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false,"enable_nanite":%s,"material_slot_index":0})"),
		*ObjectPath,
		bPrevalidationNanite ? TEXT("false") : TEXT("true")));
	const TSharedRef<TJsonReader<>> MissingSlotPathReader = TJsonReaderFactory<>::Create(MissingSlotPathResponse);
	TestTrue(
		TEXT("Missing-slot-path response is valid JSON"),
		FJsonSerializer::Deserialize(MissingSlotPathReader, MissingSlotPathResponseObject));
	TestFalse(
		TEXT("material_slot_index without material_slot_path rejects the combined request"),
		MissingSlotPathResponseObject.IsValid() && MissingSlotPathResponseObject->GetBoolField(TEXT("success")));
	TestPrevalidationState(TEXT("Missing-slot-path prevalidation"));
	TestPrevalidationHasNoExecutionSideEffects(TEXT("Missing-slot-path prevalidation"));

	TSharedPtr<FJsonObject> MissingSlotIndexResponseObject;
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	const FString MissingSlotIndexResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false,"enable_nanite":%s,"material_slot_path":"/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"})"),
		*ObjectPath,
		bPrevalidationNanite ? TEXT("false") : TEXT("true")));
	const TSharedRef<TJsonReader<>> MissingSlotIndexReader = TJsonReaderFactory<>::Create(MissingSlotIndexResponse);
	TestTrue(
		TEXT("Missing-slot-index response is valid JSON"),
		FJsonSerializer::Deserialize(MissingSlotIndexReader, MissingSlotIndexResponseObject));
	TestFalse(
		TEXT("material_slot_path without material_slot_index rejects the combined request"),
		MissingSlotIndexResponseObject.IsValid() && MissingSlotIndexResponseObject->GetBoolField(TEXT("success")));
	TestPrevalidationState(TEXT("Missing-slot-index prevalidation"));
	TestPrevalidationHasNoExecutionSideEffects(TEXT("Missing-slot-index prevalidation"));

	UMaterialInterface* DefaultMaterialFixture = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	TestNotNull(TEXT("PostEdit notification control material loads"), DefaultMaterialFixture);
	UStaticMesh* PostEditNotificationControl = DuplicateObject<UStaticMesh>(EngineCube, GetTransientPackage());
	int32 SingleMaterialAndPostEditNotificationCount = 0;
	const FDelegateHandle PostEditControlHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
		[&SingleMaterialAndPostEditNotificationCount, PostEditNotificationControl](UObject* Object, FPropertyChangedEvent& Event)
		{
			if (Object == PostEditNotificationControl && Event.GetPropertyName().IsNone())
			{
				++SingleMaterialAndPostEditNotificationCount;
			}
		});
	PostEditNotificationControl->SetMaterial(0, DefaultMaterialFixture);
	PostEditNotificationControl->PostEditChange();
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PostEditControlHandle);
	TestTrue(
		TEXT("One material assignment plus one direct PostEditChange emits generic notifications"),
		SingleMaterialAndPostEditNotificationCount > 0);

	int32 MaterialOnlyPropertyChangeCount = 0;
	int32 MaterialOnlyGenericPostEditCount = 0;
	const FDelegateHandle BaselinePropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
		[&MaterialOnlyPropertyChangeCount, &MaterialOnlyGenericPostEditCount, CommandMesh](UObject* Object, FPropertyChangedEvent& Event)
		{
			if (Object == CommandMesh)
			{
				++MaterialOnlyPropertyChangeCount;
				if (Event.GetPropertyName().IsNone())
				{
					++MaterialOnlyGenericPostEditCount;
				}
			}
		});
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	const FString BaselineResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","material_path":"/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"})"),
		*ObjectPath));
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(BaselinePropertyChangedHandle);
	TSharedPtr<FJsonObject> BaselineResponseObject;
	const TSharedRef<TJsonReader<>> BaselineReader = TJsonReaderFactory<>::Create(BaselineResponse);
	TestTrue(TEXT("Material-only baseline returns valid JSON"), FJsonSerializer::Deserialize(BaselineReader, BaselineResponseObject));
	TestTrue(
		TEXT("Material-only baseline reports success"),
		BaselineResponseObject.IsValid() && BaselineResponseObject->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Material-only baseline emits editor notifications"), MaterialOnlyPropertyChangeCount > 0);
	TestEqual(
		TEXT("Material-only edit emits one material assignment plus one PostEditChange worth of generic notifications"),
		MaterialOnlyGenericPostEditCount,
		SingleMaterialAndPostEditNotificationCount);
	TestEqual(
		TEXT("Material-only edit invokes the command's batched PostEditChange exactly once"),
		FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest(),
		1);
	TestEqual(
		TEXT("Material-only edit attempts one package save"),
		FSetMeshPropertiesCommand::GetSaveAttemptCountForTest(),
		1);
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	TestNotNull(TEXT("Asset editor subsystem is available"), AssetEditorSubsystem);
	const bool bOpenedMeshEditor = AssetEditorSubsystem && AssetEditorSubsystem->OpenEditorForAsset(CommandMesh);
	TestTrue(TEXT("Collision lifecycle fixture opens in Static Mesh Editor"), bOpenedMeshEditor);
	TestTrue(
		TEXT("Static Mesh Editor is open before collision edit"),
		AssetEditorSubsystem && AssetEditorSubsystem->FindEditorForAsset(CommandMesh, false) != nullptr);

	UMaterialInterface* CollisionFailureMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	TestNotNull(TEXT("Collision-failure material fixture loads"), CollisionFailureMaterial);
	const int32 CollisionCountBeforeApplyFailure = CommandMesh->GetBodySetup()->AggGeom.GetElementCount();
	const bool bNaniteBeforeApplyFailure = CommandMesh->GetNaniteSettings().bEnabled;
	TArray<TObjectPtr<UMaterialInterface>> MaterialsBeforeApplyFailure;
	for (const FStaticMaterial& Material : CommandMesh->GetStaticMaterials())
	{
		MaterialsBeforeApplyFailure.Add(Material.MaterialInterface);
	}
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	FSetMeshPropertiesCommand::SetCollisionResultOverrideForTest(false);
	TSharedPtr<FJsonObject> CollisionFailureResponseObject;
	const FString CollisionFailureResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false,"enable_nanite":%s,"material_path":"/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"})"),
		*ObjectPath,
		bNaniteBeforeApplyFailure ? TEXT("false") : TEXT("true")));
	FSetMeshPropertiesCommand::SetCollisionResultOverrideForTest(TOptional<bool>());
	const TSharedRef<TJsonReader<>> CollisionFailureReader = TJsonReaderFactory<>::Create(CollisionFailureResponse);
	TestTrue(
		TEXT("Collision-apply-failure response is valid JSON"),
		FJsonSerializer::Deserialize(CollisionFailureReader, CollisionFailureResponseObject));
	TestFalse(
		TEXT("Collision subsystem failure rejects the combined request"),
		CollisionFailureResponseObject.IsValid() && CollisionFailureResponseObject->GetBoolField(TEXT("success")));
	TestEqual(
		TEXT("Collision subsystem failure leaves collision unchanged"),
		CommandMesh->GetBodySetup()->AggGeom.GetElementCount(),
		CollisionCountBeforeApplyFailure);
	TestEqual(
		TEXT("Collision subsystem failure leaves Nanite unchanged"),
		CommandMesh->GetNaniteSettings().bEnabled,
		bNaniteBeforeApplyFailure);
	TArray<TObjectPtr<UMaterialInterface>> MaterialsAfterApplyFailure;
	for (const FStaticMaterial& Material : CommandMesh->GetStaticMaterials())
	{
		MaterialsAfterApplyFailure.Add(Material.MaterialInterface);
	}
	TestEqual(
		TEXT("Collision subsystem failure leaves materials unchanged"),
		MaterialsAfterApplyFailure,
		MaterialsBeforeApplyFailure);
	TestEqual(
		TEXT("Collision subsystem failure does not invoke PostEditChange"),
		FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest(),
		0);
	TestEqual(
		TEXT("Collision subsystem failure does not attempt package save"),
		FSetMeshPropertiesCommand::GetSaveAttemptCountForTest(),
		0);
	TestTrue(
		TEXT("Collision subsystem failure restores the previously open Static Mesh Editor"),
		AssetEditorSubsystem && AssetEditorSubsystem->FindEditorForAsset(CommandMesh, false) != nullptr);

	int32 MeshPostEditChangeCount = 0;
	int32 CollisionOnlyGenericPostEditCount = 0;
	const FDelegateHandle PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
		[&MeshPostEditChangeCount, &CollisionOnlyGenericPostEditCount, CommandMesh](UObject* Object, FPropertyChangedEvent& Event)
		{
			if (Object == CommandMesh)
			{
				++MeshPostEditChangeCount;
				if (Event.GetPropertyName().IsNone())
				{
					++CollisionOnlyGenericPostEditCount;
				}
			}
		});
	FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
	const FString Response = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false})"),
		*ObjectPath));
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
	TSharedPtr<FJsonObject> ResponseObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	TestTrue(TEXT("Execute returns valid JSON"), FJsonSerializer::Deserialize(Reader, ResponseObject));
	TestTrue(
		TEXT("Execute reports collision removal success"),
		ResponseObject.IsValid() && ResponseObject->GetBoolField(TEXT("success")));
	TestEqual(
		TEXT("Execute call site removes convex collision before saving"),
		CommandMesh->GetBodySetup()->AggGeom.GetElementCount(),
		0);
	TestTrue(TEXT("Collision-only edit emits editor notifications"), MeshPostEditChangeCount > 0);
	TestTrue(TEXT("Collision-only edit reaches generic PostEditChange"), CollisionOnlyGenericPostEditCount > 0);
	TestEqual(
		TEXT("Collision-only edit invokes the command's batched PostEditChange exactly once"),
		FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest(),
		1);
	TestEqual(
		TEXT("Collision-only edit attempts one package save"),
		FSetMeshPropertiesCommand::GetSaveAttemptCountForTest(),
		1);
	TestTrue(
		TEXT("Batched collision rebuild restores the previously open Static Mesh Editor"),
		AssetEditorSubsystem && AssetEditorSubsystem->FindEditorForAsset(CommandMesh, false) != nullptr);
	if (AssetEditorSubsystem)
	{
		AssetEditorSubsystem->CloseAllEditorsForAsset(CommandMesh);
	}
	TestTrue(TEXT("Execute persists the edited mesh package"), IFileManager::Get().FileExists(*PackageFilename));
	Package->SetDirtyFlag(false);
	CommandMesh = nullptr;
	TArray<UPackage*> PackagesToUnload{Package};
	Package = nullptr;
	FText UnloadError;
	TestTrue(TEXT("Saved collision fixture unloads"), UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
	UStaticMesh* ReloadedMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	TestNotNull(TEXT("Saved collision fixture reloads"), ReloadedMesh);
	if (ReloadedMesh)
	{
		TestEqual(
			TEXT("Serialized mesh remains collision-free after reload"),
			ReloadedMesh->GetBodySetup()->AggGeom.GetElementCount(),
			0);

		int32 CollisionAddPropertyChangeCount = 0;
		int32 CollisionAddGenericPostEditCount = 0;
		const FDelegateHandle CollisionAddHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
			[&CollisionAddPropertyChangeCount, &CollisionAddGenericPostEditCount, ReloadedMesh](UObject* Object, FPropertyChangedEvent& Event)
			{
				if (Object == ReloadedMesh)
				{
					++CollisionAddPropertyChangeCount;
					if (Event.GetPropertyName().IsNone())
					{
						++CollisionAddGenericPostEditCount;
					}
				}
			});
		TSharedPtr<FJsonObject> CollisionAddResponseObject;
		FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
		const FString CollisionAddResponse = Command.Execute(FString::Printf(
			TEXT(R"({"mesh_path":"%s","has_collision":true})"),
			*ObjectPath));
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(CollisionAddHandle);
		const TSharedRef<TJsonReader<>> CollisionAddReader = TJsonReaderFactory<>::Create(CollisionAddResponse);
		TestTrue(
			TEXT("Collision-add response is valid JSON"),
			FJsonSerializer::Deserialize(CollisionAddReader, CollisionAddResponseObject));
		TestTrue(
			TEXT("Collision-only Execute(true) reports success"),
			CollisionAddResponseObject.IsValid() && CollisionAddResponseObject->GetBoolField(TEXT("success")));
		TestTrue(
			TEXT("Collision-only Execute(true) creates collision"),
			ReloadedMesh->GetBodySetup()->AggGeom.GetElementCount() > 0);
		TestTrue(TEXT("Collision-add emits editor notifications"), CollisionAddPropertyChangeCount > 0);
		TestTrue(TEXT("Collision-add reaches generic PostEditChange"), CollisionAddGenericPostEditCount > 0);
		TestEqual(
			TEXT("Collision-add invokes the command's batched PostEditChange exactly once"),
			FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest(),
			1);

		UPackage* ReloadedPackage = ReloadedMesh->GetOutermost();
		ReloadedPackage->SetDirtyFlag(false);
		ReloadedMesh = nullptr;
		PackagesToUnload = {ReloadedPackage};
		UnloadError = FText::GetEmpty();
		TestTrue(TEXT("Collision-add fixture unloads"), UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
		ReloadedMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		TestNotNull(TEXT("Collision-add fixture reloads"), ReloadedMesh);
	}
	if (ReloadedMesh)
	{
		TestTrue(
			TEXT("Serialized mesh keeps generated collision after reload"),
			ReloadedMesh->GetBodySetup()->AggGeom.GetElementCount() > 0);
		UMaterialInterface* BasicShapeMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		TestNotNull(TEXT("Combined-edit material fixture loads"), BasicShapeMaterial);
		const bool bRequestedNanite = !ReloadedMesh->GetNaniteSettings().bEnabled;
		int32 CombinedPropertyChangeCount = 0;
		int32 CombinedGenericPostEditCount = 0;
		const FDelegateHandle CombinedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
			[&CombinedPropertyChangeCount, &CombinedGenericPostEditCount, ReloadedMesh](UObject* Object, FPropertyChangedEvent& Event)
			{
				if (Object == ReloadedMesh)
				{
					++CombinedPropertyChangeCount;
					if (Event.GetPropertyName().IsNone())
					{
						++CombinedGenericPostEditCount;
					}
				}
			});
		TSharedPtr<FJsonObject> CombinedResponseObject;
		FSetMeshPropertiesCommand::ResetExecutionCountersForTest();
		const FString CombinedResponse = Command.Execute(FString::Printf(
			TEXT(R"({"mesh_path":"%s","has_collision":false,"enable_nanite":%s,"material_path":"/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"})"),
			*ObjectPath,
			bRequestedNanite ? TEXT("true") : TEXT("false")));
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(CombinedHandle);
		const TSharedRef<TJsonReader<>> CombinedReader = TJsonReaderFactory<>::Create(CombinedResponse);
		TestTrue(TEXT("Combined response is valid JSON"), FJsonSerializer::Deserialize(CombinedReader, CombinedResponseObject));
		TestTrue(
			TEXT("Combined Nanite/material/collision edit reports success"),
			CombinedResponseObject.IsValid() && CombinedResponseObject->GetBoolField(TEXT("success")));
		TestTrue(TEXT("Combined edit emits editor notifications"), CombinedPropertyChangeCount > 0);
		TestEqual(
			TEXT("Combined edit emits one material assignment plus one PostEditChange worth of generic notifications"),
			CombinedGenericPostEditCount,
			SingleMaterialAndPostEditNotificationCount);
		TestEqual(
			TEXT("Combined Nanite/material/collision edit invokes the command's batched PostEditChange exactly once"),
			FSetMeshPropertiesCommand::GetPostEditChangeCallCountForTest(),
			1);
		TestEqual(TEXT("Combined edit applies Nanite setting"), ReloadedMesh->GetNaniteSettings().bEnabled, bRequestedNanite);
		TestEqual(TEXT("Combined edit removes collision"), ReloadedMesh->GetBodySetup()->AggGeom.GetElementCount(), 0);
		TestTrue(TEXT("Combined edit applies requested material"), ReloadedMesh->GetMaterial(0) == BasicShapeMaterial);

		UPackage* ReloadedPackage = ReloadedMesh->GetOutermost();
		ReloadedPackage->SetDirtyFlag(false);
		ReloadedMesh = nullptr;
		PackagesToUnload = {ReloadedPackage};
		UnloadError = FText::GetEmpty();
		TestTrue(TEXT("Combined fixture unloads"), UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
		ReloadedMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		TestNotNull(TEXT("Combined fixture reloads"), ReloadedMesh);
		if (ReloadedMesh)
		{
			TestEqual(TEXT("Serialized combined edit keeps Nanite setting"), ReloadedMesh->GetNaniteSettings().bEnabled, bRequestedNanite);
			TestEqual(TEXT("Serialized combined edit stays collision-free"), ReloadedMesh->GetBodySetup()->AggGeom.GetElementCount(), 0);
			TestTrue(TEXT("Serialized combined edit keeps requested material"), ReloadedMesh->GetMaterial(0) == BasicShapeMaterial);

			const bool bNaniteOnlyTarget = !bRequestedNanite;
			int32 NaniteOnlyGenericPostEditCount = 0;
			const FDelegateHandle NaniteOnlyHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
				[&NaniteOnlyGenericPostEditCount, ReloadedMesh](UObject* Object, FPropertyChangedEvent& Event)
				{
					if (Object == ReloadedMesh && Event.GetPropertyName().IsNone())
					{
						++NaniteOnlyGenericPostEditCount;
					}
				});
			TSharedPtr<FJsonObject> NaniteOnlyResponseObject;
			const FString NaniteOnlyResponse = Command.Execute(FString::Printf(
				TEXT(R"({"mesh_path":"%s","enable_nanite":%s})"),
				*ObjectPath,
				bNaniteOnlyTarget ? TEXT("true") : TEXT("false")));
			FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(NaniteOnlyHandle);
			const TSharedRef<TJsonReader<>> NaniteOnlyReader = TJsonReaderFactory<>::Create(NaniteOnlyResponse);
			TestTrue(TEXT("Nanite-only response is valid JSON"), FJsonSerializer::Deserialize(NaniteOnlyReader, NaniteOnlyResponseObject));
			TestTrue(
				TEXT("Nanite-only edit reports success"),
				NaniteOnlyResponseObject.IsValid() && NaniteOnlyResponseObject->GetBoolField(TEXT("success")));
			TestTrue(TEXT("Nanite-only edit reaches generic PostEditChange"), NaniteOnlyGenericPostEditCount > 0);
			TestEqual(TEXT("Nanite-only edit applies requested state"), ReloadedMesh->GetNaniteSettings().bEnabled, bNaniteOnlyTarget);

			UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
			TestNotNull(TEXT("Per-slot material fixture loads"), DefaultMaterial);
			TSharedPtr<FJsonObject> PerSlotResponseObject;
			const FString PerSlotResponse = Command.Execute(FString::Printf(
				TEXT(R"({"mesh_path":"%s","material_slot_index":0,"material_slot_path":"/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"})"),
				*ObjectPath));
			const TSharedRef<TJsonReader<>> PerSlotReader = TJsonReaderFactory<>::Create(PerSlotResponse);
			TestTrue(TEXT("Per-slot material response is valid JSON"), FJsonSerializer::Deserialize(PerSlotReader, PerSlotResponseObject));
			TestTrue(
				TEXT("Valid per-slot material edit reports success"),
				PerSlotResponseObject.IsValid() && PerSlotResponseObject->GetBoolField(TEXT("success")));
			TestTrue(TEXT("Valid per-slot material edit applies requested slot"), ReloadedMesh->GetMaterial(0) == DefaultMaterial);
			UPackage* FinalPackage = ReloadedMesh->GetOutermost();
			FinalPackage->SetDirtyFlag(false);
			ReloadedMesh = nullptr;
			PackagesToUnload = {FinalPackage};
			UnloadError = FText::GetEmpty();
			TestTrue(TEXT("Final collision fixture unloads"), UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
			ReloadedMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
			TestNotNull(TEXT("Nanite-only fixture reloads"), ReloadedMesh);
			if (ReloadedMesh)
			{
				TestEqual(
					TEXT("Serialized Nanite-only edit keeps requested state"),
					ReloadedMesh->GetNaniteSettings().bEnabled,
					bNaniteOnlyTarget);
				TestTrue(TEXT("Serialized per-slot edit keeps requested material"), ReloadedMesh->GetMaterial(0) == DefaultMaterial);
				UPackage* NaniteOnlyPackage = ReloadedMesh->GetOutermost();
				NaniteOnlyPackage->SetDirtyFlag(false);
				ReloadedMesh = nullptr;
				PackagesToUnload = {NaniteOnlyPackage};
				UnloadError = FText::GetEmpty();
				TestTrue(TEXT("Nanite-only fixture unloads"), UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
			}
		}
	}
	TestTrue(TEXT("Automation fixture package is cleaned up"), IFileManager::Get().Delete(*PackageFilename, false, true, true));

	const FString FailurePackageName = TEXT("/Game/Automation/SM_SetMeshPropertiesSaveFailure");
	const FString FailureAssetName = TEXT("SM_SetMeshPropertiesSaveFailure");
	const FString FailureObjectPath = FString::Printf(
		TEXT("%s.%s"),
		*FailurePackageName,
		*FailureAssetName);
	const FString FailureFilename = FPackageName::LongPackageNameToFilename(
		FailurePackageName,
		FPackageName::GetAssetPackageExtension());
	UnloadPackageIfLoaded(*this, FailurePackageName);
	IFileManager::Get().Delete(*FailureFilename, false, true, true);
	UPackage* FailurePackage = CreatePackage(*FailurePackageName);
	UStaticMesh* UnsavableMesh = DuplicateObject<UStaticMesh>(EngineCube, FailurePackage, *FailureAssetName);
	UnsavableMesh->SetFlags(RF_Public | RF_Standalone);
	FSetMeshPropertiesCommand::SetSaveResultOverrideForTest(false);
	const FString FailureResponse = Command.Execute(FString::Printf(
		TEXT(R"({"mesh_path":"%s","has_collision":false})"),
		*FailureObjectPath));
	FSetMeshPropertiesCommand::SetSaveResultOverrideForTest(TOptional<bool>());
	TSharedPtr<FJsonObject> FailureResponseObject;
	const TSharedRef<TJsonReader<>> FailureReader = TJsonReaderFactory<>::Create(FailureResponse);
	TestTrue(TEXT("Save-failure response is valid JSON"), FJsonSerializer::Deserialize(FailureReader, FailureResponseObject));
	TestFalse(
		TEXT("Execute reports package-save failure"),
		FailureResponseObject.IsValid() && FailureResponseObject->GetBoolField(TEXT("success")));
	TestFalse(TEXT("Simulated save failure does not create a package file"), IFileManager::Get().FileExists(*FailureFilename));
	FailurePackage->SetDirtyFlag(false);
	UnsavableMesh = nullptr;
	PackagesToUnload = {FailurePackage};
	FailurePackage = nullptr;
	UnloadError = FText::GetEmpty();
	TestTrue(TEXT("Save-failure fixture unloads"), UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true));
	TestTrue(TEXT("Save-failure fixture file is absent after cleanup"), IFileManager::Get().Delete(*FailureFilename, false, true, true));

	return true;
}

#endif
