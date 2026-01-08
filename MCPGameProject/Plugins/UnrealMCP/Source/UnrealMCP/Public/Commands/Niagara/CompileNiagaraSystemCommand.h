#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for compiling a Niagara System
 * Maps to Python MCP's compile_niagara_system
 */
class UNREALMCP_API FCompileNiagaraSystemCommand : public IUnrealMCPCommand
{
public:
    explicit FCompileNiagaraSystemCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("compile_niagara_system"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateSuccessResponse(const FString& SystemName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
