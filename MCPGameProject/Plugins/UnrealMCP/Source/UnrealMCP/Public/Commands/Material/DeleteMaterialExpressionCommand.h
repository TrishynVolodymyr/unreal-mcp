#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for deleting a material expression
 */
class UNREALMCP_API FDeleteMaterialExpressionCommand : public IUnrealMCPCommand
{
public:
    FDeleteMaterialExpressionCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateSuccessResponse() const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
