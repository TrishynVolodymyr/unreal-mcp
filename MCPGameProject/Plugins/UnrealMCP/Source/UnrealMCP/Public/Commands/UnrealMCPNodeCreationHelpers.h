#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Dom/JsonObject.h"

/**
 * Helper functions for Blueprint node creation operations
 * Extracted from UnrealMCPBlueprintActionCommands to improve maintainability
 */
class UNREALMCP_API UnrealMCPNodeCreationHelpers
{
public:
    /**
     * Parse JSON parameters with error handling
     * @param JsonParams - JSON string to parse
     * @param OutParamsObject - Parsed JSON object output
     * @param OutResultObj - Result object for error reporting
     * @return true if parsing succeeded or JsonParams was empty
     */
    static bool ParseJsonParameters(const FString& JsonParams, TSharedPtr<FJsonObject>& OutParamsObject, TSharedPtr<FJsonObject>& OutResultObj);

    /**
     * Parse node position from various formats (JSON array, string coordinates)
     * @param NodePosition - Position string in various formats
     * @param OutPositionX - Parsed X coordinate
     * @param OutPositionY - Parsed Y coordinate
     */
    static void ParseNodePosition(const FString& NodePosition, int32& OutPositionX, int32& OutPositionY);

    /**
     * Find target class by name with common class resolution
     * @param ClassName - Class name to find
     * @return Found UClass pointer or nullptr
     */
    static UClass* FindTargetClass(const FString& ClassName);

    /**
     * Build result JSON with node information
     * @param bSuccess - Success status
     * @param Message - Status message
     * @param BlueprintName - Target blueprint name
     * @param FunctionName - Function/node name
     * @param NewNode - Created node (optional)
     * @param NodeTitle - Display title of the node
     * @param NodeType - Type/class of the node
     * @param TargetClass - Target class for function calls
     * @param PositionX - X position
     * @param PositionY - Y position
     * @return JSON string result
     */
    static FString BuildNodeResult(bool bSuccess, const FString& Message, const FString& BlueprintName = TEXT(""), 
                                  const FString& FunctionName = TEXT(""), UEdGraphNode* NewNode = nullptr, 
                                  const FString& NodeTitle = TEXT(""), const FString& NodeType = TEXT(""), 
                                  UClass* TargetClass = nullptr, int32 PositionX = 0, int32 PositionY = 0);

    /**
     * Universal dynamic node creation using Blueprint Action Database
     * @param FunctionName - Name of function/node to create
     * @param EventGraph - Target graph
     * @param PositionX - X position
     * @param PositionY - Y position
     * @param NewNode - Output created node
     * @param NodeTitle - Output node title
     * @param NodeType - Output node type
     * @return true if node was created successfully
     */
    static bool TryCreateNodeUsingBlueprintActionDatabase(const FString& FunctionName, UEdGraph* EventGraph, float PositionX, float PositionY, UEdGraphNode*& NewNode, FString& NodeTitle, FString& NodeType);
}; 