#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to set properties on a sound wave asset (looping, volume, pitch, sound class)
 */
class UNREALMCP_API FSetSoundWavePropertiesCommand : public IUnrealMCPCommand
{
public:
	explicit FSetSoundWavePropertiesCommand(ISoundService& InSoundService);

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	ISoundService& SoundService;

	bool ParseParameters(const FString& JsonString, FString& OutSoundWavePath,
		bool& OutLooping, float& OutVolume, float& OutPitch, FString& OutSoundClassPath, FString& OutError) const;

	FString CreateSuccessResponse(const FString& Message) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
