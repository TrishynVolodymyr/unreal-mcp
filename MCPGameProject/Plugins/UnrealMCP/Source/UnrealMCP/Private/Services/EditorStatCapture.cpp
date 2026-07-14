#include "Services/IEditorStatCapture.h"

#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"

#if STATS
#include "Stats/StatsData.h"
#endif

namespace
{
struct FEditorStatViewportCandidate
{
	FEditorViewportClient* Client = nullptr;
	bool bIsLevelEditorClient = false;
	bool bHasViewport = false;
	bool bIsActive = false;
};

FCommonViewportClient* ResolveEditorStatViewportFromCandidates(
	FCommonViewportClient* PlayViewport,
	FCommonViewportClient* LastKeyViewport,
	FCommonViewportClient* CurrentViewport,
	const TArray<FEditorStatViewportCandidate>& Candidates)
{
	if (PlayViewport)
	{
		return PlayViewport;
	}
	if (LastKeyViewport)
	{
		return LastKeyViewport;
	}
	if (CurrentViewport)
	{
		return CurrentViewport;
	}
	for (const FEditorStatViewportCandidate& Candidate : Candidates)
	{
		if (Candidate.Client && Candidate.bIsLevelEditorClient && Candidate.bHasViewport && Candidate.bIsActive)
		{
			return Candidate.Client;
		}
	}
	for (const FEditorStatViewportCandidate& Candidate : Candidates)
	{
		if (Candidate.Client && Candidate.bIsLevelEditorClient && Candidate.bHasViewport)
		{
			return Candidate.Client;
		}
	}
	return nullptr;
}

class FEditorStatCaptureBackend final : public IEditorStatCaptureBackend
{
public:
	FEditorStatCaptureBackend(FName InStatGroup, bool bInMacroGroup)
		: StatGroup(InStatGroup)
	{
		(void)bInMacroGroup;
	}

	FEditorStatCaptureBackend(
		FName InStatGroup,
		TFunction<bool()> InIsEnabled,
		TFunction<void(bool)> InExecuteToggle)
		: StatGroup(InStatGroup)
		, IsEnabledOverride(MoveTemp(InIsEnabled))
		, ExecuteToggleOverride(MoveTemp(InExecuteToggle))
	{
	}

	virtual bool IsCaptureEnabled() const override
	{
		if (bOwnsToggle)
		{
			return IsOwnedTargetEnabled();
		}
		if (IsEnabledOverride)
		{
			return IsEnabledOverride();
		}

		if (GEditor)
		{
			for (const FEditorViewportClient* ViewportClient : GEditor->GetAllViewportClients())
			{
				if (ViewportClient && ViewportClient->IsStatEnabled(StatGroup.ToString()))
				{
					return true;
				}
			}
		}
		return GEngine && GEngine->GameViewport && GEngine->GameViewport->IsStatEnabled(StatGroup.ToString());
	}

	virtual bool IsStatsRenderingEnabled() const override
	{
#if STATS
		const FGameThreadStatsData* ViewData = FLatestGameThreadStatsData::Get().Latest;
		return !ViewData || ViewData->bRenderStats;
#else
		return false;
#endif
	}

	virtual void SetCaptureEnabled(bool bEnabled, bool bRenderStats) override
	{
		if (!GEngine && !ExecuteToggleOverride)
		{
			return;
		}

		if (bEnabled)
		{
			if (IsCaptureEnabled())
			{
				return;
			}
			if (!ExecuteToggleOverride)
			{
				OwnedViewportClient = ResolveTargetViewportClient();
			}
		}
		else if (!bOwnsToggle || !HasOwnedTarget())
		{
			OwnedViewportClient = nullptr;
			bOwnsToggle = false;
			return;
		}
		else if (!IsCaptureEnabled())
		{
			// The user (or another system) already disabled the stat during our
			// capture window. Release ownership without executing the toggle,
			// which would otherwise re-enable it.
			OwnedViewportClient = nullptr;
			bOwnsToggle = false;
			return;
		}

		if (!HasOwnedTarget())
		{
			return;
		}
		if (ExecuteToggleOverride)
		{
			ExecuteToggleOverride(bRenderStats);
			bOwnsToggle = bEnabled && IsOwnedTargetEnabled();
			return;
		}

		FString Command = FString::Printf(TEXT("stat %s"), *StatGroup.ToString());
		if (!bRenderStats)
		{
			Command += TEXT(" -nodisplay");
		}

		FCommonViewportClient* PreviousStatViewport = GStatProcessingViewportClient;
		FEditorViewportClient* EditorViewportClient = !bEnabled
			? FindEditorViewportClient(OwnedViewportClient)
			: nullptr;
		const FText CleanupRealtimeOverride = FText::FromString(TEXT("UnrealMCP Stat Cleanup"));
		if (EditorViewportClient)
		{
			EditorViewportClient->AddRealtimeOverride(true, CleanupRealtimeOverride);
		}
		GStatProcessingViewportClient = OwnedViewportClient;
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		GEngine->Exec(World, *Command);
		GStatProcessingViewportClient = PreviousStatViewport;
		if (EditorViewportClient)
		{
			EditorViewportClient->RemoveRealtimeOverride(CleanupRealtimeOverride);
		}

		bOwnsToggle = bEnabled && IsOwnedTargetEnabled();
		if (!bOwnsToggle)
		{
			OwnedViewportClient = nullptr;
		}
	}

private:
	bool HasOwnedTarget() const
	{
		return ExecuteToggleOverride
			|| (OwnedViewportClient && IsViewportClientAlive(OwnedViewportClient));
	}

	bool IsOwnedTargetEnabled() const
	{
		if (IsEnabledOverride)
		{
			return IsEnabledOverride();
		}
		return OwnedViewportClient
			&& IsViewportClientAlive(OwnedViewportClient)
			&& OwnedViewportClient->IsStatEnabled(StatGroup.ToString());
	}

	FCommonViewportClient* ResolveTargetViewportClient() const
	{
		FCommonViewportClient* PlayViewport = GEngine && GEngine->GameViewport && GEditor && GEditor->PlayWorld
			? static_cast<FCommonViewportClient*>(GEngine->GameViewport)
			: nullptr;
		TArray<FEditorStatViewportCandidate> Candidates;
		if (GEditor)
		{
			const FViewport* ActiveViewport = GEditor->GetActiveViewport();
			for (FEditorViewportClient* ViewportClient : GEditor->GetAllViewportClients())
			{
				Candidates.Add({
					ViewportClient,
					ViewportClient && ViewportClient->IsLevelEditorClient(),
					ViewportClient && ViewportClient->Viewport != nullptr,
					ViewportClient && ViewportClient->Viewport == ActiveViewport});
			}
		}
		return ResolveEditorStatViewportFromCandidates(
			PlayViewport,
			GLastKeyLevelEditingViewportClient,
			GCurrentLevelEditingViewportClient,
			Candidates);
	}

	bool IsViewportClientAlive(const FCommonViewportClient* ViewportClient) const
	{
		if (!ViewportClient)
		{
			return false;
		}
		if (GEngine && GEngine->GameViewport == ViewportClient)
		{
			return true;
		}
		if (GEditor)
		{
			for (const FEditorViewportClient* EditorViewportClient : GEditor->GetAllViewportClients())
			{
				if (EditorViewportClient == ViewportClient)
				{
					return true;
				}
			}
		}
		return false;
	}

	FEditorViewportClient* FindEditorViewportClient(const FCommonViewportClient* ViewportClient) const
	{
		if (GEditor)
		{
			for (FEditorViewportClient* EditorViewportClient : GEditor->GetAllViewportClients())
			{
				if (EditorViewportClient == ViewportClient)
				{
					return EditorViewportClient;
				}
			}
		}
		return nullptr;
	}

	FName StatGroup;
	bool bOwnsToggle = false;
	FCommonViewportClient* OwnedViewportClient = nullptr;
	TFunction<bool()> IsEnabledOverride;
	TFunction<void(bool)> ExecuteToggleOverride;
};

class FEditorStatCaptureScheduler final : public IEditorStatCaptureScheduler
{
public:
	virtual ~FEditorStatCaptureScheduler() override
	{
		for (const TPair<uint64, FTSTicker::FDelegateHandle>& Pair : Handles)
		{
			FTSTicker::GetCoreTicker().RemoveTicker(Pair.Value);
		}
	}

	virtual uint64 Schedule(float TimeoutSeconds, TFunction<void()> Callback) override
	{
		const uint64 Id = NextId++;
		const FTSTicker::FDelegateHandle Handle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([this, Id, Callback = MoveTemp(Callback)](float) mutable
			{
				Handles.Remove(Id);
				Callback();
				return false;
			}),
			TimeoutSeconds);
		Handles.Add(Id, Handle);
		return Id;
	}

	virtual void Cancel(uint64 Handle) override
	{
		if (const FTSTicker::FDelegateHandle* TickerHandle = Handles.Find(Handle))
		{
			FTSTicker::GetCoreTicker().RemoveTicker(*TickerHandle);
			Handles.Remove(Handle);
		}
	}

private:
	uint64 NextId = 1;
	TMap<uint64, FTSTicker::FDelegateHandle> Handles;
};
}

#if WITH_DEV_AUTOMATION_TESTS
int32 ResolveEditorStatCaptureViewportSourceForTest(
	bool bHasPlayViewport,
	bool bHasLastKeyViewport,
	bool bHasCurrentViewport,
	const TArray<FEditorStatCaptureViewportCandidateForTest>& Candidates)
{
	FCommonViewportClient* PlayViewport = bHasPlayViewport
		? reinterpret_cast<FCommonViewportClient*>(static_cast<UPTRINT>(1))
		: nullptr;
	FCommonViewportClient* LastKeyViewport = bHasLastKeyViewport
		? reinterpret_cast<FCommonViewportClient*>(static_cast<UPTRINT>(2))
		: nullptr;
	FCommonViewportClient* CurrentViewport = bHasCurrentViewport
		? reinterpret_cast<FCommonViewportClient*>(static_cast<UPTRINT>(3))
		: nullptr;
	TArray<FEditorStatViewportCandidate> InternalCandidates;
	InternalCandidates.Reserve(Candidates.Num());
	for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		const FEditorStatCaptureViewportCandidateForTest& Candidate = Candidates[CandidateIndex];
		InternalCandidates.Add({
			reinterpret_cast<FEditorViewportClient*>(static_cast<UPTRINT>(4 + CandidateIndex)),
			Candidate.bIsLevelEditorClient,
			Candidate.bHasViewport,
			Candidate.bIsActive});
	}

	FCommonViewportClient* Resolved = ResolveEditorStatViewportFromCandidates(
		PlayViewport,
		LastKeyViewport,
		CurrentViewport,
		InternalCandidates);
	if (!Resolved)
	{
		return INDEX_NONE;
	}
	if (Resolved == PlayViewport)
	{
		return 0;
	}
	if (Resolved == LastKeyViewport)
	{
		return 1;
	}
	if (Resolved == CurrentViewport)
	{
		return 2;
	}
	for (int32 CandidateIndex = 0; CandidateIndex < InternalCandidates.Num(); ++CandidateIndex)
	{
		if (Resolved == InternalCandidates[CandidateIndex].Client)
		{
			return 3 + CandidateIndex;
		}
	}
	return INDEX_NONE;
}
#endif

FBoundedEditorStatCapture::FBoundedEditorStatCapture(
	TSharedRef<IEditorStatCaptureBackend> InBackend,
	TSharedRef<IEditorStatCaptureScheduler> InScheduler)
	: Backend(MoveTemp(InBackend))
	, Scheduler(MoveTemp(InScheduler))
{
}

FBoundedEditorStatCapture::~FBoundedEditorStatCapture()
{
	Cleanup(true);
}

bool FBoundedEditorStatCapture::Begin(float TimeoutSeconds, FString& OutOwner, FString& OutError)
{
	if (TimeoutSeconds < 0.1f || TimeoutSeconds > 30.0f)
	{
		OutError = TEXT("timeout_seconds must be between 0.1 and 30.0");
		return false;
	}

	if (bOwnsCapture)
	{
		if (Backend->IsCaptureEnabled())
		{
			if (TimeoutHandle != 0)
			{
				Scheduler->Cancel(TimeoutHandle);
			}
			TimeoutHandle = Scheduler->Schedule(TimeoutSeconds, [this]() { HandleTimeout(); });
			OutOwner = TEXT("mcp");
			return true;
		}

		// The user disabled the stat while MCP still owned the bounded window.
		// Release stale backend ownership without toggling, then reacquire below.
		if (TimeoutHandle != 0)
		{
			Scheduler->Cancel(TimeoutHandle);
		}
		TimeoutHandle = 0;
		Backend->SetCaptureEnabled(false, bRestoreStatsRendering);
		bOwnsCapture = false;
	}

	if (Backend->IsCaptureEnabled())
	{
		OutOwner = TEXT("external");
		return true;
	}

	bRestoreStatsRendering = Backend->IsStatsRenderingEnabled();
	Backend->SetCaptureEnabled(true, false);
	if (!Backend->IsCaptureEnabled())
	{
		OutError = TEXT("Failed to enable editor stat capture");
		return false;
	}

	bOwnsCapture = true;
	TimeoutHandle = Scheduler->Schedule(TimeoutSeconds, [this]() { HandleTimeout(); });
	OutOwner = TEXT("mcp");
	return true;
}

void FBoundedEditorStatCapture::Finish()
{
	Cleanup(true);
}

void FBoundedEditorStatCapture::Cleanup(bool bCancelTimeout)
{
	if (bCancelTimeout && TimeoutHandle != 0)
	{
		Scheduler->Cancel(TimeoutHandle);
	}
	TimeoutHandle = 0;

	if (bOwnsCapture)
	{
		Backend->SetCaptureEnabled(false, bRestoreStatsRendering);
	}
	bOwnsCapture = false;
}

void FBoundedEditorStatCapture::HandleTimeout()
{
	TimeoutHandle = 0;
	Cleanup(false);
}

TSharedRef<IEditorStatCaptureBackend> CreateEditorStatCaptureBackend(FName StatGroup, bool bMacroGroup)
{
	return MakeShared<FEditorStatCaptureBackend>(StatGroup, bMacroGroup);
}

TSharedRef<IEditorStatCaptureScheduler> CreateEditorStatCaptureScheduler()
{
	return MakeShared<FEditorStatCaptureScheduler>();
}

#if WITH_DEV_AUTOMATION_TESTS
TSharedRef<IEditorStatCaptureBackend> CreateEditorStatCaptureBackendForTest(
	FName StatGroup,
	TFunction<bool()> IsEnabled,
	TFunction<void(bool)> ExecuteToggle)
{
	return MakeShared<FEditorStatCaptureBackend>(StatGroup, MoveTemp(IsEnabled), MoveTemp(ExecuteToggle));
}
#endif
