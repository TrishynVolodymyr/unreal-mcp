#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to execute arbitrary UE console commands.
 * Captures and returns text output when available.
 */
class UNREALMCP_API FExecuteConsoleCommandCommand : public IUnrealMCPCommand
{
public:
	FExecuteConsoleCommandCommand() = default;

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
};
