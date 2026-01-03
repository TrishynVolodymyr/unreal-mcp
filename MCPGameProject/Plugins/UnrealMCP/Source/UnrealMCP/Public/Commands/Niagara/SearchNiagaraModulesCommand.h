#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for searching available Niagara modules
 */
class UNREALMCP_API FSearchNiagaraModulesCommand : public IUnrealMCPCommand
{
public:
    explicit FSearchNiagaraModulesCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FString& OutSearchQuery, FString& OutStageFilter, int32& OutMaxResults, FString& OutError) const;
    FString CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Modules) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
