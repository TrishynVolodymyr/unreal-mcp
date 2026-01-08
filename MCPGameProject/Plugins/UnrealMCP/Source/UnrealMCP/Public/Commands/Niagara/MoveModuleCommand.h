#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for moving a module to a new position within its stage
 */
class UNREALMCP_API FMoveModuleCommand : public IUnrealMCPCommand
{
public:
    explicit FMoveModuleCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraModuleMoveParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ModuleName, int32 NewIndex) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
