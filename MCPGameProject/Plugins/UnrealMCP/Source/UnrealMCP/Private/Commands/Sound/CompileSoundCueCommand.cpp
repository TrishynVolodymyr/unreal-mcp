#include "Commands/Sound/CompileSoundCueCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCompileSoundCueCommand::FCompileSoundCueCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FCompileSoundCueCommand::GetCommandName() const
{
    return TEXT("compile_sound_cue");
}

bool FCompileSoundCueCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundCuePath, Error;
    return ParseParameters(Parameters, SoundCuePath, Error);
}

FString FCompileSoundCueCommand::Execute(const FString& Parameters)
{
    FString SoundCuePath, Error;
    if (!ParseParameters(Parameters, SoundCuePath, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!SoundService.CompileSoundCue(SoundCuePath, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse();
}

bool FCompileSoundCueCommand::ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutError) const
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

FString FCompileSoundCueCommand::CreateSuccessResponse() const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), TEXT("Sound Cue compiled successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FCompileSoundCueCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
