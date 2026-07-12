#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Editor/GetRenderingStatsCommand.h"

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Services/IEditorStatCapture.h"

namespace
{
class FFakeInitViewsBackend final : public IEditorStatCaptureBackend
{
public:
	bool bEnabled = false;
	bool bRenderStats = true;
	int32 EnableCalls = 0;
	int32 DisableCalls = 0;

	virtual bool IsCaptureEnabled() const override { return bEnabled; }
	virtual bool IsStatsRenderingEnabled() const override { return bRenderStats; }
	virtual void SetCaptureEnabled(bool bInEnabled, bool bInRenderStats) override
	{
		bEnabled = bInEnabled;
		bRenderStats = bInRenderStats;
		bInEnabled ? ++EnableCalls : ++DisableCalls;
	}
};

class FFakeInitViewsScheduler final : public IEditorStatCaptureScheduler
{
public:
	TFunction<void()> Pending;
	int32 ScheduleCalls = 0;
	int32 CancelCalls = 0;

	virtual uint64 Schedule(float, TFunction<void()> Callback) override
	{
		++ScheduleCalls;
		Pending = MoveTemp(Callback);
		return 1;
	}
	virtual void Cancel(uint64 Handle) override
	{
		if (Handle != 0)
		{
			++CancelCalls;
		}
		Pending = nullptr;
	}
};

TSharedPtr<FJsonObject> ParseRenderingResponse(const FString& Json)
{
	TSharedPtr<FJsonObject> Result;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	FJsonSerializer::Deserialize(Reader, Result);
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRenderingStatsBoundedInitViewsTest,
	"UnrealMCP.Editor.RenderingStats.BoundedInitViews",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRenderingStatsBoundedInitViewsTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FFakeInitViewsBackend> Backend = MakeShared<FFakeInitViewsBackend>();
	const TSharedRef<FFakeInitViewsScheduler> Scheduler = MakeShared<FFakeInitViewsScheduler>();
	FGetRenderingStatsCommand Command(Backend, Scheduler);

	TSharedPtr<FJsonObject> Result = ParseRenderingResponse(Command.Execute(TEXT("{}")));
	TestTrue(TEXT("Snapshot succeeds"), Result && Result->GetBoolField(TEXT("success")));
	TestEqual(TEXT("Snapshot does not enable InitViews"), Backend->EnableCalls, 0);

	Result = ParseRenderingResponse(Command.Execute(TEXT(R"({"action":"begin","timeout_seconds":2.0})")));
	TestEqual(TEXT("Begin reports MCP ownership"), Result ? Result->GetStringField(TEXT("capture_owner")) : FString(), FString(TEXT("mcp")));
	TestEqual(TEXT("Begin enables InitViews once"), Backend->EnableCalls, 1);
	TestEqual(TEXT("Begin arms watchdog"), Scheduler->ScheduleCalls, 1);

	Command.Execute(TEXT(R"({"action":"read","finish":true})"));
	TestEqual(TEXT("Finished read disables InitViews"), Backend->DisableCalls, 1);
	TestEqual(TEXT("Finished read cancels watchdog"), Scheduler->CancelCalls, 1);

	Backend->bEnabled = true;
	Command.Execute(TEXT(R"({"action":"begin"})"));
	Command.Execute(TEXT(R"({"action":"read","finish":true})"));
	TestEqual(TEXT("Externally owned InitViews remains enabled"), Backend->DisableCalls, 1);
	return true;
}

#endif
