// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Service for creating native property getter/setter nodes.
 * Handles creation of variable access nodes for native properties.
 */
class FNativePropertyNodeCreator
{
public:
    /**
     * Try to create a native property getter or setter node.
     * 
     * @param VarName - Name of the variable/property
     * @param bIsGetter - True for getter, false for setter
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param OutNode - Created node (if successful)
     * @param OutTitle - Title of the created node
     * @param OutNodeType - Type of the created node
     * @return True if node was created successfully
     */
    static bool TryCreateNativePropertyNode(
        const FString& VarName,
        bool bIsGetter,
        UEdGraph* EventGraph,
        int32 PositionX,
        int32 PositionY,
        UEdGraphNode*& OutNode,
        FString& OutTitle,
        FString& OutNodeType
    );
};
