#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Non-mutating rendering snapshot: draw calls, GPU frame time, and VRAM.
 * UE 5.8 visibility detail is reported unavailable rather than returning stale counters.
 */
class UNREALMCP_API FGetRenderingStatsCommand : public IUnrealMCPCommand
{
public:
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
