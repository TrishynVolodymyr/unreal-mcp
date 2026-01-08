#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Command for creating state machines in Animation Blueprints
 */
class UNREALMCP_API FCreateAnimStateMachineCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateAnimStateMachineCommand(IAnimationBlueprintService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IAnimationBlueprintService& Service;

    FString CreateSuccessResponse(const FString& StateMachineName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
