#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to create a new Sound Cue asset
 */
class UNREALMCP_API FCreateSoundCueCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateSoundCueCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutAssetName, FString& OutFolderPath,
        FString& OutInitialSoundWave, FString& OutError) const;
    FString CreateSuccessResponse(const FString& AssetPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
