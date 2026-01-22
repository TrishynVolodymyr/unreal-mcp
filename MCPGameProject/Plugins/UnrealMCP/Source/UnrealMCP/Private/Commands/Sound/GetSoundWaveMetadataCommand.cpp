#include "Commands/Sound/GetSoundWaveMetadataCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FGetSoundWaveMetadataCommand::FGetSoundWaveMetadataCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FGetSoundWaveMetadataCommand::GetCommandName() const
{
    return TEXT("get_sound_wave_metadata");
}

bool FGetSoundWaveMetadataCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundWavePath, Error;
    return ParseParameters(Parameters, SoundWavePath, Error);
}

FString FGetSoundWaveMetadataCommand::Execute(const FString& Parameters)
{
    FString SoundWavePath, Error;
    if (!ParseParameters(Parameters, SoundWavePath, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> Metadata;
    if (!SoundService.GetSoundWaveMetadata(SoundWavePath, Metadata, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Metadata);
}

bool FGetSoundWaveMetadataCommand::ParseParameters(const FString& JsonString, FString& OutSoundWavePath, FString& OutError) const
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

    return true;
}

FString FGetSoundWaveMetadataCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);

    // Copy metadata fields to response
    if (Metadata.IsValid())
    {
        for (const auto& Pair : Metadata->Values)
        {
            Response->SetField(Pair.Key, Pair.Value);
        }
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetSoundWaveMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
