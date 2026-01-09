#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for enabling/disabling an emitter within a Niagara System
 */
class UNREALMCP_API FSetEmitterEnabledCommand : public IUnrealMCPCommand
{
public:
    explicit FSetEmitterEnabledCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    struct FSetEmitterEnabledParams
    {
        FString SystemPath;
        FString EmitterName;
        bool bEnabled = true;
    };

    bool ParseParameters(const FString& JsonString, FSetEmitterEnabledParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& EmitterName, bool bEnabled) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
