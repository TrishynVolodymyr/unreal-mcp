#if WITH_DEV_AUTOMATION_TESTS

#include "Commands/Editor/GetGPUStatsCommand.h"

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Services/IGPUStatsCaptureBackend.h"

namespace
{
class FFakeGPUStatsBackend final : public IGPUStatsCaptureBackend
{
public:
	bool bDetailedEnabled = false;
	bool bStatsRenderingEnabled = true;
	bool bSnapshotReady = false;
	int32 EnableCalls = 0;
	int32 DisableCalls = 0;

	virtual double GetTotalGPUMilliseconds() const override { return 3.25; }
	virtual bool IsCaptureEnabled() const override { return bDetailedEnabled; }
	virtual bool IsStatsRenderingEnabled() const override { return bStatsRenderingEnabled; }

	virtual void SetCaptureEnabled(bool bEnabled, bool bRenderStats) override
	{
		bDetailedEnabled = bEnabled;
		bStatsRenderingEnabled = bRenderStats;
		if (bEnabled)
		{
			++EnableCalls;
		}
		else
		{
			++DisableCalls;
		}
	}

	virtual bool TryReadDetailedSnapshot(FGPUStatsSnapshot& OutSnapshot) const override
	{
		if (!bSnapshotReady)
		{
			return false;
		}

		FGPUStatsPassSample Pass;
		Pass.Name = TEXT("BasePass");
		Pass.BusyAverageMilliseconds = 1.5;
		Pass.BusyMaximumMilliseconds = 1.8;
		Pass.BusyMinimumMilliseconds = 1.2;
		OutSnapshot.Passes = {Pass};
		OutSnapshot.TotalBusyMilliseconds = 1.5;
		return true;
	}
};

class FFakeGPUStatsScheduler final : public IEditorStatCaptureScheduler
{
public:
	int32 ScheduleCalls = 0;
	int32 CancelCalls = 0;
	uint64 NextHandle = 1;
	uint64 LastScheduledHandle = 0;
	TMap<uint64, TFunction<void()>> PendingCallbacks;

	virtual uint64 Schedule(float TimeoutSeconds, TFunction<void()> Callback) override
	{
		++ScheduleCalls;
		LastTimeoutSeconds = TimeoutSeconds;
		LastScheduledHandle = NextHandle++;
		PendingCallbacks.Add(LastScheduledHandle, MoveTemp(Callback));
		return LastScheduledHandle;
	}

	virtual void Cancel(uint64 Handle) override
	{
		if (Handle != 0)
		{
			++CancelCalls;
		}
		PendingCallbacks.Remove(Handle);
	}

	bool Fire(uint64 Handle)
	{
		TFunction<void()>* PendingCallback = PendingCallbacks.Find(Handle);
		if (!PendingCallback)
		{
			return false;
		}
		TFunction<void()> Callback = MoveTemp(*PendingCallback);
		PendingCallbacks.Remove(Handle);
		Callback();
		return true;
	}

	bool Fire()
	{
		return Fire(LastScheduledHandle);
	}

	float LastTimeoutSeconds = 0.0f;
};

TSharedPtr<FJsonObject> ParseGPUStatsResponse(const FString& Json)
{
	TSharedPtr<FJsonObject> Result;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	FJsonSerializer::Deserialize(Reader, Result);
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPUStatsSnapshotDoesNotMutateTest,
	"UnrealMCP.Editor.GPUStats.SnapshotDoesNotMutateProfiler",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPUStatsSnapshotDoesNotMutateTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FFakeGPUStatsBackend> Backend = MakeShared<FFakeGPUStatsBackend>();
	const TSharedRef<FFakeGPUStatsScheduler> Scheduler = MakeShared<FFakeGPUStatsScheduler>();
	FGetGPUStatsCommand Command(Backend, Scheduler);

	const TSharedPtr<FJsonObject> Result = ParseGPUStatsResponse(Command.Execute(TEXT("{}")));
	TestTrue(TEXT("Snapshot response parses"), Result.IsValid());
	TestTrue(TEXT("Snapshot succeeds"), Result && Result->GetBoolField(TEXT("success")));
	TestFalse(TEXT("Detailed snapshot is unavailable"), Result && Result->GetBoolField(TEXT("detailed_available")));
	TestEqual(TEXT("Total GPU time remains available"), Result ? Result->GetNumberField(TEXT("total_gpu_ms")) : 0.0, 3.25);
	TestEqual(TEXT("Snapshot never enables detailed profiling"), Backend->EnableCalls, 0);
	TestEqual(TEXT("Snapshot never schedules cleanup"), Scheduler->ScheduleCalls, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPUStatsOwnedCaptureCleansEveryExitTest,
	"UnrealMCP.Editor.GPUStats.OwnedCaptureCleansEveryExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPUStatsOwnedCaptureCleansEveryExitTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FFakeGPUStatsBackend> Backend = MakeShared<FFakeGPUStatsBackend>();
	const TSharedRef<FFakeGPUStatsScheduler> Scheduler = MakeShared<FFakeGPUStatsScheduler>();
	FGetGPUStatsCommand Command(Backend, Scheduler);

	TSharedPtr<FJsonObject> Result = ParseGPUStatsResponse(Command.Execute(TEXT(R"({"action":"begin","timeout_seconds":2.0})")));
	TestTrue(TEXT("Begin succeeds"), Result && Result->GetBoolField(TEXT("success")));
	TestEqual(TEXT("Begin reports MCP ownership"), Result ? Result->GetStringField(TEXT("capture_owner")) : FString(), FString(TEXT("mcp")));
	TestEqual(TEXT("Owned begin enables profiler once"), Backend->EnableCalls, 1);
	TestEqual(TEXT("Owned begin arms watchdog"), Scheduler->ScheduleCalls, 1);
	TestEqual(TEXT("Requested timeout reaches scheduler"), Scheduler->LastTimeoutSeconds, 2.0f);

	Backend->bSnapshotReady = true;
	Result = ParseGPUStatsResponse(Command.Execute(TEXT(R"({"action":"read","finish":true})")));
	TestTrue(TEXT("Read succeeds"), Result && Result->GetBoolField(TEXT("success")));
	TestTrue(TEXT("Read returns detailed data"), Result && Result->GetBoolField(TEXT("detailed_available")));
	TestEqual(TEXT("Read returns one pass"), Result ? static_cast<int32>(Result->GetNumberField(TEXT("pass_count"))) : 0, 1);
	TestTrue(TEXT("Read summary names the expensive pass"), Result && Result->GetStringField(TEXT("message")).Contains(TEXT("BasePass")));
	TestEqual(TEXT("Finished read disables owned profiler"), Backend->DisableCalls, 1);
	TestEqual(TEXT("Finished read cancels watchdog"), Scheduler->CancelCalls, 1);

	Backend->bSnapshotReady = false;
	Command.Execute(TEXT(R"({"action":"begin"})"));
	Scheduler->Fire();
	TestEqual(TEXT("Watchdog disables abandoned capture"), Backend->DisableCalls, 2);

	Command.Execute(TEXT(R"({"action":"begin"})"));
	Command.Execute(TEXT(R"({"action":"cancel"})"));
	Command.Execute(TEXT(R"({"action":"cancel"})"));
	TestEqual(TEXT("Cancel cleanup is idempotent"), Backend->DisableCalls, 3);

	Command.Execute(TEXT(R"({"action":"begin"})"));
	Backend->bDetailedEnabled = false;
	Command.Execute(TEXT(R"({"action":"cancel"})"));
	TestEqual(
		TEXT("Cleanup releases ownership even when the user already disabled profiling"),
		Backend->DisableCalls,
		4);
	TestFalse(TEXT("Cleanup never re-enables a user-disabled profiler"), Backend->bDetailedEnabled);

	{
		FGetGPUStatsCommand DestructedCommand(Backend, Scheduler);
		DestructedCommand.Execute(TEXT(R"({"action":"begin"})"));
	}
	TestEqual(TEXT("Command destruction disables owned profiler"), Backend->DisableCalls, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPUStatsViewportResolutionPriorityTest,
	"UnrealMCP.Editor.GPUStats.ViewportResolutionPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPUStatsViewportResolutionPriorityTest::RunTest(const FString& Parameters)
{
	TArray<FEditorStatCaptureViewportCandidateForTest> Candidates = {
		{true, true, false},
		{false, true, true},
		{true, true, true},
		{true, true, false}};
	TestEqual(
		TEXT("PIE viewport has first priority"),
		ResolveEditorStatCaptureViewportSourceForTest(true, true, true, Candidates),
		0);
	TestEqual(
		TEXT("last-key editor viewport has second priority"),
		ResolveEditorStatCaptureViewportSourceForTest(false, true, true, Candidates),
		1);
	TestEqual(
		TEXT("current editor viewport has third priority"),
		ResolveEditorStatCaptureViewportSourceForTest(false, false, true, Candidates),
		2);
	TestEqual(
		TEXT("active live level viewport is the first fallback"),
		ResolveEditorStatCaptureViewportSourceForTest(false, false, false, Candidates),
		5);

	Candidates[2].bIsActive = false;
	TestEqual(
		TEXT("first live level viewport is the final fallback"),
		ResolveEditorStatCaptureViewportSourceForTest(false, false, false, Candidates),
		3);
	Candidates[0].bHasViewport = false;
	Candidates[2].bHasViewport = false;
	Candidates[3].bHasViewport = false;
	TestEqual(
		TEXT("non-level and viewport-less candidates are rejected"),
		ResolveEditorStatCaptureViewportSourceForTest(false, false, false, Candidates),
		INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPUStatsConcreteBackendUserDisableTest,
	"UnrealMCP.Editor.GPUStats.ConcreteBackendDoesNotRetoggleUserDisabledStat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPUStatsConcreteBackendUserDisableTest::RunTest(const FString& Parameters)
{
	bool bStatEnabled = false;
	int32 ToggleCalls = 0;
	const TSharedRef<IEditorStatCaptureBackend> Backend = CreateEditorStatCaptureBackendForTest(
		TEXT("GPU"),
		[&bStatEnabled]() { return bStatEnabled; },
		[&bStatEnabled, &ToggleCalls](bool)
		{
			++ToggleCalls;
			bStatEnabled = !bStatEnabled;
		});
	const TSharedRef<FFakeGPUStatsScheduler> Scheduler = MakeShared<FFakeGPUStatsScheduler>();
	FBoundedEditorStatCapture Capture(Backend, Scheduler);
	FString Owner;
	FString Error;
	TestTrue(TEXT("concrete backend begins owned capture"), Capture.Begin(2.0f, Owner, Error));
	TestEqual(TEXT("enable executes one toggle"), ToggleCalls, 1);
	TestTrue(TEXT("stat is enabled after begin"), bStatEnabled);

	// Simulate the user issuing `stat GPU` while MCP still owns the window.
	bStatEnabled = false;
	Capture.Finish();
	TestEqual(TEXT("cleanup does not execute a second toggle"), ToggleCalls, 1);
	TestFalse(TEXT("user-disabled stat stays disabled"), bStatEnabled);

	TestTrue(TEXT("concrete backend can begin a second owned capture"), Capture.Begin(2.0f, Owner, Error));
	TestEqual(TEXT("second begin executes an enable toggle"), ToggleCalls, 2);
	TestTrue(TEXT("second capture is enabled"), bStatEnabled);
	const uint64 StaleWatchdogHandle = Scheduler->LastScheduledHandle;

	// A repeated begin must repair stale MCP ownership if the user disabled the
	// stat between calls; reporting owner=mcp while the stat is off is a lie.
	bStatEnabled = false;
	Owner.Reset();
	Error.Reset();
	TestTrue(TEXT("repeated begin repairs a user-disabled owned capture"), Capture.Begin(2.0f, Owner, Error));
	TestEqual(TEXT("repeated begin executes a replacement enable toggle"), ToggleCalls, 3);
	TestTrue(TEXT("repeated begin leaves the stat enabled"), bStatEnabled);
	TestEqual(TEXT("repeated begin still reports MCP ownership"), Owner, FString(TEXT("mcp")));
	const uint64 ReplacementWatchdogHandle = Scheduler->LastScheduledHandle;
	TestNotEqual(TEXT("reacquisition installs a distinct watchdog"), ReplacementWatchdogHandle, StaleWatchdogHandle);
	TestFalse(TEXT("cancelled stale watchdog cannot fire"), Scheduler->Fire(StaleWatchdogHandle));
	TestTrue(TEXT("stale watchdog cannot disable the replacement capture"), bStatEnabled);
	TestTrue(TEXT("replacement watchdog remains armed"), Scheduler->Fire(ReplacementWatchdogHandle));
	TestEqual(TEXT("replacement watchdog disables the reacquired capture"), ToggleCalls, 4);
	TestFalse(TEXT("replacement watchdog leaves profiling disabled"), bStatEnabled);
	Capture.Finish();
	TestEqual(TEXT("finish after watchdog cleanup is idempotent"), ToggleCalls, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPUStatsExternalOwnershipAndValidationTest,
	"UnrealMCP.Editor.GPUStats.ExternalOwnershipAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPUStatsExternalOwnershipAndValidationTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FFakeGPUStatsBackend> Backend = MakeShared<FFakeGPUStatsBackend>();
	Backend->bDetailedEnabled = true;
	Backend->bSnapshotReady = true;
	const TSharedRef<FFakeGPUStatsScheduler> Scheduler = MakeShared<FFakeGPUStatsScheduler>();
	FGetGPUStatsCommand Command(Backend, Scheduler);

	TSharedPtr<FJsonObject> Result = ParseGPUStatsResponse(Command.Execute(TEXT(R"({"action":"begin"})")));
	TestEqual(TEXT("Existing profiler is externally owned"), Result ? Result->GetStringField(TEXT("capture_owner")) : FString(), FString(TEXT("external")));
	Command.Execute(TEXT(R"({"action":"read","finish":true})"));
	TestEqual(TEXT("External profiler is never disabled"), Backend->DisableCalls, 0);
	TestEqual(TEXT("External profiler does not need watchdog"), Scheduler->ScheduleCalls, 0);

	Result = ParseGPUStatsResponse(Command.Execute(TEXT(R"({"action":"explode"})")));
	TestFalse(TEXT("Unknown action fails"), Result && Result->GetBoolField(TEXT("success")));
	TestEqual(TEXT("Invalid action does not mutate profiler"), Backend->EnableCalls, 0);
	TestEqual(TEXT("Invalid action does not disable profiler"), Backend->DisableCalls, 0);

	Result = ParseGPUStatsResponse(Command.Execute(TEXT(R"({"action":"begin","timeout_seconds":0.01})")));
	TestFalse(TEXT("Out-of-range timeout fails"), Result && Result->GetBoolField(TEXT("success")));
	TestEqual(TEXT("Invalid timeout does not mutate profiler"), Backend->EnableCalls, 0);
	return true;
}

#endif
