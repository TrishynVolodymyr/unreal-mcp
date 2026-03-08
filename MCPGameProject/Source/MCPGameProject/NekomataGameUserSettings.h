#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "NekomataGameUserSettings.generated.h"

/**
 * Custom Game User Settings for Nekomata.
 * Extends UGameUserSettings with audio, controls, and language settings.
 * All UPROPERTY(config) values auto-save to GameUserSettings.ini via SaveSettings().
 */
UCLASS(config=GameUserSettings, configdonotcheckdefaults)
class MCPGAMEPROJECT_API UNekomataGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:

	/** Helper to get the typed settings instance from Blueprint */
	UFUNCTION(BlueprintCallable, Category="Settings", DisplayName="Get Nekomata Settings")
	static UNekomataGameUserSettings* GetNekomataSettings();

	// ── Audio ──

	UPROPERTY(config, BlueprintReadWrite, Category="Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(config, BlueprintReadWrite, Category="Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(config, BlueprintReadWrite, Category="Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(config, BlueprintReadWrite, Category="Audio")
	float DialogueVolume = 1.0f;

	UPROPERTY(config, BlueprintReadWrite, Category="Audio")
	float AmbientVolume = 1.0f;

	UPROPERTY(config, BlueprintReadWrite, Category="Audio")
	bool bSubtitlesEnabled = true;

	// ── Controls ──

	UPROPERTY(config, BlueprintReadWrite, Category="Controls")
	float MouseSensitivity = 0.5f;

	UPROPERTY(config, BlueprintReadWrite, Category="Controls")
	bool bInvertY = false;

	UPROPERTY(config, BlueprintReadWrite, Category="Controls")
	bool bVibration = true;

	// ── Language ──

	UPROPERTY(config, BlueprintReadWrite, Category="Language")
	FString UILanguage = TEXT("en");

	UPROPERTY(config, BlueprintReadWrite, Category="Language")
	FString VoiceLanguage = TEXT("en");
};
