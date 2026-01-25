#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to add multiple transitions in a single operation
 */
class UNREALMCP_API FBatchAddTransitionsCommand : public IUnrealMCPCommand
{
public:
    explicit FBatchAddTransitionsCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
