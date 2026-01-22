#include "Commands/Sound/RemoveSoundCueNodeCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FRemoveSoundCueNodeCommand::FRemoveSoundCueNodeCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FRemoveSoundCueNodeCommand::GetCommandName() const
{
    return TEXT("remove_sound_cue_node");
}

bool FRemoveSoundCueNodeCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundCuePath, NodeId, Error;
    return ParseParameters(Parameters, SoundCuePath, NodeId, Error);
}

FString FRemoveSoundCueNodeCommand::Execute(const FString& Parameters)
{
    FString SoundCuePath, NodeId, Error;
    if (!ParseParameters(Parameters, SoundCuePath, NodeId, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!SoundService.RemoveSoundCueNode(SoundCuePath, NodeId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(NodeId);
}

bool FRemoveSoundCueNodeCommand::ParseParameters(const FString& JsonString, FString& OutSoundCuePath,
    FString& OutNodeId, FString& OutError) const
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

    return true;
}

FString FRemoveSoundCueNodeCommand::CreateSuccessResponse(const FString& NodeId) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Removed node: %s"), *NodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FRemoveSoundCueNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
