#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to get real-time performance statistics from the editor.
 * Returns FPS, frame time, GPU time, memory, draw calls, triangles.
 */
class UNREALMCP_API FGetPerformanceStatsCommand : public IUnrealMCPCommand
{
public:
	FGetPerformanceStatsCommand() = default;

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
