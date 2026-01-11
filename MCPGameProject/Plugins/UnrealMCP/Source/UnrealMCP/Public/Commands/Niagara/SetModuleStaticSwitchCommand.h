#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a static switch on a Niagara module
 * Static switches control compile-time branching in modules, enabling different behavior modes
 */
class UNREALMCP_API FSetModuleStaticSwitchCommand : public IUnrealMCPCommand
{
public:
    explicit FSetModuleStaticSwitchCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleStaticSwitchParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& SwitchName, const FString& Value) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
