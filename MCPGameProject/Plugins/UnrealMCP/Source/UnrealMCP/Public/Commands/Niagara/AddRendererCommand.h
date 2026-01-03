#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for adding renderers to Niagara emitters
 */
class UNREALMCP_API FAddRendererCommand : public IUnrealMCPCommand
{
public:
    explicit FAddRendererCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraRendererParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& RendererId) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
