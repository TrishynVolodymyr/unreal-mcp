#include "Services/NodeLayout/NodeLayoutService.h"
#include "Utils/GraphUtils.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_CustomEvent.h"

bool FNodeLayoutService::AutoArrangeNodes(UEdGraph* Graph, int32& OutArrangedCount)
{
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("FNodeLayoutService::AutoArrangeNodes: Invalid graph"));
        return false;
    }

    OutArrangedCount = 0;

    // Get all nodes
    TArray<UEdGraphNode*> AllNodes = Graph->Nodes;
    if (AllNodes.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("FNodeLayoutService::AutoArrangeNodes: Graph has no nodes"));
        return true;
    }

    // Separate pure nodes from execution nodes
    TArray<UEdGraphNode*> PureNodes = FindPureNodes(Graph);
    TSet<UEdGraphNode*> PureNodeSet(PureNodes);

    // Find root nodes (events, entry points)
    TArray<UEdGraphNode*> RootNodes = FindRootNodes(Graph);

    // If no root nodes found, use leftmost nodes as starting points
    if (RootNodes.Num() == 0)
    {
        int32 MinX = TNumericLimits<int32>::Max();
        for (UEdGraphNode* Node : AllNodes)
        {
            if (Node && !PureNodeSet.Contains(Node))
            {
                if (Node->NodePosX < MinX)
                {
                    MinX = Node->NodePosX;
                    RootNodes.Empty();
                    RootNodes.Add(Node);
                }
                else if (Node->NodePosX == MinX)
                {
                    RootNodes.Add(Node);
                }
            }
        }
    }

    // Assign layers to execution nodes
    TMap<UEdGraphNode*, int32> NodeLayers;
    AssignLayers(RootNodes, NodeLayers);

    // Add any unvisited non-pure nodes to layer 0
    for (UEdGraphNode* Node : AllNodes)
    {
        if (Node && !PureNodeSet.Contains(Node) && !NodeLayers.Contains(Node))
        {
            NodeLayers.Add(Node, 0);
        }
    }

    // Group nodes by layer
    TMap<int32, TArray<UEdGraphNode*>> LayerGroups;
    for (auto& Pair : NodeLayers)
    {
        LayerGroups.FindOrAdd(Pair.Value).Add(Pair.Key);
    }

    // Sort layers
    TArray<int32> SortedLayers;
    LayerGroups.GetKeys(SortedLayers);
    SortedLayers.Sort();

    // Position execution nodes by layer
    for (int32 Layer : SortedLayers)
    {
        TArray<UEdGraphNode*>& NodesInLayer = LayerGroups[Layer];

        // Sort nodes in layer by their original Y position for stability
        NodesInLayer.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) {
            return A.NodePosY < B.NodePosY;
        });

        int32 X = Layer * HORIZONTAL_SPACING;
        int32 Y = 0;

        for (UEdGraphNode* Node : NodesInLayer)
        {
            if (Node)
            {
                Node->NodePosX = X;
                Node->NodePosY = Y;
                Y += VERTICAL_SPACING;
                OutArrangedCount++;
            }
        }
    }

    // Position pure nodes near their consumers
    // Track how many pure nodes have been placed per consumer to avoid overlaps
    TMap<UEdGraphNode*, int32> ConsumerPureNodeCount;
    int32 UnconnectedPureNodeCount = 0;

    for (UEdGraphNode* PureNode : PureNodes)
    {
        if (!PureNode) continue;

        UEdGraphNode* Consumer = GetPureNodeConsumer(PureNode);
        if (Consumer)
        {
            // Get the index for this consumer (how many pure nodes already placed for it)
            int32& PureIndex = ConsumerPureNodeCount.FindOrAdd(Consumer);

            // Position pure node to the left of consumer, stacked vertically
            // Each subsequent pure node for the same consumer is offset further up
            PureNode->NodePosX = Consumer->NodePosX + PURE_NODE_OFFSET_X;
            PureNode->NodePosY = Consumer->NodePosY + PURE_NODE_OFFSET_Y - (PureIndex * PURE_NODE_VERTICAL_GAP);

            PureIndex++;
        }
        else
        {
            // No consumer found, place at origin area, stacked vertically
            PureNode->NodePosX = -200;
            PureNode->NodePosY = UnconnectedPureNodeCount * PURE_NODE_VERTICAL_GAP;
            UnconnectedPureNodeCount++;
        }
        OutArrangedCount++;
    }

    // Mark graph as modified
    Graph->NotifyGraphChanged();

    UE_LOG(LogTemp, Log, TEXT("FNodeLayoutService::AutoArrangeNodes: Arranged %d nodes in %d layers"),
        OutArrangedCount, SortedLayers.Num());

    return true;
}

bool FNodeLayoutService::GetGraphLayoutInfo(UEdGraph* Graph,
    TMap<FString, FVector2D>& OutNodePositions,
    TArray<TPair<FString, FString>>& OutOverlappingPairs)
{
    if (!Graph)
    {
        return false;
    }

    OutNodePositions.Empty();
    OutOverlappingPairs.Empty();

    // Collect node positions
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node)
        {
            FString NodeId = FGraphUtils::GetReliableNodeId(Node);
            OutNodePositions.Add(NodeId, FVector2D(Node->NodePosX, Node->NodePosY));
        }
    }

    // Detect overlapping nodes
    TArray<UEdGraphNode*> Nodes = Graph->Nodes;
    for (int32 i = 0; i < Nodes.Num(); i++)
    {
        for (int32 j = i + 1; j < Nodes.Num(); j++)
        {
            if (Nodes[i] && Nodes[j] && DoNodeBoundsOverlap(Nodes[i], Nodes[j]))
            {
                OutOverlappingPairs.Add(TPair<FString, FString>(
                    FGraphUtils::GetReliableNodeId(Nodes[i]),
                    FGraphUtils::GetReliableNodeId(Nodes[j])
                ));
            }
        }
    }

    return true;
}

TArray<UEdGraphNode*> FNodeLayoutService::FindRootNodes(UEdGraph* Graph)
{
    TArray<UEdGraphNode*> RootNodes;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node) continue;

        // Check if it's an event or entry node
        bool bIsEventNode = Node->IsA<UK2Node_Event>() ||
                           Node->IsA<UK2Node_FunctionEntry>() ||
                           Node->IsA<UK2Node_CustomEvent>();

        if (bIsEventNode)
        {
            RootNodes.Add(Node);
            continue;
        }

        // Check if it has execution pins but no incoming exec connections
        bool bHasExecPins = false;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                bHasExecPins = true;
                break;
            }
        }

        if (bHasExecPins && !HasIncomingExecConnection(Node))
        {
            RootNodes.Add(Node);
        }
    }

    return RootNodes;
}

TArray<UEdGraphNode*> FNodeLayoutService::FindPureNodes(UEdGraph* Graph)
{
    TArray<UEdGraphNode*> PureNodes;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && IsPureNode(Node))
        {
            PureNodes.Add(Node);
        }
    }

    return PureNodes;
}

bool FNodeLayoutService::HasIncomingExecConnection(UEdGraphNode* Node)
{
    if (!Node) return false;

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin &&
            Pin->Direction == EGPD_Input &&
            Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
            Pin->LinkedTo.Num() > 0)
        {
            return true;
        }
    }

    return false;
}

bool FNodeLayoutService::IsPureNode(UEdGraphNode* Node)
{
    if (!Node) return false;

    // Check if node has any execution pins
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            return false; // Has exec pin, not pure
        }
    }

    return true; // No exec pins, it's pure
}

void FNodeLayoutService::AssignLayers(const TArray<UEdGraphNode*>& RootNodes,
    TMap<UEdGraphNode*, int32>& OutNodeLayers)
{
    OutNodeLayers.Empty();

    // BFS to assign layers
    TQueue<TPair<UEdGraphNode*, int32>> Queue;
    TSet<UEdGraphNode*> Visited;

    // Add root nodes at layer 0
    for (UEdGraphNode* Root : RootNodes)
    {
        if (Root)
        {
            Queue.Enqueue(TPair<UEdGraphNode*, int32>(Root, 0));
            Visited.Add(Root);
        }
    }

    while (!Queue.IsEmpty())
    {
        TPair<UEdGraphNode*, int32> Current;
        Queue.Dequeue(Current);

        UEdGraphNode* Node = Current.Key;
        int32 Layer = Current.Value;

        OutNodeLayers.Add(Node, Layer);

        // Get connected nodes via execution pins
        TArray<UEdGraphNode*> ConnectedNodes = GetOutgoingExecConnectedNodes(Node);
        for (UEdGraphNode* Connected : ConnectedNodes)
        {
            if (Connected && !Visited.Contains(Connected))
            {
                Visited.Add(Connected);
                Queue.Enqueue(TPair<UEdGraphNode*, int32>(Connected, Layer + 1));
            }
        }
    }
}

TArray<UEdGraphNode*> FNodeLayoutService::GetOutgoingExecConnectedNodes(UEdGraphNode* Node)
{
    TArray<UEdGraphNode*> ConnectedNodes;

    if (!Node) return ConnectedNodes;

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin &&
            Pin->Direction == EGPD_Output &&
            Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                if (LinkedPin && LinkedPin->GetOwningNode())
                {
                    ConnectedNodes.AddUnique(LinkedPin->GetOwningNode());
                }
            }
        }
    }

    return ConnectedNodes;
}

UEdGraphNode* FNodeLayoutService::GetPureNodeConsumer(UEdGraphNode* PureNode)
{
    if (!PureNode) return nullptr;

    // Find the first node that uses any of this pure node's output pins
    for (UEdGraphPin* Pin : PureNode->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                if (LinkedPin && LinkedPin->GetOwningNode())
                {
                    return LinkedPin->GetOwningNode();
                }
            }
        }
    }

    return nullptr;
}

bool FNodeLayoutService::DoNodeBoundsOverlap(UEdGraphNode* NodeA, UEdGraphNode* NodeB)
{
    if (!NodeA || !NodeB) return false;

    FIntRect BoundsA = GetNodeBounds(NodeA);
    FIntRect BoundsB = GetNodeBounds(NodeB);

    return BoundsA.Intersect(BoundsB);
}

FIntRect FNodeLayoutService::GetNodeBounds(UEdGraphNode* Node)
{
    if (!Node)
    {
        return FIntRect();
    }

    // Use node's actual width/height if available, otherwise use estimates
    int32 Width = (Node->NodeWidth > 0) ? Node->NodeWidth : NODE_WIDTH_ESTIMATE;
    int32 Height = (Node->NodeHeight > 0) ? Node->NodeHeight : NODE_HEIGHT_ESTIMATE;

    return FIntRect(
        FIntPoint(Node->NodePosX, Node->NodePosY),
        FIntPoint(Node->NodePosX + Width, Node->NodePosY + Height)
    );
}
