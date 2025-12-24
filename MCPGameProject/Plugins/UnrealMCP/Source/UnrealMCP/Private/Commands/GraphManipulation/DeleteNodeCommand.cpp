#include "Commands/GraphManipulation/DeleteNodeCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Utils/GraphUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node.h"

FString FDeleteNodeCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FDeleteNodeCommand::Execute(const FString& Parameters)
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
    UEdGraphNode* NodeToDelete = nullptr;
    for (UEdGraphNode* CurrentNode : Graph->Nodes)
    {
        if (CurrentNode && FGraphUtils::GetReliableNodeId(CurrentNode) == NodeId)
        {
            NodeToDelete = CurrentNode;
            break;
        }
    }

    if (!NodeToDelete)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Node not found with ID: %s"), *NodeId));
    }

    // Store node info before deletion
    FString NodeTitle = NodeToDelete->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

    // Break all pin links before deletion
    for (UEdGraphPin* Pin : NodeToDelete->Pins)
    {
        if (Pin)
        {
            Pin->BreakAllPinLinks();
        }
    }

    // Mark as modified
    Blueprint->Modify();
    Graph->Modify();

    // Remove the node from the graph
    Graph->RemoveNode(NodeToDelete);

    // Create success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("node_id"), NodeId);
    ResponseObj->SetStringField(TEXT("node_title"), NodeTitle);
    ResponseObj->SetStringField(TEXT("target_graph"), TargetGraph);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted node '%s' from graph '%s'"), *NodeTitle, *TargetGraph));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}
