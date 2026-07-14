#if WITH_DEV_AUTOMATION_TESTS

#include "UnrealMCPBridge.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandDispatchPolicyTest,
	"UnrealMCP.Bridge.CommandDispatchPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandDispatchPolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Level loads run from the game-thread ticker so UE 5.8 tasks inherit FAppTime"),
		UUnrealMCPBridge::ShouldDispatchViaTicker(TEXT("open_level")));
	TestTrue(
		TEXT("Forced viewport draws run from the ticker so renderer tasks inherit FAppTime"),
		UUnrealMCPBridge::ShouldDispatchViaTicker(TEXT("capture_viewport_screenshot")));
	TestTrue(
		TEXT("Static mesh import preserves its existing non-TaskGraph dispatch"),
		UUnrealMCPBridge::ShouldDispatchViaTicker(TEXT("import_static_mesh")));
	TestTrue(
		TEXT("LOD import preserves its existing non-TaskGraph dispatch"),
		UUnrealMCPBridge::ShouldDispatchViaTicker(TEXT("import_lod")));
	TestFalse(
		TEXT("Ordinary commands retain the default AsyncTask dispatch"),
		UUnrealMCPBridge::ShouldDispatchViaTicker(TEXT("ping")));
	return true;
}

#endif
