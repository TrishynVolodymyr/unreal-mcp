#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to delete a variable from a Blueprint
 */
class UNREALMCP_API FDeleteBlueprintVariableCommand : public IUnrealMCPCommand
{
public:
    explicit FDeleteBlueprintVariableCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    FString CreateSuccessResponse(const FString& BlueprintName, const FString& VariableName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
