#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Focused rendering diagnostics: draw call categories, VRAM, key counters.
 * Complements get_gpu_stats (GPU pass timing) and get_scene_breakdown (mesh instances).
 */
class UNREALMCP_API FGetRenderingStatsCommand : public IUnrealMCPCommand
{
public:
	FGetRenderingStatsCommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
