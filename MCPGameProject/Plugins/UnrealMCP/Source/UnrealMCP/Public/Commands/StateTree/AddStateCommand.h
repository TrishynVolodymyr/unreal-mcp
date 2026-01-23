#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for adding states to a StateTree
 */
class UNREALMCP_API FAddStateCommand : public IUnrealMCPCommand
{
public:
    explicit FAddStateCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FAddStateParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& StateName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
