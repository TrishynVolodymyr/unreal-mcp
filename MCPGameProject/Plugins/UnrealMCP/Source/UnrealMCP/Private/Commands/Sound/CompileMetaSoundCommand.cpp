#include "Commands/Sound/CompileMetaSoundCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCompileMetaSoundCommand::FCompileMetaSoundCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FCompileMetaSoundCommand::GetCommandName() const
{
    return TEXT("compile_metasound");
}

bool FCompileMetaSoundCommand::ValidateParams(const FString& Parameters) const
{
    FString MetaSoundPath, Error;
    return ParseParameters(Parameters, MetaSoundPath, Error);
}

FString FCompileMetaSoundCommand::Execute(const FString& Parameters)
{
    FString MetaSoundPath, Error;
    if (!ParseParameters(Parameters, MetaSoundPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!SoundService.CompileMetaSound(MetaSoundPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(MetaSoundPath);
}

bool FCompileMetaSoundCommand::ParseParameters(const FString& JsonString, FString& OutMetaSoundPath, FString& OutError) const
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

FString FCompileMetaSoundCommand::CreateSuccessResponse(const FString& MetaSoundPath) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Compiled MetaSound: %s"), *MetaSoundPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FCompileMetaSoundCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
