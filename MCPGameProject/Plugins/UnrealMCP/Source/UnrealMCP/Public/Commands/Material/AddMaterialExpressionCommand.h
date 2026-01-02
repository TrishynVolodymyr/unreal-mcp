#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/MaterialExpressionService.h"

/**
 * Command for adding material expressions to a material graph
 */
class UNREALMCP_API FAddMaterialExpressionCommand : public IUnrealMCPCommand
{
public:
    FAddMaterialExpressionCommand();

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    bool ParseParameters(const FString& JsonString, FMaterialExpressionCreationParams& OutParams, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
