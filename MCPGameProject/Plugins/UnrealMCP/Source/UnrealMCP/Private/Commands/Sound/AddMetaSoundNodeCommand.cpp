#include "Commands/Sound/AddMetaSoundNodeCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FAddMetaSoundNodeCommand::FAddMetaSoundNodeCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FAddMetaSoundNodeCommand::GetCommandName() const
{
    return TEXT("add_metasound_node");
}

bool FAddMetaSoundNodeCommand::ValidateParams(const FString& Parameters) const
{
    FString MetaSoundPath, NodeClassName, NodeNamespace, NodeVariant, Error;
    int32 PosX, PosY;
    return ParseParameters(Parameters, MetaSoundPath, NodeClassName, NodeNamespace, NodeVariant, PosX, PosY, Error);
}

FString FAddMetaSoundNodeCommand::Execute(const FString& Parameters)
{
    FString MetaSoundPath, NodeClassName, NodeNamespace, NodeVariant, Error;
    int32 PosX, PosY;
    if (!ParseParameters(Parameters, MetaSoundPath, NodeClassName, NodeNamespace, NodeVariant, PosX, PosY, Error))
    {
        return CreateErrorResponse(Error);
    }

    FMetaSoundNodeParams Params;
    Params.MetaSoundPath = MetaSoundPath;
    Params.NodeClassName = NodeClassName;
    Params.NodeNamespace = NodeNamespace;
    Params.NodeVariant = NodeVariant;
    Params.PosX = PosX;
    Params.PosY = PosY;

    FString NodeId;
    if (!SoundService.AddMetaSoundNode(Params, NodeId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(NodeId, NodeClassName, NodeNamespace, NodeVariant);
}

bool FAddMetaSoundNodeCommand::ParseParameters(const FString& JsonString, FString& OutMetaSoundPath,
    FString& OutNodeClassName, FString& OutNodeNamespace, FString& OutNodeVariant, int32& OutPosX, int32& OutPosY, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("node_class_name"), OutNodeClassName) || OutNodeClassName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: node_class_name");
        return false;
    }

    // Optional parameters
    if (!JsonObject->TryGetStringField(TEXT("node_namespace"), OutNodeNamespace))
    {
        OutNodeNamespace = TEXT("UE");
    }

    // Optional variant (e.g., "Audio" for oscillator nodes)
    if (!JsonObject->TryGetStringField(TEXT("node_variant"), OutNodeVariant))
    {
        OutNodeVariant = TEXT("");
    }

    double TempPosX, TempPosY;
    OutPosX = JsonObject->TryGetNumberField(TEXT("pos_x"), TempPosX) ? static_cast<int32>(TempPosX) : 0;
    OutPosY = JsonObject->TryGetNumberField(TEXT("pos_y"), TempPosY) ? static_cast<int32>(TempPosY) : 0;

    return true;
}

FString FAddMetaSoundNodeCommand::CreateSuccessResponse(const FString& NodeId, const FString& NodeClassName, const FString& NodeNamespace, const FString& NodeVariant) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("node_id"), NodeId);
    Response->SetStringField(TEXT("node_class_name"), NodeClassName);
    Response->SetStringField(TEXT("node_namespace"), NodeNamespace);
    Response->SetStringField(TEXT("node_variant"), NodeVariant);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Added node '%s::%s' (variant: '%s', ID: %s)"),
        *NodeNamespace, *NodeClassName, *NodeVariant, *NodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddMetaSoundNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
