#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for adding an emitter to a Niagara System
 */
class UNREALMCP_API FAddEmitterToSystemCommand : public IUnrealMCPCommand
{
public:
    explicit FAddEmitterToSystemCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    struct FAddEmitterParams
    {
        FString SystemPath;
        FString EmitterPath;
        FString EmitterName;
    };

    bool ParseParameters(const FString& JsonString, FAddEmitterParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FGuid& EmitterHandleId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
