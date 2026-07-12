#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IEditorStatCaptureBackend;
class IEditorStatCaptureScheduler;
class FBoundedEditorStatCapture;

/**
 * Focused rendering diagnostics: draw call categories, VRAM, key counters.
 * Complements get_gpu_stats (GPU pass timing) and get_scene_breakdown (mesh instances).
 */
class UNREALMCP_API FGetRenderingStatsCommand : public IUnrealMCPCommand
{
public:
	FGetRenderingStatsCommand();
	FGetRenderingStatsCommand(
		TSharedRef<IEditorStatCaptureBackend> InBackend,
		TSharedRef<IEditorStatCaptureScheduler> InScheduler);
	virtual ~FGetRenderingStatsCommand() override;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	TSharedRef<IEditorStatCaptureBackend> Backend;
	TUniquePtr<FBoundedEditorStatCapture> Capture;
};
