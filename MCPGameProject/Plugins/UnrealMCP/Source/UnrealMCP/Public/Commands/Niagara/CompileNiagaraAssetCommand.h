#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for compiling Niagara assets
 */
class UNREALMCP_API FCompileNiagaraAssetCommand : public IUnrealMCPCommand
{
public:
    explicit FCompileNiagaraAssetCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    struct FCompileParams
    {
        FString AssetPath;
    };

    bool ParseParameters(const FString& JsonString, FCompileParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& Warnings = TEXT("")) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
