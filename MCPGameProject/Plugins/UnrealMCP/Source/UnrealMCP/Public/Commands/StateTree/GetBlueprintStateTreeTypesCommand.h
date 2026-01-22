#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to get Blueprint-based StateTree types (tasks, conditions, evaluators)
 */
class UNREALMCP_API FGetBlueprintStateTreeTypesCommand : public IUnrealMCPCommand
{
public:
    FGetBlueprintStateTreeTypesCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
