#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for adding states to state machines in Animation Blueprints
 */
class UNREALMCP_API FAddAnimStateCommand : public IUnrealMCPCommand
{
public:
    explicit FAddAnimStateCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateSuccessResponse(const FString& StateName, const FString& StateMachineName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
