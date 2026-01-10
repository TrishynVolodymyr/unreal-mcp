#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting all modules in an emitter organized by stage
 */
class UNREALMCP_API FGetEmitterModulesCommand : public IUnrealMCPCommand
{
public:
    explicit FGetEmitterModulesCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FString& OutSystemPath, FString& OutEmitterName, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
