#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to get exposed output properties from a node (evaluator or context)
 */
class UNREALMCP_API FGetNodeExposedOutputsCommand : public IUnrealMCPCommand
{
public:
    FGetNodeExposedOutputsCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
