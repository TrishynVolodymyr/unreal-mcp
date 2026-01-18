#include "Commands/Sound/ConnectSoundCueNodesCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FConnectSoundCueNodesCommand::FConnectSoundCueNodesCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FConnectSoundCueNodesCommand::GetCommandName() const
{
    return TEXT("connect_sound_cue_nodes");
}

bool FConnectSoundCueNodesCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundCuePath, SourceNodeId, TargetNodeId, Error;
    int32 SourcePinIndex, TargetPinIndex;
    return ParseParameters(Parameters, SoundCuePath, SourceNodeId, TargetNodeId, SourcePinIndex, TargetPinIndex, Error);
}

FString FConnectSoundCueNodesCommand::Execute(const FString& Parameters)
{
    FString SoundCuePath, SourceNodeId, TargetNodeId, Error;
    int32 SourcePinIndex, TargetPinIndex;
    if (!ParseParameters(Parameters, SoundCuePath, SourceNodeId, TargetNodeId, SourcePinIndex, TargetPinIndex, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!SoundService.ConnectSoundCueNodes(SoundCuePath, SourceNodeId, TargetNodeId, SourcePinIndex, TargetPinIndex, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse();
}

bool FConnectSoundCueNodesCommand::ParseParameters(const FString& JsonString, FString& OutSoundCuePath,
    FString& OutSourceNodeId, FString& OutTargetNodeId, int32& OutSourcePinIndex, int32& OutTargetPinIndex, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("source_node_id"), OutSourceNodeId) || OutSourceNodeId.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: source_node_id");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("target_node_id"), OutTargetNodeId) || OutTargetNodeId.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: target_node_id");
        return false;
    }

    // Optional parameters with defaults
    if (!JsonObject->TryGetNumberField(TEXT("source_pin_index"), OutSourcePinIndex))
    {
        OutSourcePinIndex = 0;
    }
    if (!JsonObject->TryGetNumberField(TEXT("target_pin_index"), OutTargetPinIndex))
    {
        OutTargetPinIndex = 0;
    }

    return true;
}

FString FConnectSoundCueNodesCommand::CreateSuccessResponse() const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), TEXT("Nodes connected successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FConnectSoundCueNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
