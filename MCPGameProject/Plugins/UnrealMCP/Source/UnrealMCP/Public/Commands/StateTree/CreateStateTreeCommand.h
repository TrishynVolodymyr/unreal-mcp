#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for creating StateTree assets
 */
class UNREALMCP_API FCreateStateTreeCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateStateTreeCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    bool ParseParameters(const FString& JsonString, FStateTreeCreationParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(class UStateTree* StateTree) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
