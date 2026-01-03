#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for adding Data Interfaces to Niagara emitters
 */
class UNREALMCP_API FAddDataInterfaceCommand : public IUnrealMCPCommand
{
public:
    explicit FAddDataInterfaceCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraDataInterfaceParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& InterfaceId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
