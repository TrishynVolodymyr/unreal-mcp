#include "Commands/BlueprintNode/ConnectBlueprintNodesCommand.h"
#include "Services/IBlueprintNodeService.h"
#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FConnectBlueprintNodesCommand::FConnectBlueprintNodesCommand(TSharedPtr<IBlueprintNodeService> InBlueprintNodeService)
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

    FString BlueprintName;
    TArray<FBlueprintNodeConnectionParams> Connections;
    FString TargetGraph;
    FString ParseError;
    
    if (!ParseParameters(JsonObject, BlueprintName, Connections, TargetGraph, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    // Find the blueprint using common utils (for now, until service layer is fully implemented)
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Use enhanced connection service to get detailed results including auto-inserted nodes
    TArray<FConnectionResultInfo> EnhancedResults;
    FBlueprintNodeConnectionService::Get().ConnectBlueprintNodesEnhanced(Blueprint, Connections, TargetGraph, EnhancedResults);

    // Build response with enhanced info
    return CreateEnhancedResponse(EnhancedResults, Connections);
}

FString FConnectBlueprintNodesCommand::GetCommandName() const
{
    return TEXT("connect_blueprint_nodes");
}

bool FConnectBlueprintNodesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    TArray<FBlueprintNodeConnectionParams> Connections;
    FString TargetGraph;
    FString ParseError;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    
    return ParseParameters(JsonObject, BlueprintName, Connections, TargetGraph, ParseError);
}

bool FConnectBlueprintNodesCommand::ParseParameters(const TSharedPtr<FJsonObject>& JsonObject, FString& OutBlueprintName, TArray<FBlueprintNodeConnectionParams>& OutConnections, FString& OutTargetGraph, FString& OutError) const
{
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Parse optional target_graph parameter (defaults to "EventGraph")
    if (!JsonObject->TryGetStringField(TEXT("target_graph"), OutTargetGraph) || OutTargetGraph.IsEmpty())
    {
        OutTargetGraph = TEXT("EventGraph");
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

FString FConnectBlueprintNodesCommand::CreateSuccessResponse(const TArray<bool>& Results, const TArray<FBlueprintNodeConnectionParams>& Connections) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    
    // Create detailed results array
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (int32 i = 0; i < Results.Num(); i++)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), Results[i]);
        
        if (Results[i] && i < Connections.Num())
        {
            ResultObj->SetStringField(TEXT("source_node_id"), Connections[i].SourceNodeId);
            ResultObj->SetStringField(TEXT("target_node_id"), Connections[i].TargetNodeId);
        }
        else if (!Results[i])
        {
            ResultObj->SetStringField(TEXT("message"), TEXT("Failed to connect nodes"));
        }
        
        ResultsArray.Add(MakeShared<FJsonValueObject>(ResultObj));
    }
    
    ResponseObj->SetArrayField(TEXT("results"), ResultsArray);
    ResponseObj->SetBoolField(TEXT("batch"), true);
    
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

FString FConnectBlueprintNodesCommand::CreateMixedResponse(const TArray<bool>& Results, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    // Add detailed results array
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (int32 i = 0; i < Results.Num(); i++)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), Results[i]);
        ResultObj->SetStringField(TEXT("source_node_id"), Connections[i].SourceNodeId);
        ResultObj->SetStringField(TEXT("source_pin"), Connections[i].SourcePin);
        ResultObj->SetStringField(TEXT("target_node_id"), Connections[i].TargetNodeId);
        ResultObj->SetStringField(TEXT("target_pin"), Connections[i].TargetPin);

        ResultsArray.Add(MakeShared<FJsonValueObject>(ResultObj));
    }
    ResponseObj->SetArrayField(TEXT("connection_results"), ResultsArray);

    // Add summary statistics
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

FString FConnectBlueprintNodesCommand::CreateEnhancedResponse(const TArray<FConnectionResultInfo>& Results, const TArray<FBlueprintNodeConnectionParams>& Connections) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();

    // Count successes and collect warnings
    int32 SuccessfulConnections = 0;
    TArray<TSharedPtr<FJsonValue>> WarningsArray;
    TArray<TSharedPtr<FJsonValue>> AutoInsertedArray;

    // Create detailed results array
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (int32 i = 0; i < Results.Num(); i++)
    {
        const FConnectionResultInfo& Result = Results[i];
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), Result.bSuccess);

        if (Result.bSuccess)
        {
            SuccessfulConnections++;
            ResultObj->SetStringField(TEXT("source_node_id"), Result.SourceNodeId);
            ResultObj->SetStringField(TEXT("target_node_id"), Result.TargetNodeId);

            // Report any auto-inserted nodes
            if (Result.AutoInsertedNodes.Num() > 0)
            {
                for (const FAutoInsertedNodeInfo& AutoNode : Result.AutoInsertedNodes)
                {
                    TSharedPtr<FJsonObject> AutoNodeObj = MakeShared<FJsonObject>();
                    AutoNodeObj->SetStringField(TEXT("node_id"), AutoNode.NodeId);
                    AutoNodeObj->SetStringField(TEXT("title"), AutoNode.NodeTitle);
                    AutoNodeObj->SetStringField(TEXT("type"), AutoNode.NodeType);
                    AutoNodeObj->SetBoolField(TEXT("requires_exec"), AutoNode.bRequiresExecConnection);
                    AutoNodeObj->SetBoolField(TEXT("exec_connected"), AutoNode.bExecConnected);
                    AutoInsertedArray.Add(MakeShared<FJsonValueObject>(AutoNodeObj));

                    // Add warning if cast node has disconnected exec
                    if (AutoNode.bRequiresExecConnection && !AutoNode.bExecConnected)
                    {
                        TSharedPtr<FJsonObject> WarningObj = MakeShared<FJsonObject>();
                        WarningObj->SetStringField(TEXT("type"), TEXT("disconnected_exec"));
                        WarningObj->SetStringField(TEXT("node_id"), AutoNode.NodeId);
                        WarningObj->SetStringField(TEXT("node_title"), AutoNode.NodeTitle);
                        WarningObj->SetStringField(TEXT("message"),
                            FString::Printf(TEXT("Auto-inserted '%s' node has disconnected exec pins - it will NOT execute at runtime. Connect its exec pins or the cast will be skipped."),
                                *AutoNode.NodeTitle));
                        WarningsArray.Add(MakeShared<FJsonValueObject>(WarningObj));
                    }
                }
            }
        }
        else
        {
            ResultObj->SetStringField(TEXT("error"), Result.ErrorMessage.IsEmpty() ? TEXT("Failed to connect nodes") : Result.ErrorMessage);
        }

        ResultsArray.Add(MakeShared<FJsonValueObject>(ResultObj));
    }

    // Determine overall success (all connections succeeded)
    bool bOverallSuccess = (SuccessfulConnections == Results.Num());
    ResponseObj->SetBoolField(TEXT("success"), bOverallSuccess);
    ResponseObj->SetArrayField(TEXT("results"), ResultsArray);
    ResponseObj->SetBoolField(TEXT("batch"), true);
    ResponseObj->SetNumberField(TEXT("successful_connections"), SuccessfulConnections);
    ResponseObj->SetNumberField(TEXT("total_connections"), Results.Num());

    // Add auto-inserted nodes array if any
    if (AutoInsertedArray.Num() > 0)
    {
        ResponseObj->SetArrayField(TEXT("auto_inserted_nodes"), AutoInsertedArray);
    }

    // Add warnings array if any
    if (WarningsArray.Num() > 0)
    {
        ResponseObj->SetArrayField(TEXT("warnings"), WarningsArray);
        ResponseObj->SetBoolField(TEXT("has_warnings"), true);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
