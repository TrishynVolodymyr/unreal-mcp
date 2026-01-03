#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting parameter values on Niagara Systems
 */
class UNREALMCP_API FSetNiagaraParameterCommand : public IUnrealMCPCommand
{
public:
    explicit FSetNiagaraParameterCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateSuccessResponse(const FString& ParameterName, const FString& Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
