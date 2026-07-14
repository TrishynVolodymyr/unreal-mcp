#pragma once

#include "CoreMinimal.h"

class UNREALMCP_API IEditorStatCaptureBackend
{
public:
	virtual ~IEditorStatCaptureBackend() = default;
	virtual bool IsCaptureEnabled() const = 0;
	virtual bool IsStatsRenderingEnabled() const = 0;
	virtual void SetCaptureEnabled(bool bEnabled, bool bRenderStats) = 0;
};

class UNREALMCP_API IEditorStatCaptureScheduler
{
public:
	virtual ~IEditorStatCaptureScheduler() = default;
	virtual uint64 Schedule(float TimeoutSeconds, TFunction<void()> Callback) = 0;
	virtual void Cancel(uint64 Handle) = 0;
};

class UNREALMCP_API FBoundedEditorStatCapture
{
public:
	FBoundedEditorStatCapture(
		TSharedRef<IEditorStatCaptureBackend> InBackend,
		TSharedRef<IEditorStatCaptureScheduler> InScheduler);
	~FBoundedEditorStatCapture();

	bool Begin(float TimeoutSeconds, FString& OutOwner, FString& OutError);
	void Finish();

private:
	void Cleanup(bool bCancelTimeout);
	void HandleTimeout();

	TSharedRef<IEditorStatCaptureBackend> Backend;
	TSharedRef<IEditorStatCaptureScheduler> Scheduler;
	uint64 TimeoutHandle = 0;
	bool bOwnsCapture = false;
	bool bRestoreStatsRendering = true;
};

UNREALMCP_API TSharedRef<IEditorStatCaptureBackend> CreateEditorStatCaptureBackend(
	FName StatGroup,
	bool bMacroGroup = false);
UNREALMCP_API TSharedRef<IEditorStatCaptureScheduler> CreateEditorStatCaptureScheduler();

#if WITH_DEV_AUTOMATION_TESTS
struct FEditorStatCaptureViewportCandidateForTest
{
	bool bIsLevelEditorClient = false;
	bool bHasViewport = false;
	bool bIsActive = false;
};

// Returns -1 for no target, 0/1/2 for PIE/last/current, or 3 + candidate index.
UNREALMCP_API int32 ResolveEditorStatCaptureViewportSourceForTest(
	bool bHasPlayViewport,
	bool bHasLastKeyViewport,
	bool bHasCurrentViewport,
	const TArray<FEditorStatCaptureViewportCandidateForTest>& Candidates);

UNREALMCP_API TSharedRef<IEditorStatCaptureBackend> CreateEditorStatCaptureBackendForTest(
	FName StatGroup,
	TFunction<bool()> IsEnabled,
	TFunction<void(bool)> ExecuteToggle);
#endif
