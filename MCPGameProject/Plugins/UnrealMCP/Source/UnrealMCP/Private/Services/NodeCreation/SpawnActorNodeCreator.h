// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Helper class for creating Spawn Actor from Class nodes.
 */
class FSpawnActorNodeCreator
{
public:
    /**
     * Check if the function name is a Spawn Actor from Class request.
     *
     * @param FunctionName - The requested function name
     * @return True if this is a Spawn Actor from Class request
     */
    static bool IsSpawnActorFromClassRequest(const FString& FunctionName);

    /**
     * Try to create a Spawn Actor from Class node (UK2Node_SpawnActorFromClass).
     *
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param NewNode - Created node (if successful)
     * @param NodeTitle - Title of the created node
     * @param NodeType - Type of the created node
     * @return True if node was created successfully
     */
    static bool TryCreateSpawnActorFromClassNode(
        UEdGraph* EventGraph,
        float PositionX,
        float PositionY,
        UEdGraphNode*& NewNode,
        FString& NodeTitle,
        FString& NodeType
    );
};
