#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class ISoundService;

/**
 * Command to create a Sound Class asset for audio categorization
 */
class UNREALMCP_API FCreateSoundClassCommand : public IUnrealMCPCommand
{
public:
	explicit FCreateSoundClassCommand(ISoundService& InSoundService);

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	ISoundService& SoundService;

	bool ParseParameters(const FString& JsonString, FString& OutAssetName, FString& OutFolderPath,
		FString& OutParentClassPath, float& OutDefaultVolume, FString& OutError) const;

	FString CreateSuccessResponse(const FString& AssetPath) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
