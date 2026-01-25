#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to validate all property bindings in a StateTree
 */
class UNREALMCP_API FValidateAllBindingsCommand : public IUnrealMCPCommand
{
public:
    explicit FValidateAllBindingsCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
