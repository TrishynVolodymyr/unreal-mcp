// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Service for creating arithmetic and comparison operator nodes.
 * Handles creation of +, -, *, /, <, >, ==, != and other operator nodes.
 */
class FArithmeticNodeCreator
{
public:
    /**
     * Try to create an arithmetic or comparison operation node.
     * 
     * @param OperationName - Name of the operation ("+", "-", "Add", "Less", etc.)
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param OutNode - Created node (if successful)
     * @param OutTitle - Title of the created node
     * @param OutNodeType - Type of the created node
     * @return True if node was created successfully
     */
    static bool TryCreateArithmeticOrComparisonNode(
        const FString& OperationName,
        UEdGraph* EventGraph,
        int32 PositionX,
        int32 PositionY,
        UEdGraphNode*& OutNode,
        FString& OutTitle,
        FString& OutNodeType
    );
};
