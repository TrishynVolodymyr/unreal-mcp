#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for retrieving diagnostics/validation results from a StateTree
 */
class UNREALMCP_API FGetStateTreeDiagnosticsCommand : public IUnrealMCPCommand
{
public:
    explicit FGetStateTreeDiagnosticsCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
