#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a color curve input on a Niagara module
 * Allows defining RGBA color gradients for inputs like "Color" to create
 * color-over-life effects
 */
class UNREALMCP_API FSetModuleColorCurveInputCommand : public IUnrealMCPCommand
{
public:
    explicit FSetModuleColorCurveInputCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleColorCurveInputParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleName, const FString& InputName, int32 KeyframeCount) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
