#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for duplicating a Niagara System
 * Maps to Python MCP's duplicate_niagara_system
 */
class UNREALMCP_API FDuplicateNiagaraSystemCommand : public IUnrealMCPCommand
{
public:
    explicit FDuplicateNiagaraSystemCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("duplicate_niagara_system"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateSuccessResponse(const FString& Name, const FString& Path) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
