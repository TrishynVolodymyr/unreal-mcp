// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Service for creating Blueprint nodes using the Blueprint Action Database.
 * This is the main dynamic node creation system that discovers available functions.
 */
class FBlueprintActionDatabaseNodeCreator
{
public:
    /**
     * Try to create a node using Blueprint Action Database.
     * This is the universal node creation method that searches through all available Blueprint actions.
     * 
     * @param FunctionName - Name of the function to create a node for
     * @param ClassName - Optional class name to disambiguate functions with the same name
     * @param EventGraph - Graph to create the node in
     * @param PositionX - X position in the graph
     * @param PositionY - Y position in the graph
     * @param NewNode - Created node (if successful)
     * @param NodeTitle - Title of the created node
     * @param NodeType - Type of the created node
     * @param OutErrorMessage - Optional output parameter for detailed error message
     * @param OutWarningMessage - Optional output parameter for warning message (e.g., WidgetBlueprintLibrary usage)
     * @return True if node was created successfully
     */
    static bool TryCreateNodeUsingBlueprintActionDatabase(
        const FString& FunctionName,
        const FString& ClassName,
        UEdGraph* EventGraph,
        float PositionX,
        float PositionY,
        UEdGraphNode*& NewNode,
        FString& NodeTitle,
        FString& NodeType,
        FString* OutErrorMessage = nullptr,
        FString* OutWarningMessage = nullptr
    );
};
