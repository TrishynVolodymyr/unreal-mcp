#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a vector parameter on a Niagara System
 * Maps to Python MCP's set_niagara_vector_param
 */
class UNREALMCP_API FSetNiagaraVectorParamCommand : public IUnrealMCPCommand
{
public:
    explicit FSetNiagaraVectorParamCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("set_niagara_vector_param"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateSuccessResponse(const FString& ParamName, const FVector& Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
