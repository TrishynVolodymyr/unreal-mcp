#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for removing a module from an emitter stage
 */
class UNREALMCP_API FRemoveModuleFromEmitterCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveModuleFromEmitterCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleRemoveParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
