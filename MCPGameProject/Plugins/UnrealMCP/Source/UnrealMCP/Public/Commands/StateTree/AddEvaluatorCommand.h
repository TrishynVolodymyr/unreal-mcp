#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for adding evaluators to a StateTree
 */
class UNREALMCP_API FAddEvaluatorCommand : public IUnrealMCPCommand
{
public:
    explicit FAddEvaluatorCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FAddEvaluatorParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& EvaluatorType) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
