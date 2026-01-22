#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for removing a state from a StateTree
 */
class UNREALMCP_API FRemoveStateCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveStateCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FRemoveStateParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& StateName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
