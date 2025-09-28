#include "Commands/BlueprintNode/FindBlueprintNodesCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FFindBlueprintNodesCommand::FFindBlueprintNodesCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FFindBlueprintNodesCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, NodeType, EventType, TargetGraph;
    FString ParseError;
    
    // Parse parameters with proper validation
    if (!ParseParameters(Parameters, BlueprintName, NodeType, EventType, TargetGraph, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the Blueprint using common utilities - Service Layer Pattern
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Delegate to service layer for business logic
    TArray<FBlueprintNodeInfo> NodeInfos;
    if (BlueprintNodeService.FindBlueprintNodes(Blueprint, NodeType, EventType, TargetGraph, NodeInfos))
    {
        return CreateSuccessResponse(NodeInfos);
    }
    else
    {
        return CreateErrorResponse(TEXT("Failed to find Blueprint nodes"));
    }
}

FString FFindBlueprintNodesCommand::GetCommandName() const
{
    return TEXT("find_blueprint_nodes");
}

bool FFindBlueprintNodesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, NodeType, EventType, TargetGraph;
    FString ParseError;
    
    // Delegate to parameter parsing for validation
    return ParseParameters(Parameters, BlueprintName, NodeType, EventType, TargetGraph, ParseError);
}

bool FFindBlueprintNodesCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutNodeType, FString& OutEventType, FString& OutTargetGraph, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Validate blueprint name is not empty
    if (OutBlueprintName.IsEmpty())
    {
        OutError = TEXT("Blueprint name cannot be empty");
        return false;
    }
    
    // Parse optional parameters
    JsonObject->TryGetStringField(TEXT("node_type"), OutNodeType);
    JsonObject->TryGetStringField(TEXT("event_type"), OutEventType);
    JsonObject->TryGetStringField(TEXT("target_graph"), OutTargetGraph);
    
    return true;
}

FString FFindBlueprintNodesCommand::CreateSuccessResponse(const TArray<FBlueprintNodeInfo>& NodeInfos) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    // Add node information array
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FBlueprintNodeInfo& NodeInfo : NodeInfos)
    {
        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("id"), NodeInfo.NodeId);
        NodeObj->SetStringField(TEXT("title"), NodeInfo.NodeTitle);
        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }
    ResponseObj->SetArrayField(TEXT("nodes"), NodesArray);
    ResponseObj->SetNumberField(TEXT("node_count"), NodeInfos.Num());
    
    // Add node IDs array for backward compatibility
    TArray<TSharedPtr<FJsonValue>> NodeIdsArray;
    for (const FBlueprintNodeInfo& NodeInfo : NodeInfos)
    {
        NodeIdsArray.Add(MakeShared<FJsonValueString>(NodeInfo.NodeId));
    }
    ResponseObj->SetArrayField(TEXT("node_ids"), NodeIdsArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FFindBlueprintNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
