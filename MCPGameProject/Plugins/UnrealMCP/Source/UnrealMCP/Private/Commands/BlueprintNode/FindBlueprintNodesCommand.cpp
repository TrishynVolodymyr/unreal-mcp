#include "Commands/BlueprintNode/FindBlueprintNodesCommand.h"
#include "Commands/UnrealMCPCommonUtils.h"
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
    
    if (!ParseParameters(Parameters, BlueprintName, NodeType, EventType, TargetGraph, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the Blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Find nodes using the service
    TArray<FString> NodeIds;
    if (BlueprintNodeService.FindBlueprintNodes(Blueprint, NodeType, EventType, TargetGraph, NodeIds))
    {
        return CreateSuccessResponse(NodeIds);
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
    
    // Parse optional parameters
    JsonObject->TryGetStringField(TEXT("node_type"), OutNodeType);
    JsonObject->TryGetStringField(TEXT("event_type"), OutEventType);
    JsonObject->TryGetStringField(TEXT("target_graph"), OutTargetGraph);
    
    return true;
}

FString FFindBlueprintNodesCommand::CreateSuccessResponse(const TArray<FString>& NodeIds) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    // Add node IDs array
    TArray<TSharedPtr<FJsonValue>> NodeIdsArray;
    for (const FString& NodeId : NodeIds)
    {
        NodeIdsArray.Add(MakeShared<FJsonValueString>(NodeId));
    }
    ResponseObj->SetArrayField(TEXT("node_ids"), NodeIdsArray);
    ResponseObj->SetNumberField(TEXT("node_count"), NodeIds.Num());
    
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