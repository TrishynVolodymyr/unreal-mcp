#include "Commands/Sound/SetSoundWavePropertiesCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FSetSoundWavePropertiesCommand::FSetSoundWavePropertiesCommand(ISoundService& InSoundService)
	: SoundService(InSoundService)
{
}

FString FSetSoundWavePropertiesCommand::GetCommandName() const
{
	return TEXT("set_sound_wave_properties");
}

bool FSetSoundWavePropertiesCommand::ValidateParams(const FString& Parameters) const
{
	FString SoundWavePath, SoundClassPath, Error;
	bool bLooping;
	float Volume, Pitch;
	return ParseParameters(Parameters, SoundWavePath, bLooping, Volume, Pitch, SoundClassPath, Error);
}

FString FSetSoundWavePropertiesCommand::Execute(const FString& Parameters)
{
	FString SoundWavePath, SoundClassPath, Error;
	bool bLooping;
	float Volume, Pitch;

	if (!ParseParameters(Parameters, SoundWavePath, bLooping, Volume, Pitch, SoundClassPath, Error))
	{
		return CreateErrorResponse(Error);
	}

	if (!SoundService.SetSoundWaveProperties(SoundWavePath, bLooping, Volume, Pitch, SoundClassPath, Error))
	{
		return CreateErrorResponse(Error);
	}

	return CreateSuccessResponse(FString::Printf(TEXT("Set properties on sound wave: %s"), *SoundWavePath));
}

bool FSetSoundWavePropertiesCommand::ParseParameters(const FString& JsonString, FString& OutSoundWavePath,
	bool& OutLooping, float& OutVolume, float& OutPitch, FString& OutSoundClassPath, FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Failed to parse JSON parameters");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("sound_wave_path"), OutSoundWavePath) || OutSoundWavePath.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: sound_wave_path");
		return false;
	}

	OutLooping = false;
	JsonObject->TryGetBoolField(TEXT("looping"), OutLooping);

	OutVolume = 1.0f;
	JsonObject->TryGetNumberField(TEXT("volume"), OutVolume);

	OutPitch = 1.0f;
	JsonObject->TryGetNumberField(TEXT("pitch"), OutPitch);

	OutSoundClassPath = TEXT("");
	JsonObject->TryGetStringField(TEXT("sound_class_path"), OutSoundClassPath);

	return true;
}

FString FSetSoundWavePropertiesCommand::CreateSuccessResponse(const FString& Message) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("message"), Message);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}

FString FSetSoundWavePropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}
