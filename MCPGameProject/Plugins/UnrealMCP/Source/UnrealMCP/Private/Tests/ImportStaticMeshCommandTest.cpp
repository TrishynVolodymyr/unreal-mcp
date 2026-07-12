#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Editor/ImportStaticMeshCommand.h"

#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Misc/AutomationTest.h"

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
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","auto_generate_collision":false,"vertex_color_import_option":"Replace"})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestTrue(TEXT("Explicit payload parses"), Command.ParseParametersForTest(ExplicitJson, Settings, Error));
	TestFalse(TEXT("Explicit collision option disables collision"), Settings.bAutoGenerateCollision);
	TestEqual(
		TEXT("Explicit vertex color option selects Replace"),
		Settings.VertexColorImportOption,
		EVertexColorImportOption::Replace);

	UFbxImportUI* ExplicitUI = NewObject<UFbxImportUI>();
	Command.ConfigureImportUIForTest(*ExplicitUI, Settings);
	TestFalse(TEXT("Configured UI disables collision"), ExplicitUI->StaticMeshImportData->bAutoGenerateCollision);
	TestEqual(
		TEXT("Configured UI imports FBX vertex colors"),
		ExplicitUI->StaticMeshImportData->VertexColorImportOption.GetValue(),
		EVertexColorImportOption::Replace);

	const FString InvalidJson = TEXT(
		R"({"source_file_path":"E:/mesh.fbx","asset_name":"SM_Test","vertex_color_import_option":"Flatten"})");
	Settings = FImportStaticMeshSettings{};
	Error.Reset();
	TestFalse(TEXT("Invalid vertex color option is rejected"), Command.ParseParametersForTest(InvalidJson, Settings, Error));
	TestTrue(TEXT("Invalid option diagnostic lists valid values"), Error.Contains(TEXT("Ignore, Replace, Override")));

	return true;
}

#endif
