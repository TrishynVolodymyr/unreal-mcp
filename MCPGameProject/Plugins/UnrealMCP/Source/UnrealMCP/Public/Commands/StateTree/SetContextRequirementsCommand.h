#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * Command to set context requirements for a StateTree schema
 */
class UNREALMCP_API FSetContextRequirementsCommand : public IUnrealMCPCommand
{
public:
    FSetContextRequirementsCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
