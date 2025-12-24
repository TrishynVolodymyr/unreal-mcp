#include "Commands/GraphManipulation/DisconnectNodeCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Utils/GraphUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node.h"

FString FDisconnectNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FDisconnectNodeCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Parse parameters
    FString BlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: blueprint_name"));
    }

    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return CreateErrorResponse(TEXT("Missing required parameter: node_id"));
    }

    FString TargetGraph = TEXT("EventGraph");
    JsonObject->TryGetStringField(TEXT("target_graph"), TargetGraph);

    bool bDisconnectInputs = true;
    bool bDisconnectOutputs = true;
    JsonObject->TryGetBoolField(TEXT("disconnect_inputs"), bDisconnectInputs);
    JsonObject->TryGetBoolField(TEXT("disconnect_outputs"), bDisconnectOutputs);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the graph
    UEdGraph* Graph = nullptr;
    for (UEdGraph* CurrentGraph : Blueprint->UbergraphPages)
    {
        if (CurrentGraph && CurrentGraph->GetName() == TargetGraph)
        {
            Graph = CurrentGraph;
            break;
        }
    }

    // Also check function graphs
    if (!Graph)
    {
        for (UEdGraph* CurrentGraph : Blueprint->FunctionGraphs)
        {
            if (CurrentGraph && CurrentGraph->GetName() == TargetGraph)
            {
                Graph = CurrentGraph;
                break;
            }
        }
    }

    if (!Graph)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *TargetGraph));
    }

    // Find the node
    UEdGraphNode* Node = nullptr;
    for (UEdGraphNode* CurrentNode : Graph->Nodes)
    {
        if (CurrentNode && FGraphUtils::GetReliableNodeId(CurrentNode) == NodeId)
        {
            Node = CurrentNode;
            break;
        }
    }

    if (!Node)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Node not found with ID: %s"), *NodeId));
    }

    // Disconnect pins
    TArray<FString> DisconnectedPins;
    int32 TotalDisconnections = 0;

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        bool bShouldDisconnect = false;
        if (Pin->Direction == EGPD_Input && bDisconnectInputs)
        {
            bShouldDisconnect = true;
        }
        else if (Pin->Direction == EGPD_Output && bDisconnectOutputs)
        {
            bShouldDisconnect = true;
        }

        if (bShouldDisconnect && Pin->LinkedTo.Num() > 0)
        {
            int32 ConnectionCount = Pin->LinkedTo.Num();
            Pin->BreakAllPinLinks();
            TotalDisconnections += ConnectionCount;
            DisconnectedPins.Add(Pin->GetName());
        }
    }

    // Mark blueprint as modified
    Blueprint->Modify();
    Graph->Modify();

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    ResponseObj->SetStringField(TEXT("target_graph"), TargetGraph);
    ResponseObj->SetNumberField(TEXT("total_disconnections"), TotalDisconnections);
    
    TArray<TSharedPtr<FJsonValue>> PinArray;
    for (const FString& PinName : DisconnectedPins)
    {
        PinArray.Add(MakeShareable(new FJsonValueString(PinName)));
    }
    ResponseObj->SetArrayField(TEXT("disconnected_pins"), PinArray);
    
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Disconnected %d connections from node %s"), TotalDisconnections, *NodeId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
