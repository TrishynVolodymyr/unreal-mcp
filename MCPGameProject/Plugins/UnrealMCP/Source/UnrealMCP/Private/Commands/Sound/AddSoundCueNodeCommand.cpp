#include "Commands/Sound/AddSoundCueNodeCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FAddSoundCueNodeCommand::FAddSoundCueNodeCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FAddSoundCueNodeCommand::GetCommandName() const
{
    return TEXT("add_sound_cue_node");
}

bool FAddSoundCueNodeCommand::ValidateParams(const FString& Parameters) const
{
    FString SoundCuePath, NodeType, SoundWavePath, Error;
    int32 PosX, PosY;
    return ParseParameters(Parameters, SoundCuePath, NodeType, SoundWavePath, PosX, PosY, Error);
}

FString FAddSoundCueNodeCommand::Execute(const FString& Parameters)
{
    FString SoundCuePath, NodeType, SoundWavePath, Error;
    int32 PosX, PosY;
    if (!ParseParameters(Parameters, SoundCuePath, NodeType, SoundWavePath, PosX, PosY, Error))
    {
        return CreateErrorResponse(Error);
    }

    FSoundCueNodeParams Params;
    Params.SoundCuePath = SoundCuePath;
    Params.NodeType = NodeType;
    Params.SoundWavePath = SoundWavePath;
    Params.PosX = PosX;
    Params.PosY = PosY;

    FString NodeId;
    if (!SoundService.AddSoundCueNode(Params, NodeId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(NodeId);
}

bool FAddSoundCueNodeCommand::ParseParameters(const FString& JsonString, FString& OutSoundCuePath, FString& OutNodeType,
    FString& OutSoundWavePath, int32& OutPosX, int32& OutPosY, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("node_type"), OutNodeType) || OutNodeType.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: node_type");
        return false;
    }

    // Optional parameters
    JsonObject->TryGetStringField(TEXT("sound_wave_path"), OutSoundWavePath);

    if (!JsonObject->TryGetNumberField(TEXT("pos_x"), OutPosX))
    {
        OutPosX = 0;
    }
    if (!JsonObject->TryGetNumberField(TEXT("pos_y"), OutPosY))
    {
        OutPosY = 0;
    }

    return true;
}

FString FAddSoundCueNodeCommand::CreateSuccessResponse(const FString& NodeId) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("node_id"), NodeId);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Added node: %s"), *NodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddSoundCueNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
