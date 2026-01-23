#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for duplicating a StateTree asset
 */
class UNREALMCP_API FDuplicateStateTreeCommand : public IUnrealMCPCommand
{
public:
    explicit FDuplicateStateTreeCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    FString CreateSuccessResponse(class UStateTree* StateTree) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
