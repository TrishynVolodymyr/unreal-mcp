#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

// Forward declaration
class ISoundService;

/**
 * Command to spawn an ambient sound actor in the level
 */
class UNREALMCP_API FSpawnAmbientSoundCommand : public IUnrealMCPCommand
{
public:
    explicit FSpawnAmbientSoundCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundPath, FString& OutActorName,
        FVector& OutLocation, FRotator& OutRotation, bool& OutAutoActivate,
        FString& OutAttenuationPath, FString& OutError) const;

    FString CreateSuccessResponse(const FString& ActorName, const FVector& Location) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
