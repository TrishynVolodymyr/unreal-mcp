#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a linked input on a Niagara module
 * Allows binding module inputs to particle attributes like Particles.NormalizedAge
 * for time-based animations like alpha fade over particle lifetime
 */
class UNREALMCP_API FSetModuleLinkedInputCommand : public IUnrealMCPCommand
{
public:
    explicit FSetModuleLinkedInputCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleLinkedInputParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleName, const FString& InputName, const FString& LinkedValue) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
