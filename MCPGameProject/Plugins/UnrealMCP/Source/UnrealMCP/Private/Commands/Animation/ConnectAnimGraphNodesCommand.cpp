#include "Commands/Animation/ConnectAnimGraphNodesCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FConnectAnimGraphNodesCommand::FConnectAnimGraphNodesCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FConnectAnimGraphNodesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString AnimBlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing required 'anim_blueprint_name' parameter"));
    }

    FString SourceNodeName;
    if (!JsonObject->TryGetStringField(TEXT("source_node_name"), SourceNodeName))
    {
        return CreateErrorResponse(TEXT("Missing required 'source_node_name' parameter"));
    }

    // Target node name is optional - empty means output pose/root
    FString TargetNodeName;
    JsonObject->TryGetStringField(TEXT("target_node_name"), TargetNodeName);

    // Pin names with defaults
    FString SourcePinName = TEXT("Pose");
    FString TargetPinName = TEXT("Result");
    JsonObject->TryGetStringField(TEXT("source_pin_name"), SourcePinName);
    JsonObject->TryGetStringField(TEXT("target_pin_name"), TargetPinName);

    UAnimBlueprint* AnimBlueprint = Service.FindAnimBlueprint(AnimBlueprintName);
    if (!AnimBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Blueprint '%s' not found"), *AnimBlueprintName));
    }

    FString Error;
    if (!Service.ConnectAnimGraphNodes(AnimBlueprint, SourceNodeName, TargetNodeName, SourcePinName, TargetPinName, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(SourceNodeName, TargetNodeName, SourcePinName, TargetPinName);
}

FString FConnectAnimGraphNodesCommand::GetCommandName() const
{
    return TEXT("connect_anim_graph_nodes");
}

bool FConnectAnimGraphNodesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, SourceNodeName;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("source_node_name"), SourceNodeName);
}

FString FConnectAnimGraphNodesCommand::CreateSuccessResponse(const FString& SourceNodeName, const FString& TargetNodeName, const FString& SourcePinName, const FString& TargetPinName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("source_node"), SourceNodeName);
    ResponseObj->SetStringField(TEXT("target_node"), TargetNodeName.IsEmpty() ? TEXT("OutputPose") : TargetNodeName);
    ResponseObj->SetStringField(TEXT("source_pin"), SourcePinName);
    ResponseObj->SetStringField(TEXT("target_pin"), TargetPinName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully connected '%s.%s' to '%s.%s'"),
        *SourceNodeName, *SourcePinName,
        TargetNodeName.IsEmpty() ? TEXT("OutputPose") : *TargetNodeName, *TargetPinName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FConnectAnimGraphNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
