#include "Commands/Sound/SetSoundCueNodePropertyCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FSetSoundCueNodePropertyCommand::FSetSoundCueNodePropertyCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FSetSoundCueNodePropertyCommand::GetCommandName() const
{
    return TEXT("set_sound_cue_node_property");
}

bool FSetSoundCueNodePropertyCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundCuePath, NodeId, PropertyName, Error;
    TSharedPtr<FJsonValue> PropertyValue;
    return ParseParameters(Parameters, SoundCuePath, NodeId, PropertyName, PropertyValue, Error);
}

FString FSetSoundCueNodePropertyCommand::Execute(const FString& Parameters)
{
    FString SoundCuePath, NodeId, PropertyName, Error;
    TSharedPtr<FJsonValue> PropertyValue;
    if (!ParseParameters(Parameters, SoundCuePath, NodeId, PropertyName, PropertyValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!SoundService.SetSoundCueNodeProperty(SoundCuePath, NodeId, PropertyName, PropertyValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse();
}

bool FSetSoundCueNodePropertyCommand::ParseParameters(const FString& JsonString, FString& OutSoundCuePath,
    FString& OutNodeId, FString& OutPropertyName, TSharedPtr<FJsonValue>& OutPropertyValue, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("node_id"), OutNodeId) || OutNodeId.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: node_id");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("property_name"), OutPropertyName) || OutPropertyName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: property_name");
        return false;
    }

    OutPropertyValue = JsonObject->TryGetField(TEXT("value"));
    if (!OutPropertyValue.IsValid())
    {
        OutError = TEXT("Missing required parameter: value");
        return false;
    }

    return true;
}

FString FSetSoundCueNodePropertyCommand::CreateSuccessResponse() const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), TEXT("Property set successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetSoundCueNodePropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
