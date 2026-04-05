#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Stop the current Play In Editor (PIE) session.
 */
class UNREALMCP_API FStopPIECommand : public IUnrealMCPCommand
{
public:
	FStopPIECommand() = default;
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
