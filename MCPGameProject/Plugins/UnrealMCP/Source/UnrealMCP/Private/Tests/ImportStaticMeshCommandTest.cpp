#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Editor/ImportStaticMeshCommand.h"

#include "EditorFramework/AssetImportData.h"
#include "Engine/StaticMesh.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FImportStaticMeshOptionsTest,
	"UnrealMCP.Editor.ImportStaticMesh.Options",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FImportStaticMeshReimportIntegrationTest,
	"UnrealMCP.Editor.ImportStaticMesh.ReimportIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	bool ParseResponse(const FString& Response, TSharedPtr<FJsonObject>& OutObject)
	{
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
		return FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid();
	}

	bool SaveMeshPackage(UStaticMesh& Mesh)
	{
		UPackage* Package = Mesh.GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, &Mesh, *Filename, SaveArgs);
	}

	TArray<FColor> GetLOD0VertexColors(const UStaticMesh* Mesh)
	{
		TArray<FColor> Colors;
		const FStaticMeshRenderData* RenderData = Mesh ? Mesh->GetRenderData() : nullptr;
		if (!RenderData || RenderData->LODResources.IsEmpty())
		{
			return Colors;
		}
		const FColorVertexBuffer& ColorBuffer = RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer;
		Colors.Reserve(ColorBuffer.GetNumVertices());
		for (uint32 VertexIndex = 0; VertexIndex < ColorBuffer.GetNumVertices(); ++VertexIndex)
		{
			Colors.Add(ColorBuffer.VertexColor(VertexIndex));
		}
		return Colors;
	}

	bool CleanupAutomationAsset(
		FAutomationTestBase& Test,
		const FString& PackageName,
		const FString& PackageFilename,
		UStaticMesh*& Mesh)
	{
		UPackage* Package = Mesh ? Mesh->GetOutermost() : FindPackage(nullptr, *PackageName);
		bool bUnloaded = true;
		if (Package)
		{
			Package->SetDirtyFlag(false);
			TArray<UPackage*> PackagesToUnload{Package};
			Mesh = nullptr;
			FText UnloadError;
			bUnloaded = UPackageTools::UnloadPackages(PackagesToUnload, UnloadError, true);
			Test.TestTrue(TEXT("Automation mesh package unloads before cleanup"), bUnloaded);
		}
		const bool bDeleted = IFileManager::Get().Delete(*PackageFilename, false, true, true);
		Test.TestTrue(TEXT("Automation mesh package is cleaned up"), bDeleted);
		return bUnloaded && bDeleted;
	}
}

bool FImportStaticMeshOptionsTest::RunTest(const FString& Parameters)
{
	FImportStaticMeshCommand Command;
	FString Error;
	FImportStaticMeshSettings Settings;

	const FString DefaultsJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","folder_path":"/Game/Test","import_materials":false})");
	TestTrue(TEXT("Legacy payload parses"), Command.ParseParametersForTest(DefaultsJson, Settings, Error));
	TestTrue(TEXT("Legacy collision default stays enabled"), Settings.bAutoGenerateCollision);
	TestEqual(
		TEXT("Legacy vertex color default stays Replace"),
		Settings.VertexColorImportOption,
		EVertexColorImportOption::Replace);

	UFbxImportUI* DefaultsUI = NewObject<UFbxImportUI>();
	Command.ConfigureImportUIForTest(*DefaultsUI, Settings);
	TestTrue(TEXT("Default UI enables collision"), DefaultsUI->StaticMeshImportData->bAutoGenerateCollision);
	TestEqual(
		TEXT("Default UI preserves legacy FBX vertex color import"),
		DefaultsUI->StaticMeshImportData->VertexColorImportOption.GetValue(),
		EVertexColorImportOption::Replace);

	const FString ExplicitJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","auto_generate_collision":false,"vertex_color_import_option":"Ignore"})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestTrue(TEXT("Explicit payload parses"), Command.ParseParametersForTest(ExplicitJson, Settings, Error));
	TestFalse(TEXT("Explicit collision option disables collision"), Settings.bAutoGenerateCollision);
	TestEqual(
		TEXT("Explicit vertex color option selects Ignore"),
		Settings.VertexColorImportOption,
		EVertexColorImportOption::Ignore);

	UFbxImportUI* ExplicitUI = NewObject<UFbxImportUI>();
	Command.ConfigureImportUIForTest(*ExplicitUI, Settings);
	TestFalse(TEXT("Configured UI disables collision"), ExplicitUI->StaticMeshImportData->bAutoGenerateCollision);
	TestEqual(
		TEXT("Configured UI ignores FBX vertex colors"),
		ExplicitUI->StaticMeshImportData->VertexColorImportOption.GetValue(),
		EVertexColorImportOption::Ignore);

	const FString OverrideJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Override","vertex_override_color":[0.2,0.4,0.6,0.8]})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestTrue(TEXT("Override payload with color parses"), Command.ParseParametersForTest(OverrideJson, Settings, Error));
	TestEqual(TEXT("Override option is selected"), Settings.VertexColorImportOption, EVertexColorImportOption::Override);
	UFbxImportUI* OverrideUI = NewObject<UFbxImportUI>();
	Command.ConfigureImportUIForTest(*OverrideUI, Settings);
	TestEqual(
		TEXT("Override color reaches import UI"),
		OverrideUI->StaticMeshImportData->VertexOverrideColor,
		FColor(51, 102, 153, 204));

	const FString MissingOverrideColorJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Override"})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(
		TEXT("Override without explicit color is rejected"),
		Command.ParseParametersForTest(MissingOverrideColorJson, Settings, Error));

	const FString ThreeComponentOverrideJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Override","vertex_override_color":[0.2,0.4,0.6]})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(TEXT("Override requires explicit RGBA"), Command.ParseParametersForTest(ThreeComponentOverrideJson, Settings, Error));

	const FString OutOfRangeOverrideJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Override","vertex_override_color":[0.2,1.2,0.6,1.0]})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(TEXT("Override rejects out-of-range components"), Command.ParseParametersForTest(OutOfRangeOverrideJson, Settings, Error));
	TestTrue(TEXT("Range diagnostic explains accepted interval"), Error.Contains(TEXT("0.0 and 1.0")));

	const FString NonNumericOverrideJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Override","vertex_override_color":[0.2,"0.4",0.6,0.8]})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(TEXT("Override rejects non-numeric RGBA components"), Command.ParseParametersForTest(NonNumericOverrideJson, Settings, Error));
	TestTrue(TEXT("Non-numeric diagnostic explains the numeric contract"), Error.Contains(TEXT("must be numbers")));

	const FString InvalidJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Flatten"})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(TEXT("Invalid vertex color option is rejected"), Command.ParseParametersForTest(InvalidJson, Settings, Error));
	TestTrue(TEXT("Invalid option diagnostic lists valid values"), Error.Contains(TEXT("Ignore, Replace, Override")));

	const FString TempSourcePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealMCPImportOptions.fbx"));
	TestTrue(TEXT("temporary source file is created"), FFileHelper::SaveStringToFile(TEXT("test seam"), *TempSourcePath));
	const FString JsonSourcePath = TempSourcePath.Replace(TEXT("\\"), TEXT("/"));
	const FString SeamFolderPath = TEXT("/Game/Automation/ImportStaticMeshExecutionOverrideTest");
	const FString SeamAssetName = TEXT("SM_ExecutionOverrideGuard");
	const FString SeamPackageName = SeamFolderPath / SeamAssetName;
	const FString SeamPackageFilename = FPackageName::LongPackageNameToFilename(
		SeamPackageName,
		FPackageName::GetAssetPackageExtension());
	UStaticMesh* ExistingSeamMesh = NewObject<UStaticMesh>(
		CreatePackage(*SeamPackageName),
		*SeamAssetName,
		RF_Public | RF_Standalone);
	UAssetImportData* ExistingSeamImportData = NewObject<UAssetImportData>(ExistingSeamMesh);
	ExistingSeamImportData->Update(TEXT("E:/sentinel-before-seam.fbx"));
	ExistingSeamMesh->SetAssetImportData(ExistingSeamImportData);
	ExistingSeamMesh->GetOutermost()->SetDirtyFlag(false);
	const FString ExecuteJson = FString::Printf(
		TEXT(R"({"source_file_path":"%s","asset_name":"%s","folder_path":"%s","auto_generate_collision":false,"vertex_color_import_option":"Ignore"})"),
		*JsonSourcePath,
		*SeamAssetName,
		*SeamFolderPath);

	bool bExecutionOverrideCalled = false;
	FImportStaticMeshCommand ExecuteCommand(
		[&bExecutionOverrideCalled, ExistingSeamMesh, ExistingSeamImportData, this](
			const FImportStaticMeshSettings& Received,
			const UFbxImportUI& ImportUI)
		{
			bExecutionOverrideCalled = true;
			TestFalse(TEXT("Execute forwards collision setting"), Received.bAutoGenerateCollision);
			TestEqual(
				TEXT("Execute forwards parsed vertex mode"),
				Received.VertexColorImportOption,
				EVertexColorImportOption::Ignore);
			TestFalse(TEXT("real Execute call-site configures collision on FBX UI"), ImportUI.StaticMeshImportData->bAutoGenerateCollision);
			TestEqual(
				TEXT("real Execute call-site configures vertex mode on FBX UI"),
				ImportUI.StaticMeshImportData->VertexColorImportOption.GetValue(),
				EVertexColorImportOption::Ignore);
			TestTrue(
				TEXT("Execution seam runs before replacing existing non-FBX metadata"),
				ExistingSeamMesh->GetAssetImportData() == ExistingSeamImportData);
			TestTrue(
				TEXT("Execution seam sees original source provenance"),
				ExistingSeamImportData->GetFirstFilename().EndsWith(TEXT("sentinel-before-seam.fbx")));
			TestFalse(
				TEXT("Execution seam runs before dirtying the existing package"),
				ExistingSeamMesh->GetOutermost()->IsDirty());
			return FString(TEXT("{\"success\":true}"));
		});
	const FString ExecuteResponse = ExecuteCommand.Execute(ExecuteJson);
	TestTrue(TEXT("Execute invokes the import seam"), bExecutionOverrideCalled);
	TestTrue(TEXT("Execute returns seam response"), ExecuteResponse.Contains(TEXT("\"success\":true")));
	TestTrue(
		TEXT("Execution seam leaves existing import metadata untouched after return"),
		ExistingSeamMesh->GetAssetImportData() == ExistingSeamImportData);
	TestFalse(TEXT("Execution seam does not save the existing package"), IFileManager::Get().FileExists(*SeamPackageFilename));
	CleanupAutomationAsset(*this, SeamPackageName, SeamPackageFilename, ExistingSeamMesh);
	TestTrue(TEXT("temporary source file is cleaned up"), IFileManager::Get().Delete(*TempSourcePath, false, true, true));

	UStaticMesh* ExistingMesh = NewObject<UStaticMesh>();
	UAssetImportData* NonLegacyImportData = NewObject<UAssetImportData>(ExistingMesh);
	NonLegacyImportData->Update(TEXT("E:/old-source.fbx"));
	ExistingMesh->SetAssetImportData(NonLegacyImportData);

	UFbxImportUI* ReimportUI = NewObject<UFbxImportUI>();
	Settings = FImportStaticMeshSettings{};
	Settings.bAutoGenerateCollision = false;
	Settings.VertexColorImportOption = EVertexColorImportOption::Replace;
	ReimportUI->StaticMeshImportData->bCombineMeshes = false;
	ReimportUI->StaticMeshImportData->bConvertSceneUnit = true;
	FImportStaticMeshCommand::ConfigureImportUIForTest(*ReimportUI, Settings);
	UFbxStaticMeshImportData* PreparedImportData =
		FImportStaticMeshCommand::PrepareExistingMeshForReimportForTest(
			*ExistingMesh,
			*ReimportUI,
			Settings,
			TEXT("E:/new-source.fbx"));

	TestNotNull(TEXT("Non-legacy metadata is replaced with FBX static-mesh import data"), PreparedImportData);
	TestTrue(TEXT("Prepared metadata is installed on the real mesh"), ExistingMesh->GetAssetImportData() == PreparedImportData);
	TestTrue(TEXT("Same-path reimport combines the requested target"), PreparedImportData->bCombineMeshes);
	TestTrue(TEXT("Same-path reimport preserves scene-unit conversion"), PreparedImportData->bConvertSceneUnit);
	TestFalse(TEXT("Same-path reimport honors disabled collision"), PreparedImportData->bAutoGenerateCollision);
	TestEqual(
		TEXT("Same-path reimport honors source vertex colors"),
		PreparedImportData->VertexColorImportOption.GetValue(),
		EVertexColorImportOption::Replace);
	TestTrue(
		TEXT("Same-path reimport updates source provenance"),
		PreparedImportData->GetFirstFilename().EndsWith(TEXT("new-source.fbx")));

	UStaticMesh* LegacyMesh = NewObject<UStaticMesh>();
	UFbxStaticMeshImportData* LegacyImportData = NewObject<UFbxStaticMeshImportData>(LegacyMesh);
	LegacyImportData->bCombineMeshes = false;
	LegacyImportData->bConvertSceneUnit = false;
	LegacyImportData->bAutoGenerateCollision = true;
	LegacyImportData->VertexColorImportOption = EVertexColorImportOption::Ignore;
	LegacyImportData->Update(TEXT("E:/legacy-source.fbx"));
	LegacyMesh->SetAssetImportData(LegacyImportData);
	UFbxStaticMeshImportData* ReusedLegacyImportData =
		FImportStaticMeshCommand::PrepareExistingMeshForReimportForTest(
			*LegacyMesh,
			*ReimportUI,
			Settings,
			TEXT("E:/legacy-new-source.fbx"));
	TestTrue(TEXT("Existing FBX metadata object is reused"), ReusedLegacyImportData == LegacyImportData);
	TestTrue(TEXT("Legacy metadata now combines the target"), ReusedLegacyImportData->bCombineMeshes);
	TestTrue(TEXT("Legacy metadata now converts scene units"), ReusedLegacyImportData->bConvertSceneUnit);
	TestFalse(TEXT("Legacy metadata now disables generated collision"), ReusedLegacyImportData->bAutoGenerateCollision);
	TestEqual(
		TEXT("Legacy metadata now imports source vertex colors"),
		ReusedLegacyImportData->VertexColorImportOption.GetValue(),
		EVertexColorImportOption::Replace);
	TestTrue(
		TEXT("Legacy metadata source path is updated"),
		ReusedLegacyImportData->GetFirstFilename().EndsWith(TEXT("legacy-new-source.fbx")));

	Settings.VertexColorImportOption = EVertexColorImportOption::Override;
	Settings.VertexOverrideColor = FColor(26, 77, 128, 204);
	Settings.bHasVertexOverrideColor = true;
	FImportStaticMeshCommand::PrepareExistingMeshForReimportForTest(
		*LegacyMesh,
		*ReimportUI,
		Settings,
		TEXT("E:/legacy-override-source.fbx"));
	TestEqual(
		TEXT("Same-path reimport persists the Override vertex-color mode"),
		LegacyImportData->VertexColorImportOption.GetValue(),
		EVertexColorImportOption::Override);
	TestEqual(
		TEXT("Same-path reimport persists the explicit vertex override color"),
		LegacyImportData->VertexOverrideColor,
		Settings.VertexOverrideColor);

	return true;
}

bool FImportStaticMeshReimportIntegrationTest::RunTest(const FString& Parameters)
{
	const FString SourceFbx = FPaths::EngineContentDir() / TEXT("FbxEditorAutomation/BlenderCube.fbx");
	const FString ReimportSourceFbx = FPaths::EngineContentDir() / TEXT("FbxEditorAutomation/LOD_StaticMesh.fbx");
	const FString FailureSourceFbx = FPaths::ProjectSavedDir() / TEXT("Automation/UnrealMCP_EmptyScene.fbx");
	const FString FolderPath = TEXT("/Game/Automation/ImportStaticMeshCommandTest");
	const FString AssetName = TEXT("SM_ReimportContractTest");
	const FString PackageName = FolderPath / AssetName;
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension());
	UStaticMesh* Mesh = FindObject<UStaticMesh>(nullptr, *ObjectPath);
	CleanupAutomationAsset(*this, PackageName, PackageFilename, Mesh);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FailureSourceFbx), true);
	const FString EmptyFbx = TEXT(
		"; FBX 7.4.0 project file\n"
		"FBXHeaderExtension: {\n"
		"  FBXHeaderVersion: 1003\n"
		"  FBXVersion: 7400\n"
		"  Creator: \"UnrealMCP automation\"\n"
		"}\n"
		"GlobalSettings: { Version: 1000 }\n"
		"Definitions: { Version: 100 Count: 0 }\n"
		"Objects: { }\n"
		"Connections: { }\n");
	TestTrue(TEXT("Empty FBX failure fixture is written"), FFileHelper::SaveStringToFile(EmptyFbx, *FailureSourceFbx));

	FImportStaticMeshCommand Command;
	AddExpectedMessagePlain(
		TEXT("Failed to find object 'StaticMesh /Game/Automation/ImportStaticMeshCommandTest/SM_ReimportContractTest.SM_ReimportContractTest'"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		1);
	AddExpectedMessagePlain(
		TEXT("has degenerate tangent bases which will result in incorrect shading"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		1);
	AddExpectedMessagePlain(
		TEXT("has some nearly zero bi-normals which can create some issues"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		1);
	AddExpectedMessagePlain(
		TEXT("-- import failed"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		2);
	auto MakePayload = [&AssetName](
		const FString& SourcePath,
		const FString& PayloadFolderPath,
		bool bAutoGenerateCollision,
		const TCHAR* VertexColorOption)
	{
		return FString::Printf(
			TEXT(R"({"source_file_path":"%s","asset_name":"%s","folder_path":"%s","import_materials":false,"auto_generate_collision":%s,"vertex_color_import_option":"%s"})"),
			*SourcePath.Replace(TEXT("\\"), TEXT("/")),
			*AssetName,
			*PayloadFolderPath,
			bAutoGenerateCollision ? TEXT("true") : TEXT("false"),
			VertexColorOption);
	};

	TSharedPtr<FJsonObject> FirstResponse;
	TestTrue(TEXT("First import response is valid JSON"), ParseResponse(
		Command.Execute(MakePayload(SourceFbx, FolderPath, false, TEXT("Replace"))),
		FirstResponse));
	TestTrue(TEXT("First import succeeds"), FirstResponse.IsValid() && FirstResponse->GetBoolField(TEXT("success")));
	Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	TestNotNull(TEXT("First import creates requested mesh"), Mesh);
	if (!Mesh || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.IsEmpty())
	{
		CleanupAutomationAsset(*this, PackageName, PackageFilename, Mesh);
		IFileManager::Get().Delete(*FailureSourceFbx, false, true, true);
		return false;
	}
	const int32 FirstTriangles = Mesh->GetRenderData()->LODResources[0].GetNumTriangles();
	const FVector FirstBounds = Mesh->GetBounds().BoxExtent;
	TestTrue(TEXT("First import has render triangles"), FirstTriangles > 0);
	TestTrue(TEXT("First import saves successfully"), SaveMeshPackage(*Mesh));

	TSharedPtr<FJsonObject> UnsupportedMaterialReimportResponse;
	const FString UnsupportedMaterialReimportPayload = FString::Printf(
		TEXT(R"({"source_file_path":"%s","asset_name":"%s","folder_path":"%s","import_materials":true,"auto_generate_collision":false,"vertex_color_import_option":"Replace"})"),
		*SourceFbx.Replace(TEXT("\\"), TEXT("/")),
		*AssetName,
		*FolderPath);
	TestTrue(TEXT("Unsupported material-reimport response is valid JSON"), ParseResponse(
		Command.Execute(UnsupportedMaterialReimportPayload),
		UnsupportedMaterialReimportResponse));
	TestFalse(
		TEXT("Same-path reimport rejects import_materials=true instead of reporting an ignored option as success"),
		UnsupportedMaterialReimportResponse.IsValid()
			&& UnsupportedMaterialReimportResponse->GetBoolField(TEXT("success")));
	TestTrue(
		TEXT("Rejected material reimport leaves the existing UObject untouched"),
		LoadObject<UStaticMesh>(nullptr, *ObjectPath) == Mesh);
	TestEqual(
		TEXT("Rejected material reimport leaves geometry untouched"),
		static_cast<int32>(Mesh->GetRenderData()->LODResources[0].GetNumTriangles()),
		FirstTriangles);

	UAssetImportData* NonLegacyImportData = NewObject<UAssetImportData>(Mesh);
	NonLegacyImportData->Update(TEXT("E:/interchange-placeholder.fbx"));
	Mesh->SetAssetImportData(NonLegacyImportData);
	Mesh->MarkPackageDirty();
	TestTrue(TEXT("Non-legacy metadata fixture saves successfully"), SaveMeshPackage(*Mesh));
	FText ReloadError;
	TArray<UPackage*> PackagesToReload{Mesh->GetOutermost()};
	TestTrue(
		TEXT("Saved non-legacy fixture reloads before same-path reimport"),
		UPackageTools::ReloadPackages(
			PackagesToReload,
			ReloadError,
			EReloadPackagesInteractionMode::AssumePositive));
	Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	TestNotNull(TEXT("Reloaded non-legacy fixture loads"), Mesh);
	if (Mesh)
	{
		UStaticMesh* MeshBeforeNonLegacyFailure = Mesh;
		const FString NonLegacyProvenance = Mesh->GetAssetImportData()
			? Mesh->GetAssetImportData()->GetFirstFilename()
			: FString();
		TArray<TObjectPtr<UMaterialInterface>> NonLegacyMaterials;
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			NonLegacyMaterials.Add(Material.MaterialInterface);
		}
		AddExpectedMessagePlain(
			TEXT("No FBX mesh found when reimport Unreal mesh"),
			ELogVerbosity::Error,
			EAutomationExpectedMessageFlags::Contains,
			1);
		AddExpectedMessagePlain(
			TEXT("Fail to reimport mesh"),
			ELogVerbosity::Error,
			EAutomationExpectedMessageFlags::Contains,
			1);
		TSharedPtr<FJsonObject> NonLegacyFailureResponse;
		TestTrue(TEXT("Non-legacy failure response is valid JSON"), ParseResponse(
			Command.Execute(MakePayload(FailureSourceFbx, FolderPath, true, TEXT("Ignore"))),
			NonLegacyFailureResponse));
		TestFalse(
			TEXT("Failed same-path reimport from non-legacy metadata reports failure"),
			NonLegacyFailureResponse.IsValid() && NonLegacyFailureResponse->GetBoolField(TEXT("success")));
		Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		TestTrue(TEXT("Non-legacy atomic failure restores a replacement UObject"), Mesh && Mesh != MeshBeforeNonLegacyFailure);
		TestTrue(
			TEXT("Non-legacy failure restores the exact metadata class"),
			Mesh && Mesh->GetAssetImportData() && Mesh->GetAssetImportData()->GetClass() == UAssetImportData::StaticClass());
		TestTrue(
			TEXT("Non-legacy failure restores provenance"),
			Mesh && Mesh->GetAssetImportData() && Mesh->GetAssetImportData()->GetFirstFilename() == NonLegacyProvenance);
		const FStaticMeshRenderData* NonLegacyRestoredRenderData = Mesh ? Mesh->GetRenderData() : nullptr;
		TestNotNull(TEXT("Non-legacy atomic failure restores render data"), NonLegacyRestoredRenderData);
		TestTrue(
			TEXT("Non-legacy atomic failure restores LOD0 resources"),
			NonLegacyRestoredRenderData && !NonLegacyRestoredRenderData->LODResources.IsEmpty());
		if (NonLegacyRestoredRenderData && !NonLegacyRestoredRenderData->LODResources.IsEmpty())
		{
			TestEqual(
				TEXT("Non-legacy failure restores triangle count"),
				static_cast<int32>(NonLegacyRestoredRenderData->LODResources[0].GetNumTriangles()),
				FirstTriangles);
			TestTrue(TEXT("Non-legacy failure restores bounds"), Mesh->GetBounds().BoxExtent.Equals(FirstBounds, 0.01));
			TArray<TObjectPtr<UMaterialInterface>> RestoredNonLegacyMaterials;
			for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
			{
				RestoredNonLegacyMaterials.Add(Material.MaterialInterface);
			}
			TestEqual(
				TEXT("Non-legacy failure restores material interfaces"),
				RestoredNonLegacyMaterials,
				NonLegacyMaterials);
		}
	}

	TSharedPtr<FJsonObject> SecondResponse;
	const FColor PersistedOverrideColor(26, 77, 128, 204);
	const FString OverrideReimportPayload = FString::Printf(
		TEXT(R"({"source_file_path":"%s","asset_name":"%s","folder_path":"%s","import_materials":false,"auto_generate_collision":false,"vertex_color_import_option":"Override","vertex_override_color":[0.1,0.3,0.5,0.8]})"),
		*ReimportSourceFbx.Replace(TEXT("\\"), TEXT("/")),
		*AssetName,
		*(FolderPath + TEXT("/")));
	TestTrue(TEXT("Same-path reimport response is valid JSON"), ParseResponse(
		Command.Execute(OverrideReimportPayload),
		SecondResponse));
	TestTrue(TEXT("Same-path reimport succeeds"), SecondResponse.IsValid() && SecondResponse->GetBoolField(TEXT("success")));
	Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	TestNotNull(TEXT("Same-path target still loads"), Mesh);
	if (Mesh)
	{
		TestTrue(TEXT("Reimported mesh saves successfully"), SaveMeshPackage(*Mesh));
		PackagesToReload = {Mesh->GetOutermost()};
		ReloadError = FText::GetEmpty();
		TestTrue(
			TEXT("Reimported mesh reloads from disk"),
			UPackageTools::ReloadPackages(
				PackagesToReload,
				ReloadError,
				EReloadPackagesInteractionMode::AssumePositive));
		Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	}
	const FStaticMeshRenderData* SuccessfulReimportRenderData = Mesh ? Mesh->GetRenderData() : nullptr;
	TestNotNull(TEXT("Successful same-path reimport keeps render data"), SuccessfulReimportRenderData);
	TestTrue(
		TEXT("Successful same-path reimport keeps LOD0 resources"),
		SuccessfulReimportRenderData && !SuccessfulReimportRenderData->LODResources.IsEmpty());
	if (SuccessfulReimportRenderData && !SuccessfulReimportRenderData->LODResources.IsEmpty())
	{
			TestTrue(
				TEXT("Successful same-path reimport replaces geometry through the real handler"),
				static_cast<int32>(SuccessfulReimportRenderData->LODResources[0].GetNumTriangles()) != FirstTriangles
					|| !Mesh->GetBounds().BoxExtent.Equals(FirstBounds, 0.01));
		UFbxStaticMeshImportData* PersistedImportData = Cast<UFbxStaticMeshImportData>(Mesh->GetAssetImportData());
		TestNotNull(TEXT("Same-path reimport persists FBX metadata"), PersistedImportData);
		if (PersistedImportData)
		{
			TestTrue(TEXT("Persisted reimport metadata combines the target"), PersistedImportData->bCombineMeshes);
			TestTrue(TEXT("Persisted reimport metadata converts scene units"), PersistedImportData->bConvertSceneUnit);
			TestFalse(TEXT("Persisted reimport metadata disables generated collision"), PersistedImportData->bAutoGenerateCollision);
				TestEqual(
					TEXT("Persisted reimport metadata keeps Override vertex-color mode"),
					PersistedImportData->VertexColorImportOption.GetValue(),
					EVertexColorImportOption::Override);
				TestEqual(
					TEXT("Persisted reimport metadata keeps the explicit vertex override color"),
					PersistedImportData->VertexOverrideColor,
					PersistedOverrideColor);
				TestTrue(
					TEXT("Persisted reimport provenance points at the replacement FBX"),
					FPaths::IsSamePath(PersistedImportData->GetFirstFilename(), ReimportSourceFbx));
			}
		const TArray<FColor> ReimportedVertexColors = GetLOD0VertexColors(Mesh);
		TestTrue(TEXT("Override reimport produces a real LOD0 vertex-color buffer"), !ReimportedVertexColors.IsEmpty());
		bool bAllVerticesUseOverrideColor = !ReimportedVertexColors.IsEmpty();
		for (const FColor& VertexColor : ReimportedVertexColors)
		{
			bAllVerticesUseOverrideColor &= VertexColor == PersistedOverrideColor;
		}
		TestTrue(TEXT("Override reimport writes the explicit color into every LOD0 vertex"), bAllVerticesUseOverrideColor);
		}

	UFbxStaticMeshImportData* SceneOwnedImportData = Mesh
		? Cast<UFbxStaticMeshImportData>(Mesh->GetAssetImportData())
		: nullptr;
	TestNotNull(TEXT("Scene-owned rejection fixture has FBX metadata"), SceneOwnedImportData);
	if (SceneOwnedImportData)
	{
		SceneOwnedImportData->bImportAsScene = true;
		Mesh->MarkPackageDirty();
		TestTrue(TEXT("Scene-owned rejection fixture saves"), SaveMeshPackage(*Mesh));
		const int32 SceneTriangles = Mesh->GetRenderData() && !Mesh->GetRenderData()->LODResources.IsEmpty()
			? static_cast<int32>(Mesh->GetRenderData()->LODResources[0].GetNumTriangles())
			: INDEX_NONE;
		const FVector SceneBounds = Mesh->GetBounds().BoxExtent;
		const FString SceneProvenance = SceneOwnedImportData->GetFirstFilename();
		const bool bSceneCombineMeshes = SceneOwnedImportData->bCombineMeshes;
		const bool bSceneConvertUnit = SceneOwnedImportData->bConvertSceneUnit;
		const bool bSceneCollision = SceneOwnedImportData->bAutoGenerateCollision;
		const EVertexColorImportOption::Type SceneVertexColor = SceneOwnedImportData->VertexColorImportOption.GetValue();
		const FColor SceneVertexOverrideColor = SceneOwnedImportData->VertexOverrideColor;
		TSharedPtr<FJsonObject> SceneOwnedResponse;
		TestTrue(TEXT("Scene-owned response is valid JSON"), ParseResponse(
			Command.Execute(MakePayload(SourceFbx, FolderPath, true, TEXT("Ignore"))),
			SceneOwnedResponse));
		TestFalse(
			TEXT("Standalone command rejects an FBX Scene-owned target"),
			SceneOwnedResponse.IsValid() && SceneOwnedResponse->GetBoolField(TEXT("success")));
		TestTrue(
			TEXT("Scene-owned rejection comes from the explicit ownership guard"),
			SceneOwnedResponse.IsValid()
				&& SceneOwnedResponse->GetStringField(TEXT("error")).Contains(TEXT("FBX Scene-owned")));
		TestTrue(TEXT("Rejected scene-owned mesh keeps its ownership flag"), SceneOwnedImportData->bImportAsScene);
		TestEqual(TEXT("Rejected scene-owned mesh keeps combine metadata"), SceneOwnedImportData->bCombineMeshes, bSceneCombineMeshes);
		TestEqual(TEXT("Rejected scene-owned mesh keeps scene-unit metadata"), SceneOwnedImportData->bConvertSceneUnit, bSceneConvertUnit);
		TestEqual(TEXT("Rejected scene-owned mesh keeps collision metadata"), SceneOwnedImportData->bAutoGenerateCollision, bSceneCollision);
		TestEqual(
			TEXT("Rejected scene-owned mesh keeps vertex-color metadata"),
			SceneOwnedImportData->VertexColorImportOption.GetValue(),
			SceneVertexColor);
		TestEqual(
			TEXT("Rejected scene-owned mesh keeps vertex override color"),
			SceneOwnedImportData->VertexOverrideColor,
			SceneVertexOverrideColor);
		TestEqual(TEXT("Rejected scene-owned mesh keeps provenance"), SceneOwnedImportData->GetFirstFilename(), SceneProvenance);
		TestEqual(
			TEXT("Rejected scene-owned mesh keeps triangle count"),
			Mesh->GetRenderData() && !Mesh->GetRenderData()->LODResources.IsEmpty()
				? static_cast<int32>(Mesh->GetRenderData()->LODResources[0].GetNumTriangles())
				: INDEX_NONE,
			SceneTriangles);
		TestTrue(TEXT("Rejected scene-owned mesh keeps bounds"), Mesh->GetBounds().BoxExtent.Equals(SceneBounds, 0.01));
		SceneOwnedImportData->bImportAsScene = false;
		Mesh->MarkPackageDirty();
		TestTrue(TEXT("Standalone failure fixture clears scene ownership and saves"), SaveMeshPackage(*Mesh));
	}

	const int32 TrianglesBeforeFailure = Mesh && Mesh->GetRenderData() && !Mesh->GetRenderData()->LODResources.IsEmpty()
		? static_cast<int32>(Mesh->GetRenderData()->LODResources[0].GetNumTriangles())
		: INDEX_NONE;
	const FVector BoundsBeforeFailure = Mesh ? Mesh->GetBounds().BoxExtent : FVector::ZeroVector;
	const TArray<FColor> VertexColorsBeforeFailure = GetLOD0VertexColors(Mesh);
	TArray<FName> MaterialSlotsBeforeFailure;
	TArray<TObjectPtr<UMaterialInterface>> MaterialsBeforeFailure;
	if (Mesh)
	{
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			MaterialSlotsBeforeFailure.Add(Material.ImportedMaterialSlotName);
			MaterialsBeforeFailure.Add(Material.MaterialInterface);
		}
	}
	UStaticMesh* MeshBeforeFailure = Mesh;
	UAssetImportData* ImportDataBeforeFailure = Mesh ? Mesh->GetAssetImportData() : nullptr;
	const UClass* ImportDataClassBeforeFailure = ImportDataBeforeFailure
		? ImportDataBeforeFailure->GetClass()
		: nullptr;
	UFbxStaticMeshImportData* MutableFbxDataBeforeFailure = Cast<UFbxStaticMeshImportData>(ImportDataBeforeFailure);
	TestNotNull(TEXT("Failure rollback fixture has mutable FBX metadata"), MutableFbxDataBeforeFailure);
	if (MutableFbxDataBeforeFailure)
	{
		MutableFbxDataBeforeFailure->ImportTranslation = FVector(11.0, 22.0, 33.0);
		MutableFbxDataBeforeFailure->ImportRotation = FRotator(7.0, 13.0, 19.0);
		MutableFbxDataBeforeFailure->ImportUniformScale = 1.75f;
		MutableFbxDataBeforeFailure->bGenerateLightmapUVs = false;
		MutableFbxDataBeforeFailure->bRemoveDegenerates = false;
		MutableFbxDataBeforeFailure->VertexColorImportOption = EVertexColorImportOption::Override;
		MutableFbxDataBeforeFailure->VertexOverrideColor = FColor(17, 34, 51, 68);
	}
	const UFbxStaticMeshImportData* FbxDataBeforeFailure = MutableFbxDataBeforeFailure;
	const bool bCombineMeshesBeforeFailure = FbxDataBeforeFailure && FbxDataBeforeFailure->bCombineMeshes;
	const bool bConvertSceneUnitBeforeFailure = FbxDataBeforeFailure && FbxDataBeforeFailure->bConvertSceneUnit;
	const bool bCollisionBeforeFailure = FbxDataBeforeFailure && FbxDataBeforeFailure->bAutoGenerateCollision;
	const FVector TranslationBeforeFailure = FbxDataBeforeFailure
		? FbxDataBeforeFailure->ImportTranslation
		: FVector::ZeroVector;
	const FRotator RotationBeforeFailure = FbxDataBeforeFailure
		? FbxDataBeforeFailure->ImportRotation
		: FRotator::ZeroRotator;
	const float UniformScaleBeforeFailure = FbxDataBeforeFailure ? FbxDataBeforeFailure->ImportUniformScale : 0.0f;
	const bool bGenerateLightmapUVsBeforeFailure = FbxDataBeforeFailure && FbxDataBeforeFailure->bGenerateLightmapUVs;
	const bool bRemoveDegeneratesBeforeFailure = FbxDataBeforeFailure && FbxDataBeforeFailure->bRemoveDegenerates;
	const EVertexColorImportOption::Type VertexColorBeforeFailure = FbxDataBeforeFailure
		? FbxDataBeforeFailure->VertexColorImportOption.GetValue()
		: EVertexColorImportOption::Ignore;
	const FColor VertexOverrideColorBeforeFailure = FbxDataBeforeFailure
		? FbxDataBeforeFailure->VertexOverrideColor
		: FColor::Black;
	const FString ProvenanceBeforeFailure = ImportDataBeforeFailure
		? ImportDataBeforeFailure->GetFirstFilename()
		: FString();
	TSharedPtr<FJsonObject> FailureResponse;
	AddExpectedMessagePlain(
		TEXT("No FBX mesh found when reimport Unreal mesh"),
		ELogVerbosity::Error,
		EAutomationExpectedMessageFlags::Contains,
		1);
	AddExpectedMessagePlain(
		TEXT("Fail to reimport mesh"),
		ELogVerbosity::Error,
		EAutomationExpectedMessageFlags::Contains,
		1);
	TestTrue(TEXT("Failed reimport response is valid JSON"), ParseResponse(
		Command.Execute(MakePayload(FailureSourceFbx, FolderPath, true, TEXT("Ignore"))),
		FailureResponse));
	TestFalse(TEXT("Handler failure on same-path reimport reports failure"), FailureResponse.IsValid() && FailureResponse->GetBoolField(TEXT("success")));
	Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	TestTrue(TEXT("Atomic handler failure restores the mesh as a replacement UObject"), Mesh && Mesh != MeshBeforeFailure);
	TestTrue(
		TEXT("Failed reimport restores prior source provenance"),
		Mesh && Mesh->GetAssetImportData() && Mesh->GetAssetImportData()->GetFirstFilename() == ProvenanceBeforeFailure);
	TestTrue(
		TEXT("Failed reimport restores the prior import-data class"),
		Mesh && Mesh->GetAssetImportData() && Mesh->GetAssetImportData()->GetClass() == ImportDataClassBeforeFailure);
	const UFbxStaticMeshImportData* RestoredFbxData = Mesh
		? Cast<UFbxStaticMeshImportData>(Mesh->GetAssetImportData())
		: nullptr;
	TestNotNull(TEXT("Failed reimport restores FBX static-mesh metadata"), RestoredFbxData);
	if (RestoredFbxData)
	{
		TestEqual(TEXT("Failed reimport restores combine-meshes metadata"), RestoredFbxData->bCombineMeshes, bCombineMeshesBeforeFailure);
		TestEqual(TEXT("Failed reimport restores scene-unit metadata"), RestoredFbxData->bConvertSceneUnit, bConvertSceneUnitBeforeFailure);
		TestEqual(TEXT("Failed reimport restores collision metadata"), RestoredFbxData->bAutoGenerateCollision, bCollisionBeforeFailure);
		TestTrue(
			TEXT("Failed reimport restores import translation"),
			RestoredFbxData->ImportTranslation.Equals(TranslationBeforeFailure));
		TestTrue(
			TEXT("Failed reimport restores import rotation"),
			RestoredFbxData->ImportRotation.Equals(RotationBeforeFailure));
		TestEqual(TEXT("Failed reimport restores uniform import scale"), RestoredFbxData->ImportUniformScale, UniformScaleBeforeFailure);
		TestEqual(
			TEXT("Failed reimport restores lightmap-UV generation metadata"),
			static_cast<bool>(RestoredFbxData->bGenerateLightmapUVs),
			bGenerateLightmapUVsBeforeFailure);
		TestEqual(
			TEXT("Failed reimport restores degenerate-removal metadata"),
			static_cast<bool>(RestoredFbxData->bRemoveDegenerates),
			bRemoveDegeneratesBeforeFailure);
		TestEqual(
			TEXT("Failed reimport restores vertex-color metadata"),
			RestoredFbxData->VertexColorImportOption.GetValue(),
			VertexColorBeforeFailure);
		TestEqual(
			TEXT("Failed reimport restores vertex override color"),
			RestoredFbxData->VertexOverrideColor,
			VertexOverrideColorBeforeFailure);
	}
	const FStaticMeshRenderData* RestoredRenderData = Mesh ? Mesh->GetRenderData() : nullptr;
	TestNotNull(TEXT("Failed reimport restores render data"), RestoredRenderData);
	TestTrue(
		TEXT("Failed reimport restores LOD0 resources"),
		RestoredRenderData && !RestoredRenderData->LODResources.IsEmpty());
	if (RestoredRenderData && !RestoredRenderData->LODResources.IsEmpty())
	{
		TestEqual(
			TEXT("Failed reimport restores prior triangle count"),
			static_cast<int32>(RestoredRenderData->LODResources[0].GetNumTriangles()),
			TrianglesBeforeFailure);
		TestTrue(TEXT("Failed reimport restores prior bounds"), Mesh->GetBounds().BoxExtent.Equals(BoundsBeforeFailure, 0.01));
		TArray<FName> RestoredMaterialSlots;
		TArray<TObjectPtr<UMaterialInterface>> RestoredMaterials;
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			RestoredMaterialSlots.Add(Material.ImportedMaterialSlotName);
			RestoredMaterials.Add(Material.MaterialInterface);
		}
		TestEqual(TEXT("Failed reimport restores prior material slots"), RestoredMaterialSlots, MaterialSlotsBeforeFailure);
		TestEqual(TEXT("Failed reimport restores prior material interfaces"), RestoredMaterials, MaterialsBeforeFailure);
		TestEqual(TEXT("Failed reimport restores actual LOD0 vertex colors"), GetLOD0VertexColors(Mesh), VertexColorsBeforeFailure);
	}

	CleanupAutomationAsset(*this, PackageName, PackageFilename, Mesh);
	TestTrue(TEXT("Empty FBX failure fixture is cleaned up"), IFileManager::Get().Delete(*FailureSourceFbx, false, true, true));
	return true;
}

#endif
