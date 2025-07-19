#include "Commands/BlueprintNode/ConnectBlueprintNodesCommand.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FConnectBlueprintNodesCommand::FConnectBlueprintNodesCommand(IBlueprintNodeService& InBlueprintNodeService)
    : BlueprintNodeService(InBlueprintNodeService)
{
}

FString FConnectBlueprintNodesCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    TArray<FBlueprintNodeConnectionParams> Connections;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, Connections, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the Blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Connect the nodes using the service
    TArray<bool> Results;
    bool bAllSucceeded = BlueprintNodeService.ConnectBlueprintNodes(Blueprint, Connections, Results);
    
    if (bAllSucceeded)
    {
        return CreateSuccessResponse(Results);
    }
    else
    {
        // Some connections failed, but we still return the results
        return CreateSuccessResponse(Results);
    }
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
    
    // Check for single connection (legacy format)
    FString SourceNodeId, SourcePin, TargetNodeId, TargetPin;
    if (JsonObject->TryGetStringField(TEXT("source_node_id"), SourceNodeId) &&
        JsonObject->TryGetStringField(TEXT("source_pin"), SourcePin) &&
        JsonObject->TryGetStringField(TEXT("target_node_id"), TargetNodeId) &&
        JsonObject->TryGetStringField(TEXT("target_pin"), TargetPin))
    {
        // Single connection format
        FBlueprintNodeConnectionParams Connection;
        Connection.SourceNodeId = SourceNodeId;
        Connection.SourcePin = SourcePin;
        Connection.TargetNodeId = TargetNodeId;
        Connection.TargetPin = TargetPin;
        
        FString ValidationError;
        if (!Connection.IsValid(ValidationError))
        {
            OutError = ValidationError;
            return false;
        }
        
        OutConnections.Add(Connection);
    }
    else
    {
        // Batch connections format
        const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
        if (!JsonObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
        {
            OutError = TEXT("Missing required 'connections' parameter or single connection parameters");
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