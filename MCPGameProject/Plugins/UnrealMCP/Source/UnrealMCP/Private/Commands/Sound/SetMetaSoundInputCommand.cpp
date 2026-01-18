#include "Commands/Sound/SetMetaSoundInputCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FSetMetaSoundInputCommand::FSetMetaSoundInputCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FSetMetaSoundInputCommand::GetCommandName() const
{
    return TEXT("set_metasound_input");
}

bool FSetMetaSoundInputCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MetaSoundPath, NodeId, InputName;
    return JsonObject->TryGetStringField(TEXT("metasound_path"), MetaSoundPath) && !MetaSoundPath.IsEmpty() &&
           JsonObject->TryGetStringField(TEXT("node_id"), NodeId) && !NodeId.IsEmpty() &&
           JsonObject->TryGetStringField(TEXT("input_name"), InputName) && !InputName.IsEmpty() &&
           JsonObject->HasField(TEXT("value"));
}

FString FSetMetaSoundInputCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse JSON parameters"));
    }

    FString MetaSoundPath, NodeId, InputName;
    if (!JsonObject->TryGetStringField(TEXT("metasound_path"), MetaSoundPath) || MetaSoundPath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required parameter: metasound_path"));
    }

    if (!JsonObject->TryGetStringField(TEXT("node_id"), NodeId) || NodeId.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required parameter: node_id"));
    }

    if (!JsonObject->TryGetStringField(TEXT("input_name"), InputName) || InputName.IsEmpty())
    {
        return CreateErrorResponse(TEXT("Missing required parameter: input_name"));
    }

    if (!JsonObject->HasField(TEXT("value")))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: value"));
    }

    TSharedPtr<FJsonValue> Value = JsonObject->TryGetField(TEXT("value"));

    FString Error;
    if (!SoundService.SetMetaSoundNodeInput(MetaSoundPath, NodeId, InputName, Value, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(InputName, NodeId);
}

FString FSetMetaSoundInputCommand::CreateSuccessResponse(const FString& InputName, const FString& NodeId) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Set input '%s' on node %s"), *InputName, *NodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMetaSoundInputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
