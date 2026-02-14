#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for adding a node to a PCG Graph
 */
class UNREALMCP_API FAddPCGNodeCommand : public IUnrealMCPCommand
{
public:
	FAddPCGNodeCommand();

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
