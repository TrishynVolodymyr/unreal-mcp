#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for adding conditions to transitions in a StateTree
 */
class UNREALMCP_API FAddConditionToTransitionCommand : public IUnrealMCPCommand
{
public:
    explicit FAddConditionToTransitionCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FAddConditionParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& StateName, int32 TransitionIndex) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
