#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a float parameter on a Niagara System
 * Maps to Python MCP's set_niagara_float_param
 */
class UNREALMCP_API FSetNiagaraFloatParamCommand : public IUnrealMCPCommand
{
public:
    explicit FSetNiagaraFloatParamCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_niagara_float_param"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateSuccessResponse(const FString& ParamName, float Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
