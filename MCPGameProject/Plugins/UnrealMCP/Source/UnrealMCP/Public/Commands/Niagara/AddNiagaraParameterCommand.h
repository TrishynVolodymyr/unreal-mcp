#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for adding parameters to Niagara Systems
 */
class UNREALMCP_API FAddNiagaraParameterCommand : public IUnrealMCPCommand
{
public:
    explicit FAddNiagaraParameterCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraParameterAddParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ParameterName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
