#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for creating new Niagara Emitter assets
 */
class UNREALMCP_API FCreateNiagaraEmitterCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateNiagaraEmitterCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraEmitterCreationParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& EmitterPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
