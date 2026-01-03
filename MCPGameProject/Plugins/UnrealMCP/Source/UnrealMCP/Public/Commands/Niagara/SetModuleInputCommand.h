#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting an input value on a module
 */
class UNREALMCP_API FSetModuleInputCommand : public IUnrealMCPCommand
{
public:
    explicit FSetModuleInputCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleInputParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& PreviousValue, const FString& NewValue) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
