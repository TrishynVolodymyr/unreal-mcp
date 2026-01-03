#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/INiagaraService.h"

/**
 * Command for spawning Niagara actors in the level
 */
class UNREALMCP_API FSpawnNiagaraActorCommand : public IUnrealMCPCommand
{
public:
    explicit FSpawnNiagaraActorCommand(INiagaraService& InNiagaraService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    INiagaraService& NiagaraService;

    bool ParseParameters(const FString& JsonString, FNiagaraActorSpawnParams& OutParams, FString& OutError) const;
    FString CreateSuccessResponse(const FString& ActorName) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
