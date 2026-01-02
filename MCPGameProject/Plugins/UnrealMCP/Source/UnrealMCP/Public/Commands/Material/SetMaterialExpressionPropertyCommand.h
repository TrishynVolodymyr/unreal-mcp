#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for setting a property on a material expression
 */
class UNREALMCP_API FSetMaterialExpressionPropertyCommand : public IUnrealMCPCommand
{
public:
    FSetMaterialExpressionPropertyCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateSuccessResponse(const FString& PropertyName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
