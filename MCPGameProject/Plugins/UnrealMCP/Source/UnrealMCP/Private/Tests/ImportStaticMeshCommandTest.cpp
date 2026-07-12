#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Editor/ImportStaticMeshCommand.h"

#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FImportStaticMeshOptionsTest,
	"UnrealMCP.Editor.ImportStaticMesh.Options",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

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

	const FString InvalidJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Flatten"})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(TEXT("Invalid vertex color option is rejected"), Command.ParseParametersForTest(InvalidJson, Settings, Error));
	TestTrue(TEXT("Invalid option diagnostic lists valid values"), Error.Contains(TEXT("Ignore, Replace, Override")));

	const FString TempSourcePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealMCPImportOptions.fbx"));
	TestTrue(TEXT("temporary source file is created"), FFileHelper::SaveStringToFile(TEXT("test seam"), *TempSourcePath));
	const FString JsonSourcePath = TempSourcePath.Replace(TEXT("\\"), TEXT("/"));
	const FString ExecuteJson = FString::Printf(
		TEXT(R"({"source_file_path":"%s","asset_name":"SM_Test","folder_path":"/Game/Test","auto_generate_collision":false,"vertex_color_import_option":"Ignore"})"),
		*JsonSourcePath);

	bool bExecutionOverrideCalled = false;
	FImportStaticMeshCommand ExecuteCommand(
		[&bExecutionOverrideCalled, this](const FImportStaticMeshSettings& Received, const UFbxImportUI& ImportUI)
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
			return FString(TEXT("{\"success\":true}"));
		});
	const FString ExecuteResponse = ExecuteCommand.Execute(ExecuteJson);
	IFileManager::Get().Delete(*TempSourcePath);
	TestTrue(TEXT("Execute invokes the import seam"), bExecutionOverrideCalled);
	TestTrue(TEXT("Execute returns seam response"), ExecuteResponse.Contains(TEXT("\"success\":true")));

	return true;
}

#endif
