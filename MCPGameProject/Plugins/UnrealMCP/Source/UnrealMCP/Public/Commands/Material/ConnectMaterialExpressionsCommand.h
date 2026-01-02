#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/MaterialExpressionService.h"

/**
 * Command for connecting two material expressions together
 */
class UNREALMCP_API FConnectMaterialExpressionsCommand : public IUnrealMCPCommand
{
public:
    FConnectMaterialExpressionsCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    bool ParseParameters(const FString& JsonString, FMaterialExpressionConnectionParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse() const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
    FString CreateBatchSuccessResponse(const TArray<FString>& Results) const;
};
