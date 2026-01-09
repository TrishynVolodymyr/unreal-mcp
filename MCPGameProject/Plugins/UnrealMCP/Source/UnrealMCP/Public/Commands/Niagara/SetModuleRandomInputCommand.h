#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for setting a random range input on a Niagara module
 * Allows defining uniform random values between min and max for inputs
 * like particle size, lifetime, velocity to create natural variation
 */
class UNREALMCP_API FSetModuleRandomInputCommand : public IUnrealMCPCommand
{
public:
    explicit FSetModuleRandomInputCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleRandomInputParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleName, const FString& InputName, const FString& MinValue, const FString& MaxValue) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
