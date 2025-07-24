#include "Commands/BlueprintNode/ConnectBlueprintNodesCommand.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"

FConnectBlueprintNodesCommand::FConnectBlueprintNodesCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FConnectBlueprintNodesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }
    
    // Only batch mode is supported - require 'connections' array
    if (!JsonObject->HasField(TEXT("connections")))
    {
        return CreateErrorResponse(TEXT("Missing 'connections' parameter - only batch connections are supported"));
    }
    
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray = JsonObject->GetArrayField(TEXT("connections"));
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    
    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }
    
    // Find the blueprint and event graph once
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return CreateErrorResponse(TEXT("Failed to get event graph"));
    }
    
    for (const TSharedPtr<FJsonValue>& ConnVal : ConnectionsArray)
    {
        TSharedPtr<FJsonObject> ConnObj = ConnVal->AsObject();
        if (!ConnObj.IsValid())
        {
            TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
            Err->SetBoolField(TEXT("success"), false);
            Err->SetStringField(TEXT("message"), TEXT("Invalid connection object"));
            ResultsArray.Add(MakeShared<FJsonValueObject>(Err));
            continue;
        }
        
        FString SourceNodeId, TargetNodeId, SourcePinName, TargetPinName;
        bool ok = ConnObj->TryGetStringField(TEXT("source_node_id"), SourceNodeId)
            && ConnObj->TryGetStringField(TEXT("target_node_id"), TargetNodeId)
            && ConnObj->TryGetStringField(TEXT("source_pin"), SourcePinName)
            && ConnObj->TryGetStringField(TEXT("target_pin"), TargetPinName);
        
        if (!ok)
        {
            TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
            Err->SetBoolField(TEXT("success"), false);
            Err->SetStringField(TEXT("message"), TEXT("Missing required fields in connection object"));
            ResultsArray.Add(MakeShared<FJsonValueObject>(Err));
            continue;
        }
        
        // Find nodes
        UEdGraphNode* SourceNode = nullptr;
        UEdGraphNode* TargetNode = nullptr;
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node->NodeGuid.ToString() == SourceNodeId)
            {
                SourceNode = Node;
            }
            else if (Node->NodeGuid.ToString() == TargetNodeId)
            {
                TargetNode = Node;
            }
        }
        
        if (!SourceNode || !TargetNode)
        {
            TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
            Err->SetBoolField(TEXT("success"), false);
            Err->SetStringField(TEXT("message"), TEXT("Source or target node not found"));
            ResultsArray.Add(MakeShared<FJsonValueObject>(Err));
            continue;
        }
        
        // Connect
        if (FUnrealMCPCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
        {
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
            ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
            ResultsArray.Add(MakeShared<FJsonValueObject>(ResultObj));
        }
        else
        {
            TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
            Err->SetBoolField(TEXT("success"), false);
            Err->SetStringField(TEXT("message"), TEXT("Failed to connect nodes"));
            ResultsArray.Add(MakeShared<FJsonValueObject>(Err));
        }
    }
    
    // Return all results
    TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
    Out->SetArrayField(TEXT("results"), ResultsArray);
    Out->SetBoolField(TEXT("batch"), true);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Out.ToSharedRef(), Writer);
    return OutputString;
}

FString FConnectBlueprintNodesCommand::GetCommandName() const
{
    return TEXT("connect_blueprint_nodes");
}

bool FConnectBlueprintNodesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    TArray<FBlueprintNodeConnectionParams> Connections;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, Connections, ParseError);
}

bool FConnectBlueprintNodesCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FBlueprintNodeConnectionParams>& OutConnections, FString& OutError) const
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
    
    OutConnections.Empty();
    
    // Only batch connections format is supported
    const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
    if (!JsonObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
    {
        OutError = TEXT("Missing required 'connections' parameter - only batch connections are supported");
        return false;
    }
    
    for (const TSharedPtr<FJsonValue>& ConnectionValue : *ConnectionsArray)
    {
        const TSharedPtr<FJsonObject>* ConnectionObj;
        if (!ConnectionValue->TryGetObject(ConnectionObj) || !ConnectionObj->IsValid())
        {
            OutError = TEXT("Invalid connection object in connections array");
            return false;
        }
        
        FBlueprintNodeConnectionParams Connection;
        if (!(*ConnectionObj)->TryGetStringField(TEXT("source_node_id"), Connection.SourceNodeId) ||
            !(*ConnectionObj)->TryGetStringField(TEXT("source_pin"), Connection.SourcePin) ||
            !(*ConnectionObj)->TryGetStringField(TEXT("target_node_id"), Connection.TargetNodeId) ||
            !(*ConnectionObj)->TryGetStringField(TEXT("target_pin"), Connection.TargetPin))
        {
            OutError = TEXT("Missing required fields in connection object");
            return false;
        }
        
        FString ValidationError;
        if (!Connection.IsValid(ValidationError))
        {
            OutError = ValidationError;
            return false;
        }
        
        OutConnections.Add(Connection);
    }
    
    if (OutConnections.Num() == 0)
    {
        OutError = TEXT("No valid connections specified");
        return false;
    }
    
    return true;
}

FString FConnectBlueprintNodesCommand::CreateSuccessResponse(const TArray<bool>& Results) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    // Add results array
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (bool Result : Results)
    {
        ResultsArray.Add(MakeShared<FJsonValueBoolean>(Result));
    }
    ResponseObj->SetArrayField(TEXT("results"), ResultsArray);
    
    // Count successful connections
    int32 SuccessfulConnections = 0;
    for (bool Result : Results)
    {
        if (Result)
        {
            SuccessfulConnections++;
        }
    }
    
    ResponseObj->SetNumberField(TEXT("successful_connections"), SuccessfulConnections);
    ResponseObj->SetNumberField(TEXT("total_connections"), Results.Num());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FConnectBlueprintNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}