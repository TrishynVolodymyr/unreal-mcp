#include "Utils/GraphUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_VariableGet.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Serialization/JsonSerializer.h"

bool FGraphUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                   UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }

    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);

    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }

    return false;
}

UEdGraphPin* FGraphUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }

    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"),
           *PinName, (int32)Direction, *Node->GetName());

    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"),
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }

    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }

    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) &&
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }

    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

UK2Node_Event* FGraphUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

FString FGraphUtils::GetReliableNodeId(UEdGraphNode* Node)
{
    if (!Node)
    {
        return TEXT("00000000000000000000000000000000");
    }

    // Check if NodeGuid is valid (not all zeros)
    if (Node->NodeGuid.IsValid())
    {
        return Node->NodeGuid.ToString();
    }

    // NodeGuid is invalid - generate a stable ID from the object's unique ID
    // This ensures we get a unique identifier even for nodes with uninitialized GUIDs
    // Format: "OBJID_" followed by the hex representation of the unique ID
    uint32 UniqueId = Node->GetUniqueID();
    return FString::Printf(TEXT("OBJID_%08X%08X%08X%08X"), UniqueId, UniqueId ^ 0xDEADBEEF, UniqueId ^ 0xCAFEBABE, UniqueId ^ 0x12345678);
}

void FGraphUtils::DetectGraphWarnings(UEdGraph* Graph, TArray<FGraphWarning>& OutWarnings)
{
    if (!Graph)
    {
        return;
    }

    FString GraphName = Graph->GetName();

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Check for Cast nodes (K2Node_DynamicCast) with disconnected exec pins
        FString NodeClassName = Node->GetClass()->GetName();
        if (NodeClassName.Contains(TEXT("DynamicCast")))
        {
            bool bHasExecInput = false;
            bool bHasExecOutput = false;

            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                {
                    if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
                    {
                        bHasExecInput = true;
                    }
                    else if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                    {
                        bHasExecOutput = true;
                    }
                }
            }

            // Warn if cast node has disconnected exec pins
            if (!bHasExecInput || !bHasExecOutput)
            {
                FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                FString Message = FString::Printf(
                    TEXT("Cast node '%s' has disconnected exec pins - it will NOT execute at runtime"),
                    *NodeTitle);

                OutWarnings.Add(FGraphWarning(
                    TEXT("disconnected_cast_exec"),
                    GetReliableNodeId(Node),
                    NodeTitle,
                    GraphName,
                    Message
                ));
            }
        }
    }
}

void FGraphUtils::DetectBlueprintWarnings(UBlueprint* Blueprint, TArray<FGraphWarning>& OutWarnings)
{
    if (!Blueprint)
    {
        return;
    }

    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        DetectGraphWarnings(Graph, OutWarnings);
    }
}

bool FGraphUtils::IsEntryPoint(UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    // Check for standard entry point types
    return Node->IsA<UK2Node_Event>() ||
           Node->IsA<UK2Node_FunctionEntry>() ||
           Node->IsA<UK2Node_CustomEvent>();
}

bool FGraphUtils::IsPureNode(UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    // A pure node has no execution pins
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            return false;
        }
    }

    return true;
}

void FGraphUtils::TraceExecutionFlow(const TArray<UEdGraphNode*>& EntryPoints, TSet<UEdGraphNode*>& OutReachableNodes)
{
    OutReachableNodes.Empty();

    // BFS to trace execution flow
    TQueue<UEdGraphNode*> Queue;

    // Add all entry points to the queue
    for (UEdGraphNode* Entry : EntryPoints)
    {
        if (Entry)
        {
            Queue.Enqueue(Entry);
            OutReachableNodes.Add(Entry);
        }
    }

    while (!Queue.IsEmpty())
    {
        UEdGraphNode* Current;
        Queue.Dequeue(Current);

        // Follow all output execution pins
        for (UEdGraphPin* Pin : Current->Pins)
        {
            if (Pin &&
                Pin->Direction == EGPD_Output &&
                Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (LinkedPin)
                    {
                        UEdGraphNode* ConnectedNode = LinkedPin->GetOwningNode();
                        if (ConnectedNode && !OutReachableNodes.Contains(ConnectedNode))
                        {
                            OutReachableNodes.Add(ConnectedNode);
                            Queue.Enqueue(ConnectedNode);
                        }
                    }
                }
            }
        }
    }
}

void FGraphUtils::TraceDataDependencies(const TSet<UEdGraphNode*>& ExecReachableNodes, TSet<UEdGraphNode*>& OutDataDependencies)
{
    OutDataDependencies.Empty();

    // For each exec-reachable node, trace backward through data pins
    TQueue<UEdGraphNode*> Queue;
    TSet<UEdGraphNode*> Visited;

    // Start with all exec-reachable nodes
    for (UEdGraphNode* Node : ExecReachableNodes)
    {
        Queue.Enqueue(Node);
        Visited.Add(Node);
    }

    while (!Queue.IsEmpty())
    {
        UEdGraphNode* Current;
        Queue.Dequeue(Current);

        // Follow all input data pins (not exec pins)
        for (UEdGraphPin* Pin : Current->Pins)
        {
            if (Pin &&
                Pin->Direction == EGPD_Input &&
                Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (LinkedPin)
                    {
                        UEdGraphNode* ConnectedNode = LinkedPin->GetOwningNode();
                        if (ConnectedNode && !Visited.Contains(ConnectedNode))
                        {
                            Visited.Add(ConnectedNode);
                            // Add to dependencies if it's a pure node
                            // (non-pure nodes should already be in ExecReachableNodes if reachable)
                            if (IsPureNode(ConnectedNode))
                            {
                                OutDataDependencies.Add(ConnectedNode);
                                // Continue tracing from this pure node
                                Queue.Enqueue(ConnectedNode);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool FGraphUtils::DetectOrphanedNodes(UEdGraph* Graph, TArray<FString>& OutOrphanedNodeIds)
{
    OutOrphanedNodeIds.Empty();

    if (!Graph)
    {
        return false;
    }

    // Step 1: Find all entry points
    TArray<UEdGraphNode*> EntryPoints;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && IsEntryPoint(Node))
        {
            EntryPoints.Add(Node);
        }
    }

    // If no entry points found, we can't determine orphans
    // (this might be a macro library or other special graph type)
    if (EntryPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("DetectOrphanedNodes: No entry points found in graph '%s'"), *Graph->GetName());
        return true;
    }

    // Step 2: Trace execution flow forward from entry points
    TSet<UEdGraphNode*> ExecReachableNodes;
    TraceExecutionFlow(EntryPoints, ExecReachableNodes);

    // Step 3: Trace data dependencies backward from exec-reachable nodes
    TSet<UEdGraphNode*> DataDependencies;
    TraceDataDependencies(ExecReachableNodes, DataDependencies);

    // Step 4: Combine all reachable nodes
    TSet<UEdGraphNode*> AllReachableNodes = ExecReachableNodes;
    AllReachableNodes.Append(DataDependencies);

    // Step 5: Find orphaned nodes (not in reachable set)
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Skip comment nodes - they're not executable but not "orphaned"
        FString NodeClassName = Node->GetClass()->GetName();
        if (NodeClassName.Contains(TEXT("Comment")))
        {
            continue;
        }

        if (!AllReachableNodes.Contains(Node))
        {
            OutOrphanedNodeIds.Add(GetReliableNodeId(Node));
        }
    }

    UE_LOG(LogTemp, Display, TEXT("DetectOrphanedNodes: Graph '%s' - %d entry points, %d exec-reachable, %d data deps, %d orphaned"),
        *Graph->GetName(), EntryPoints.Num(), ExecReachableNodes.Num(), DataDependencies.Num(), OutOrphanedNodeIds.Num());

    return true;
}

bool FGraphUtils::GetOrphanedNodesInfo(UEdGraph* Graph, TArray<TSharedPtr<FJsonObject>>& OutOrphanedNodes)
{
    OutOrphanedNodes.Empty();

    if (!Graph)
    {
        return false;
    }

    // Get orphaned node IDs
    TArray<FString> OrphanedNodeIds;
    if (!DetectOrphanedNodes(Graph, OrphanedNodeIds))
    {
        return false;
    }

    // Build a map of node IDs for quick lookup
    TMap<FString, UEdGraphNode*> NodeIdMap;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            NodeIdMap.Add(GetReliableNodeId(Node), Node);
        }
    }

    // Build info for each orphaned node
    for (const FString& NodeId : OrphanedNodeIds)
    {
        UEdGraphNode** NodePtr = NodeIdMap.Find(NodeId);
        if (NodePtr && *NodePtr)
        {
            UEdGraphNode* Node = *NodePtr;

            TSharedPtr<FJsonObject> NodeInfo = MakeShared<FJsonObject>();
            NodeInfo->SetStringField(TEXT("node_id"), NodeId);
            NodeInfo->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
            NodeInfo->SetStringField(TEXT("class"), Node->GetClass()->GetName());
            NodeInfo->SetNumberField(TEXT("pos_x"), Node->NodePosX);
            NodeInfo->SetNumberField(TEXT("pos_y"), Node->NodePosY);

            // Count connections (for debugging - shows if it's connected to other orphans)
            int32 InputConnections = 0;
            int32 OutputConnections = 0;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin)
                {
                    if (Pin->Direction == EGPD_Input)
                    {
                        InputConnections += Pin->LinkedTo.Num();
                    }
                    else
                    {
                        OutputConnections += Pin->LinkedTo.Num();
                    }
                }
            }
            NodeInfo->SetNumberField(TEXT("input_connections"), InputConnections);
            NodeInfo->SetNumberField(TEXT("output_connections"), OutputConnections);

            OutOrphanedNodes.Add(NodeInfo);
        }
    }

    return true;
}
