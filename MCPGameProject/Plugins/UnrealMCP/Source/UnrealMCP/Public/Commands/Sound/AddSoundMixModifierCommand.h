#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to add or update a Sound Class adjuster in a Sound Mix
 */
class UNREALMCP_API FAddSoundMixModifierCommand : public IUnrealMCPCommand
{
public:
	explicit FAddSoundMixModifierCommand(ISoundService& InSoundService);

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	ISoundService& SoundService;

	bool ParseParameters(const FString& JsonString, FString& OutSoundMixPath,
		FString& OutSoundClassPath, float& OutVolumeAdjust, float& OutPitchAdjust,
		FString& OutError) const;

	FString CreateSuccessResponse(const FString& SoundMixPath, const FString& SoundClassPath) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
