#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Start a Play In Editor (PIE) session.
 * Queues session start for the next engine tick.
 */
class UNREALMCP_API FStartPIECommand : public IUnrealMCPCommand
{
public:
	FStartPIECommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
