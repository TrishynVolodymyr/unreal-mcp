// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Helper class for creating Array operation nodes (GET, LENGTH, etc.).
 *
 * The Blueprint Action Database may return deprecated node types for array operations.
 * This class creates the correct modern node types directly:
 * - UK2Node_GetArrayItem for Array GET operations
 * - UK2Node_CallArrayFunction for Array LENGTH operations
 */
class FArrayNodeCreator
{
public:
    /**
     * Check if the function name is an array GET operation.
     */
    static bool IsArrayGetOperation(const FString& FunctionName, const FString& ClassName);

    /**
     * Check if the function name is an array LENGTH operation.
     */
    static bool IsArrayLengthOperation(const FString& FunctionName, const FString& ClassName);

    /**
     * Try to create an Array GET node (UK2Node_GetArrayItem).
     *
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param NewNode - Created node (if successful)
     * @param NodeTitle - Title of the created node
     * @param NodeType - Type of the created node
     * @return True if node was created successfully
     */
    static bool TryCreateArrayGetNode(
        UEdGraph* EventGraph,
        float PositionX,
        float PositionY,
        UEdGraphNode*& NewNode,
        FString& NodeTitle,
        FString& NodeType
    );

    /**
     * Try to create an Array LENGTH node (UK2Node_CallArrayFunction).
     *
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param NewNode - Created node (if successful)
     * @param NodeTitle - Title of the created node
     * @param NodeType - Type of the created node
     * @return True if node was created successfully
     */
    static bool TryCreateArrayLengthNode(
        UEdGraph* EventGraph,
        float PositionX,
        float PositionY,
        UEdGraphNode*& NewNode,
        FString& NodeTitle,
        FString& NodeType
    );
};
