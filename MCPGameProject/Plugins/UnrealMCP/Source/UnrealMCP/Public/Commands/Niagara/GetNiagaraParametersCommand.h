#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting all parameters from a Niagara System
 * Maps to Python MCP's get_niagara_parameters
 */
class UNREALMCP_API FGetNiagaraParametersCommand : public IUnrealMCPCommand
{
public:
    explicit FGetNiagaraParametersCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_niagara_parameters"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
