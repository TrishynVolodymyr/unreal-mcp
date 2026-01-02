#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for connecting an expression to a material output property
 */
class UNREALMCP_API FConnectExpressionToMaterialOutputCommand : public IUnrealMCPCommand
{
public:
    FConnectExpressionToMaterialOutputCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateSuccessResponse(const FString& MaterialProperty) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
