#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for creating new Niagara System assets
 */
class UNREALMCP_API FCreateNiagaraSystemCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateNiagaraSystemCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraSystemCreationParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& SystemPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
