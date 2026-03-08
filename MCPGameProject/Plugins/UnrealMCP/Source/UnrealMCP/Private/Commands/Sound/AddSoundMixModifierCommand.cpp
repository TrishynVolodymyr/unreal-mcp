#include "Commands/Sound/AddSoundMixModifierCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FAddSoundMixModifierCommand::FAddSoundMixModifierCommand(ISoundService& InSoundService)
	: SoundService(InSoundService)
{
}

FString FAddSoundMixModifierCommand::GetCommandName() const
{
	return TEXT("add_sound_mix_modifier");
}

bool FAddSoundMixModifierCommand::ValidateParams(const FString& Parameters) const
{
	FString SoundMixPath, SoundClassPath, Error;
	float VolumeAdjust, PitchAdjust;
	return ParseParameters(Parameters, SoundMixPath, SoundClassPath, VolumeAdjust, PitchAdjust, Error);
}

FString FAddSoundMixModifierCommand::Execute(const FString& Parameters)
{
	FString SoundMixPath, SoundClassPath, Error;
	float VolumeAdjust, PitchAdjust;

	if (!ParseParameters(Parameters, SoundMixPath, SoundClassPath, VolumeAdjust, PitchAdjust, Error))
	{
		return CreateErrorResponse(Error);
	}

	if (!SoundService.AddSoundMixModifier(SoundMixPath, SoundClassPath, VolumeAdjust, PitchAdjust, Error))
	{
		return CreateErrorResponse(Error);
	}

	return CreateSuccessResponse(SoundMixPath, SoundClassPath);
}

bool FAddSoundMixModifierCommand::ParseParameters(const FString& JsonString, FString& OutSoundMixPath,
	FString& OutSoundClassPath, float& OutVolumeAdjust, float& OutPitchAdjust, FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Failed to parse JSON parameters");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("sound_mix_path"), OutSoundMixPath) || OutSoundMixPath.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: sound_mix_path");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("sound_class_path"), OutSoundClassPath) || OutSoundClassPath.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: sound_class_path");
		return false;
	}

	OutVolumeAdjust = 1.0f;
	JsonObject->TryGetNumberField(TEXT("volume_adjust"), OutVolumeAdjust);

	OutPitchAdjust = 1.0f;
	JsonObject->TryGetNumberField(TEXT("pitch_adjust"), OutPitchAdjust);

	return true;
}

FString FAddSoundMixModifierCommand::CreateSuccessResponse(const FString& SoundMixPath, const FString& SoundClassPath) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("sound_mix_path"), SoundMixPath);
	Response->SetStringField(TEXT("sound_class_path"), SoundClassPath);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Added modifier to %s for %s"), *SoundMixPath, *SoundClassPath));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}

FString FAddSoundMixModifierCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}
