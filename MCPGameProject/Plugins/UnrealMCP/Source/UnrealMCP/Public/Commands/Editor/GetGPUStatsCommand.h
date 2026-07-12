#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IGPUStatsCaptureBackend;
class IEditorStatCaptureScheduler;
class FBoundedEditorStatCapture;

/**
 * Command to get per-pass GPU profiler timing data.
 * Returns breakdown: BasePass, Shadows, Translucency, PostProcessing, etc.
 */
class UNREALMCP_API FGetGPUStatsCommand : public IUnrealMCPCommand
{
public:
	FGetGPUStatsCommand();
	FGetGPUStatsCommand(
		TSharedRef<IGPUStatsCaptureBackend> InBackend,
		TSharedRef<IEditorStatCaptureScheduler> InScheduler);
	virtual ~FGetGPUStatsCommand() override;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	TSharedRef<IGPUStatsCaptureBackend> Backend;
	TUniquePtr<FBoundedEditorStatCapture> Capture;
};
