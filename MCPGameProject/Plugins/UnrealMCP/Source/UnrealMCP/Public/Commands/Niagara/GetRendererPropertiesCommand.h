#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting all properties and bindings from a Niagara renderer
 */
class UNREALMCP_API FGetRendererPropertiesCommand : public IUnrealMCPCommand
{
public:
    explicit FGetRendererPropertiesCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
