#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a property on an emitter
 */
class UNREALMCP_API FSetEmitterPropertyCommand : public IUnrealMCPCommand
{
public:
    explicit FSetEmitterPropertyCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraEmitterPropertyParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& PropertyName, const FString& PropertyValue) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
