#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for listing available StateTree evaluator types
 */
class UNREALMCP_API FGetAvailableEvaluatorsCommand : public IUnrealMCPCommand
{
public:
    explicit FGetAvailableEvaluatorsCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
