#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

/**
 * Main node creation functions for Blueprint node operations
 * Contains the core CreateNodeByActionName function and universal dynamic creation
 */
class UNREALMCP_API UnrealMCPNodeCreators
{
public:
    /**
     * Creates a Blueprint node by action name using multiple strategies
     * This is the main entry point for node creation
     * 
     * @param BlueprintName - Name of the target Blueprint
     * @param FunctionName - Name of the function/node to create
     * @param ClassName - Optional class name for function resolution
     * @param NodePosition - Position in the graph as JSON array [x, y]
     * @param JsonParams - Additional parameters as JSON string
     * @return JSON result string with node creation details
     */
    static FString CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, 
                                         const FString& ClassName, const FString& NodePosition, 
                                         const FString& JsonParams);
}; 