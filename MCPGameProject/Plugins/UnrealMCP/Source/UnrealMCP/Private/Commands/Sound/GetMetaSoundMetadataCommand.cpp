#include "Commands/Sound/GetMetaSoundMetadataCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FGetMetaSoundMetadataCommand::FGetMetaSoundMetadataCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FGetMetaSoundMetadataCommand::GetCommandName() const
{
    return TEXT("get_metasound_metadata");
}

bool FGetMetaSoundMetadataCommand::ValidateParams(const FString& Parameters) const
{
    FString MetaSoundPath, Error;
    return ParseParameters(Parameters, MetaSoundPath, Error);
}

FString FGetMetaSoundMetadataCommand::Execute(const FString& Parameters)
{
    FString MetaSoundPath, Error;
    if (!ParseParameters(Parameters, MetaSoundPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> Metadata;
    if (!SoundService.GetMetaSoundMetadata(MetaSoundPath, Metadata, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Add success field
    Metadata->SetBoolField(TEXT("success"), true);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Metadata.ToSharedRef(), Writer);

    return OutputString;
}

bool FGetMetaSoundMetadataCommand::ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("metasound_path"), OutMetaSoundPath) || OutMetaSoundPath.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: metasound_path");
        return false;
    }

    return true;
}

FString FGetMetaSoundMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
