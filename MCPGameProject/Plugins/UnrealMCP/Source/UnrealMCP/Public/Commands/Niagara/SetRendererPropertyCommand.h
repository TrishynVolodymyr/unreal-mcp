#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting properties on Niagara renderers
 */
class UNREALMCP_API FSetRendererPropertyCommand : public IUnrealMCPCommand
{
public:
    explicit FSetRendererPropertyCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateSuccessResponse(const FString& PropertyName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
