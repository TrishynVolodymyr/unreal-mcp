#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to get metadata about a Sound Cue asset
 */
class UNREALMCP_API FGetSoundCueMetadataCommand : public IUnrealMCPCommand
{
public:
    explicit FGetSoundCueMetadataCommand(ISoundService& InSoundService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    ISoundService& SoundService;

    bool ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutError) const;
    FString CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
