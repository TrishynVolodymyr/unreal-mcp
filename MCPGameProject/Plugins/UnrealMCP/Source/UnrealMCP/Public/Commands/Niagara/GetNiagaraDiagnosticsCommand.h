#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting diagnostics/validation results from a Niagara System.
 * Uses UE's built-in NiagaraValidation system (ValidateAllRulesInSystem).
 * Maps to Python MCP's get_niagara_diagnostics
 */
class UNREALMCP_API FGetNiagaraDiagnosticsCommand : public IUnrealMCPCommand
{
public:
    explicit FGetNiagaraDiagnosticsCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_niagara_diagnostics"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
