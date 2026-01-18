#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to create a sound attenuation settings asset
 */
class UNREALMCP_API FCreateSoundAttenuationCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateSoundAttenuationCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutAssetName, FString& OutFolderPath,
        float& OutInnerRadius, float& OutFalloffDistance, FString& OutAttenuationFunction,
        bool& OutSpatialize, FString& OutError) const;

    FString CreateSuccessResponse(const FString& AssetPath) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
