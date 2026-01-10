#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting properties from an emitter
 */
class UNREALMCP_API FGetEmitterPropertiesCommand : public IUnrealMCPCommand
{
public:
    explicit FGetEmitterPropertiesCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FString& OutSystemPath, FString& OutEmitterName, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
