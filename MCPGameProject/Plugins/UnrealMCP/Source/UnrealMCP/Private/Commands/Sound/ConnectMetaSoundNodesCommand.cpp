#include "Commands/Sound/ConnectMetaSoundNodesCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FConnectMetaSoundNodesCommand::FConnectMetaSoundNodesCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FConnectMetaSoundNodesCommand::GetCommandName() const
{
    return TEXT("connect_metasound_nodes");
}

bool FConnectMetaSoundNodesCommand::ValidateParams(const FString& Parameters) const
{
    FString MetaSoundPath, SourceNodeId, SourcePinName, TargetNodeId, TargetPinName, Error;
    return ParseParameters(Parameters, MetaSoundPath, SourceNodeId, SourcePinName, TargetNodeId, TargetPinName, Error);
}

FString FConnectMetaSoundNodesCommand::Execute(const FString& Parameters)
{
    FString MetaSoundPath, SourceNodeId, SourcePinName, TargetNodeId, TargetPinName, Error;
    if (!ParseParameters(Parameters, MetaSoundPath, SourceNodeId, SourcePinName, TargetNodeId, TargetPinName, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!SoundService.ConnectMetaSoundNodes(MetaSoundPath, SourceNodeId, SourcePinName, TargetNodeId, TargetPinName, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(SourceNodeId, SourcePinName, TargetNodeId, TargetPinName);
}

bool FConnectMetaSoundNodesCommand::ParseParameters(const FString& JsonString, FString& OutMetaSoundPath,
    FString& OutSourceNodeId, FString& OutSourcePinName, FString& OutTargetNodeId, FString& OutTargetPinName, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("source_node_id"), OutSourceNodeId) || OutSourceNodeId.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: source_node_id");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("source_pin_name"), OutSourcePinName) || OutSourcePinName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: source_pin_name");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("target_node_id"), OutTargetNodeId) || OutTargetNodeId.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: target_node_id");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("target_pin_name"), OutTargetPinName) || OutTargetPinName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: target_pin_name");
        return false;
    }

    return true;
}

FString FConnectMetaSoundNodesCommand::CreateSuccessResponse(const FString& SourceNodeId, const FString& SourcePinName,
    const FString& TargetNodeId, const FString& TargetPinName) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Connected %s.%s -> %s.%s"),
        *SourceNodeId, *SourcePinName, *TargetNodeId, *TargetPinName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FConnectMetaSoundNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
