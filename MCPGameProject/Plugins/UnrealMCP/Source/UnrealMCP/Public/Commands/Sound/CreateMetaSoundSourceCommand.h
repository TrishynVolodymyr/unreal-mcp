#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to create a new MetaSound Source asset
 */
class UNREALMCP_API FCreateMetaSoundSourceCommand : public IUnrealMCPCommand
{
public:
    explicit FCreateMetaSoundSourceCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutAssetName, FString& OutFolderPath,
        FString& OutOutputFormat, bool& OutIsOneShot, FString& OutError) const;
    FString CreateSuccessResponse(const FString& AssetPath, const FString& AssetName,
        const FString& OutputFormat, bool bIsOneShot) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
