// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Helper class for creating Enhanced Input Action nodes.
 */
class FEnhancedInputActionNodeCreator
{
public:
    /**
     * Try to create an Enhanced Input Action event node.
     * Searches the Asset Registry for Input Action assets and creates UK2Node_EnhancedInputAction.
     *
     * @param ActionName - Name of the Input Action to create a node for (e.g., "IA_Jump")
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param NewNode - Created node (if successful)
     * @param NodeTitle - Title of the created node
     * @param NodeType - Type of the created node
     * @return True if node was created successfully
     */
    static bool TryCreateEnhancedInputActionNode(
        const FString& ActionName,
        UEdGraph* EventGraph,
        float PositionX,
        float PositionY,
        UEdGraphNode*& NewNode,
        FString& NodeTitle,
        FString& NodeType
    );
};
