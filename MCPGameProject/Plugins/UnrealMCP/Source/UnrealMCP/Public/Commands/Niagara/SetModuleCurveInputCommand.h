#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a curve input on a Niagara module
 * Allows defining float curves for inputs like "Scale Factor" to create
 * scale-over-life effects
 */
class UNREALMCP_API FSetModuleCurveInputCommand : public IUnrealMCPCommand
{
public:
    explicit FSetModuleCurveInputCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleCurveInputParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleName, const FString& InputName, int32 KeyframeCount) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
