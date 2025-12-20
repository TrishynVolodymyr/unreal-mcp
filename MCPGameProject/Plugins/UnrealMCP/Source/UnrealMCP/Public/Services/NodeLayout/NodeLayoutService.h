#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

/**
 * Service for Blueprint graph node layout operations.
 * Provides auto-arrangement and layout analysis functionality.
 */
class UNREALMCP_API FNodeLayoutService
{
public:
    // Layout constants
    static constexpr int32 HORIZONTAL_SPACING = 350;   // Space between layers (left-to-right)
    static constexpr int32 VERTICAL_SPACING = 200;     // Space between nodes in same layer (increased)
    static constexpr int32 NODE_WIDTH_ESTIMATE = 200;  // Estimated node width
    static constexpr int32 NODE_HEIGHT_ESTIMATE = 120; // Estimated node height (increased)
    static constexpr int32 PURE_NODE_OFFSET_X = -200;  // Pure nodes offset left of consumer
    static constexpr int32 PURE_NODE_OFFSET_Y = -50;   // Pure nodes offset above consumer
    static constexpr int32 PURE_NODE_VERTICAL_GAP = 140; // Vertical gap between stacked pure nodes

    /**
     * Auto-arrange all nodes in a graph using connection-aware horizontal flow layout.
     * @param Graph The graph to arrange
     * @param OutArrangedCount Number of nodes that were arranged
     * @return True if arrangement was successful
     */
    static bool AutoArrangeNodes(UEdGraph* Graph, int32& OutArrangedCount);

    /**
     * Get layout information for a graph including node positions and overlap detection.
     * @param Graph The graph to analyze
     * @param OutNodePositions Map of node GUID to position
     * @param OutOverlappingPairs Pairs of node GUIDs that overlap
     * @return True if analysis was successful
     */
    static bool GetGraphLayoutInfo(UEdGraph* Graph,
        TMap<FString, FVector2D>& OutNodePositions,
        TArray<TPair<FString, FString>>& OutOverlappingPairs);

private:
    /**
     * Find root nodes (nodes with no incoming execution connections).
     * These are typically Event nodes, entry points, etc.
     */
    static TArray<UEdGraphNode*> FindRootNodes(UEdGraph* Graph);

    /**
     * Find pure nodes (nodes with no execution pins - getters, math, etc.)
     */
    static TArray<UEdGraphNode*> FindPureNodes(UEdGraph* Graph);

    /**
     * Check if a node has any incoming execution connections.
     */
    static bool HasIncomingExecConnection(UEdGraphNode* Node);

    /**
     * Check if a node is a "pure" node (no execution pins).
     */
    static bool IsPureNode(UEdGraphNode* Node);

    /**
     * Assign layer indices to nodes based on their distance from root nodes.
     * @param RootNodes Starting nodes (layer 0)
     * @param OutNodeLayers Map of node to layer index
     */
    static void AssignLayers(const TArray<UEdGraphNode*>& RootNodes,
        TMap<UEdGraphNode*, int32>& OutNodeLayers);

    /**
     * Get nodes connected via outgoing execution pins.
     */
    static TArray<UEdGraphNode*> GetOutgoingExecConnectedNodes(UEdGraphNode* Node);

    /**
     * Get the node that consumes a pure node's output (for positioning pure nodes).
     */
    static UEdGraphNode* GetPureNodeConsumer(UEdGraphNode* PureNode);

    /**
     * Check if two node bounds overlap.
     */
    static bool DoNodeBoundsOverlap(UEdGraphNode* NodeA, UEdGraphNode* NodeB);

    /**
     * Get estimated bounds for a node.
     */
    static FIntRect GetNodeBounds(UEdGraphNode* Node);
};
