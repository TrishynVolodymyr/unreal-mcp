#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for removing a transition from a StateTree
 */
class UNREALMCP_API FRemoveTransitionCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveTransitionCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FRemoveTransitionParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& StateName, int32 TransitionIndex) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
