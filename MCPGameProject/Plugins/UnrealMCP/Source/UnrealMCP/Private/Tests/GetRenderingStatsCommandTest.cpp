#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Editor/GetRenderingStatsCommand.h"

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
TSharedPtr<FJsonObject> ParseRenderingResponse(const FString& Json)
{
	TSharedPtr<FJsonObject> Result;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	FJsonSerializer::Deserialize(Reader, Result);
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRenderingStatsSnapshotTest,
	"UnrealMCP.Editor.RenderingStats.Snapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRenderingStatsSnapshotTest::RunTest(const FString& Parameters)
{
	FGetRenderingStatsCommand Command;
	TSharedPtr<FJsonObject> Result = ParseRenderingResponse(Command.Execute(TEXT("{}")));
	TestTrue(TEXT("Snapshot succeeds"), Result && Result->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Snapshot includes draw calls"), Result && Result->HasTypedField<EJson::Number>(TEXT("draw_calls")));
	TestTrue(TEXT("Snapshot includes primitives"), Result && Result->HasTypedField<EJson::Number>(TEXT("primitives_drawn")));
	TestTrue(TEXT("Snapshot includes GPU frame time"), Result && Result->HasTypedField<EJson::Number>(TEXT("gpu_time_ms")));
	TestTrue(TEXT("Snapshot includes VRAM object"), Result && Result->HasTypedField<EJson::Object>(TEXT("vram")));

	const TSharedPtr<FJsonObject>* Visibility = nullptr;
	TestTrue(TEXT("Snapshot includes visibility contract"), Result && Result->TryGetObjectField(TEXT("visibility"), Visibility));
	TestFalse(TEXT("Snapshot never claims fresh InitViews detail"), Visibility && (*Visibility)->GetBoolField(TEXT("detailed_available")));

	Result = ParseRenderingResponse(Command.Execute(TEXT(R"({"action":"begin"})")));
	TestFalse(TEXT("Removed mutating capture mode is rejected"), Result && Result->GetBoolField(TEXT("success")));
	return true;
}

#endif
