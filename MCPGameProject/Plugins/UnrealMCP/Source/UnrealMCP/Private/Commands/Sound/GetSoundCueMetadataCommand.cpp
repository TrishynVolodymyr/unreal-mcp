#include "Commands/Sound/GetSoundCueMetadataCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FGetSoundCueMetadataCommand::FGetSoundCueMetadataCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FGetSoundCueMetadataCommand::GetCommandName() const
{
    return TEXT("get_sound_cue_metadata");
}

bool FGetSoundCueMetadataCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundCuePath, Error;
    return ParseParameters(Parameters, SoundCuePath, Error);
}

FString FGetSoundCueMetadataCommand::Execute(const FString& Parameters)
{
    FString SoundCuePath, Error;
    if (!ParseParameters(Parameters, SoundCuePath, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> Metadata;
    if (!SoundService.GetSoundCueMetadata(SoundCuePath, Metadata, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Metadata);
}

bool FGetSoundCueMetadataCommand::ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("sound_cue_path"), OutSoundCuePath) || OutSoundCuePath.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: sound_cue_path");
        return false;
    }

    return true;
}

FString FGetSoundCueMetadataCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const
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

FString FGetSoundCueMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
