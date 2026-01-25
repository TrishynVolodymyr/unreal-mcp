#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IStateTreeService.h"

/**
 * Command for compiling a StateTree for runtime use
 */
class UNREALMCP_API FCompileStateTreeCommand : public IUnrealMCPCommand
{
public:
    explicit FCompileStateTreeCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IStateTreeService& Service;

    FString CreateSuccessResponse(const FString& StateTreeName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
