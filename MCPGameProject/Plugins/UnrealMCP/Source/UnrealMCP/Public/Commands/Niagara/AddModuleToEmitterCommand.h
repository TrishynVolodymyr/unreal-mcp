#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for adding a module to an emitter stage
 */
class UNREALMCP_API FAddModuleToEmitterCommand : public IUnrealMCPCommand
{
public:
    explicit FAddModuleToEmitterCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleAddParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
