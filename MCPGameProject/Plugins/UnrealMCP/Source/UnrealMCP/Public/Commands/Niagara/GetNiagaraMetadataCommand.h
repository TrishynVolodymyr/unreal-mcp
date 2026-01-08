#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for getting metadata about Niagara assets
 */
class UNREALMCP_API FGetNiagaraMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetNiagaraMetadataCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    struct FGetMetadataParams
    {
        FString AssetPath;
        TArray<FString> Fields;
        // Optional params for module_inputs field
        FString EmitterName;
        FString ModuleName;
        FString Stage;
    };

    bool ParseParameters(const FString& JsonString, FGetMetadataParams& OutParams, FString& OutError) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
