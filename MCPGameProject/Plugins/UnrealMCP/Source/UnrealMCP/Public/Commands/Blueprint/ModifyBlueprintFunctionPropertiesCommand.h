#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command for modifying properties of existing Blueprint functions
 * Allows changing is_pure, is_const, access_specifier, and category
 */
class UNREALMCP_API FModifyBlueprintFunctionPropertiesCommand : public IUnrealMCPCommand
{
public:
    explicit FModifyBlueprintFunctionPropertiesCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    FString CreateSuccessResponse(const FString& BlueprintName, const FString& FunctionName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
