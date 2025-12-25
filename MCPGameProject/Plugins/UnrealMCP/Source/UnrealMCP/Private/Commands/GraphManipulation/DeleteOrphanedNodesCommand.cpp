#include "Commands/GraphManipulation/DeleteOrphanedNodesCommand.h"
#include "Services/IBlueprintService.h"
#include "Utils/GraphUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

FDeleteOrphanedNodesCommand::FDeleteOrphanedNodesCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FDeleteOrphanedNodesCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString GraphName;
    bool bIncludeEventGraph = false;
    bool bExcludeReturnNodes = true;
    FString ParseError;

    if (!ParseParameters(Parameters, BlueprintName, GraphName, bIncludeEventGraph, bExcludeReturnNodes, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Collect graphs to process
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    TArray<UEdGraph*> GraphsToProcess;
    TArray<FString> SkippedGraphs;

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        FString CurrentGraphName = Graph->GetName();

        // If specific graph requested, only process that one
        if (!GraphName.IsEmpty())
        {
            if (CurrentGraphName.Equals(GraphName, ESearchCase::IgnoreCase))
            {
                GraphsToProcess.Add(Graph);
            }
            continue;
        }

        // Skip EventGraph unless explicitly included
        if (!bIncludeEventGraph && CurrentGraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
        {
            SkippedGraphs.Add(TEXT("EventGraph (excluded by default)"));
            continue;
        }

        GraphsToProcess.Add(Graph);
    }

    // Check if specific graph was requested but not found
    if (!GraphName.IsEmpty() && GraphsToProcess.Num() == 0)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Graph '%s' not found in Blueprint '%s'"), *GraphName, *BlueprintName));
    }

    // Collect and delete orphaned nodes
    TArray<FString> DeletedNodeTitles;
    int32 TotalDeleted = 0;

    // Create a transaction for undo support
    const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Orphaned Nodes")));

    for (UEdGraph* Graph : GraphsToProcess)
    {
        TArray<FString> OrphanedNodeIds;
        if (!FGraphUtils::DetectOrphanedNodes(Graph, OrphanedNodeIds))
        {
            continue;
        }

        // Build node ID to node map
        TMap<FString, UEdGraphNode*> NodeIdMap;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node)
            {
                NodeIdMap.Add(FGraphUtils::GetReliableNodeId(Node), Node);
            }
        }

        // Delete each orphaned node
        for (const FString& NodeId : OrphanedNodeIds)
        {
            UEdGraphNode** NodePtr = NodeIdMap.Find(NodeId);
            if (!NodePtr || !*NodePtr)
            {
                continue;
            }

            UEdGraphNode* Node = *NodePtr;

            // Optionally skip auto-generated Return Nodes at (0,0)
            if (bExcludeReturnNodes)
            {
                UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node);
                if (ResultNode && Node->NodePosX == 0 && Node->NodePosY == 0)
                {
                    // Check if it has no connections (truly unused)
                    bool bHasConnections = false;
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->LinkedTo.Num() > 0)
                        {
                            bHasConnections = true;
                            break;
                        }
                    }

                    if (!bHasConnections)
                    {
                        UE_LOG(LogTemp, Display, TEXT("DeleteOrphanedNodes: Skipping auto-generated Return Node at (0,0) in graph '%s'"),
                            *Graph->GetName());
                        continue;
                    }
                }
            }

            // Get node title for logging
            FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            FString FullTitle = FString::Printf(TEXT("%s [%s]"), *NodeTitle, *Graph->GetName());

            // Mark graph and node as modified for undo support
            Graph->Modify();
            Node->Modify();

            // Disconnect all pins before removing
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin)
                {
                    Pin->BreakAllPinLinks();
                }
            }

            // Remove the node
            Graph->RemoveNode(Node);
            DeletedNodeTitles.Add(FullTitle);
            TotalDeleted++;

            UE_LOG(LogTemp, Display, TEXT("DeleteOrphanedNodes: Deleted '%s' from graph '%s'"),
                *NodeTitle, *Graph->GetName());
        }
    }

    // Mark blueprint as modified if we deleted anything
    if (TotalDeleted > 0)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    return CreateSuccessResponse(BlueprintName, TotalDeleted, DeletedNodeTitles, SkippedGraphs);
}

FString FDeleteOrphanedNodesCommand::GetCommandName() const
{
    return TEXT("delete_orphaned_nodes");
}

bool FDeleteOrphanedNodesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, GraphName, Error;
    bool bIncludeEventGraph, bExcludeReturnNodes;
    return ParseParameters(Parameters, BlueprintName, GraphName, bIncludeEventGraph, bExcludeReturnNodes, Error);
}

bool FDeleteOrphanedNodesCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName,
                                                  FString& OutGraphName, bool& OutIncludeEventGraph,
                                                  bool& OutExcludeReturnNodes, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Required: blueprint_name
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }

    // Optional: graph_name (empty = all graphs except EventGraph)
    JsonObject->TryGetStringField(TEXT("graph_name"), OutGraphName);

    // Optional: include_event_graph (default: false)
    OutIncludeEventGraph = false;
    JsonObject->TryGetBoolField(TEXT("include_event_graph"), OutIncludeEventGraph);

    // Optional: exclude_return_nodes (default: true)
    OutExcludeReturnNodes = true;
    JsonObject->TryGetBoolField(TEXT("exclude_return_nodes"), OutExcludeReturnNodes);

    return true;
}

FString FDeleteOrphanedNodesCommand::CreateSuccessResponse(const FString& BlueprintName, int32 DeletedCount,
                                                          const TArray<FString>& DeletedNodeTitles,
                                                          const TArray<FString>& SkippedGraphs) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetNumberField(TEXT("deleted_count"), DeletedCount);

    // Add deleted node titles
    TArray<TSharedPtr<FJsonValue>> DeletedArray;
    for (const FString& Title : DeletedNodeTitles)
    {
        DeletedArray.Add(MakeShared<FJsonValueString>(Title));
    }
    ResponseObj->SetArrayField(TEXT("deleted_nodes"), DeletedArray);

    // Add skipped graphs info
    if (SkippedGraphs.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> SkippedArray;
        for (const FString& Skipped : SkippedGraphs)
        {
            SkippedArray.Add(MakeShared<FJsonValueString>(Skipped));
        }
        ResponseObj->SetArrayField(TEXT("skipped_graphs"), SkippedArray);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FDeleteOrphanedNodesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}
