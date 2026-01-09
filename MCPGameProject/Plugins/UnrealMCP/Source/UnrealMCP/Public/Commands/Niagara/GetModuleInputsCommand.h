#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting input information (names, types) for a module
 */
class UNREALMCP_API FGetModuleInputsCommand : public IUnrealMCPCommand
{
public:
    explicit FGetModuleInputsCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FString& OutSystemPath, FString& OutEmitterName,
                        FString& OutModuleName, FString& OutStage, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
