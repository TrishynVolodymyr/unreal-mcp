#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting metadata from a Niagara System
 * Maps to Python MCP's get_niagara_system_metadata
 */
class UNREALMCP_API FGetNiagaraSystemMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetNiagaraSystemMetadataCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("get_niagara_system_metadata"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
