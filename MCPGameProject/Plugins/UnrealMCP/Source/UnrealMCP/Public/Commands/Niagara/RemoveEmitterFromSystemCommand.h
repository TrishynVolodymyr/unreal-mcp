#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for removing an emitter from a Niagara System
 */
class UNREALMCP_API FRemoveEmitterFromSystemCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveEmitterFromSystemCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    struct FRemoveEmitterParams
    {
        FString SystemPath;
        FString EmitterName;
    };

    bool ParseParameters(const FString& JsonString, FRemoveEmitterParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& SystemPath, const FString& EmitterName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
