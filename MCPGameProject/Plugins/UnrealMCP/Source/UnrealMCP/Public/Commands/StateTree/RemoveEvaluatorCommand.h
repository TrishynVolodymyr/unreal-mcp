#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to remove an evaluator from a StateTree
 */
class UNREALMCP_API FRemoveEvaluatorCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveEvaluatorCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
