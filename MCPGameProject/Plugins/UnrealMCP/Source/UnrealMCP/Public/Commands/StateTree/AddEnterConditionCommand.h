#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for adding enter conditions to states in a StateTree
 */
class UNREALMCP_API FAddEnterConditionCommand : public IUnrealMCPCommand
{
public:
    explicit FAddEnterConditionCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FAddEnterConditionParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& StateName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
