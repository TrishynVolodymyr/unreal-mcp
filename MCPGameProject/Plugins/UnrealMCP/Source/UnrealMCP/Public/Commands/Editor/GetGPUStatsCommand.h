#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to get per-pass GPU profiler timing data.
 * Returns breakdown: BasePass, Shadows, Translucency, PostProcessing, etc.
 */
class UNREALMCP_API FGetGPUStatsCommand : public IUnrealMCPCommand
{
public:
	FGetGPUStatsCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
