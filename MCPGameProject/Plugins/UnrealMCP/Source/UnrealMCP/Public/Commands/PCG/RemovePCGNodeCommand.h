#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for removing a node from a PCG Graph.
 * Automatically disconnects all edges. Cannot remove Input/Output nodes.
 */
class UNREALMCP_API FRemovePCGNodeCommand : public IUnrealMCPCommand
{
public:
	FRemovePCGNodeCommand();

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
