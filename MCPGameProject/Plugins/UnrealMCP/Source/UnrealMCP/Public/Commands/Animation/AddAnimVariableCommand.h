#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for adding variables to Animation Blueprints
 */
class UNREALMCP_API FAddAnimVariableCommand : public IUnrealMCPCommand
{
public:
    explicit FAddAnimVariableCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateSuccessResponse(const FString& VariableName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
