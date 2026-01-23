#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for adding tasks to states in a StateTree
 */
class UNREALMCP_API FAddTaskToStateCommand : public IUnrealMCPCommand
{
public:
    explicit FAddTaskToStateCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FAddTaskParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& StateName, const FString& TaskType) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
