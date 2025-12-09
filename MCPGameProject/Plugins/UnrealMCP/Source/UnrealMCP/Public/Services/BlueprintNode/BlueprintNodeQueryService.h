#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
struct FBlueprintNodeInfo;

/**
 * Service for querying Blueprint nodes and graphs
 * Extracted from BlueprintNodeService for better code organization
 */
class UNREALMCP_API FBlueprintNodeQueryService
{
public:
    /**
     * Get the singleton instance
     */
    static FBlueprintNodeQueryService& Get();

    /**
     * Find Blueprint nodes by type, event type, or in a specific graph
     * @param Blueprint - Target Blueprint
     * @param NodeType - Type of node to find (empty for all)
     * @param EventType - Event type to filter by (empty for all)
     * @param TargetGraph - Graph to search in (empty for all graphs)
     * @param OutNodeInfos - Array of found node information
     * @return true if search succeeded
     */
    bool FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType,
                           const FString& TargetGraph, TArray<FBlueprintNodeInfo>& OutNodeInfos);

    /**
     * Get all graph names from a Blueprint
     * @param Blueprint - Target Blueprint
     * @param OutGraphNames - Array of graph names
     * @return true if succeeded
     */
    bool GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames);

    /**
     * Get variable information from a Blueprint
     * @param Blueprint - Target Blueprint
     * @param VariableName - Name of the variable
     * @param OutVariableType - Type of the variable
     * @param OutAdditionalInfo - Additional JSON information about the variable
     * @return true if variable found
     */
    bool GetVariableInfo(UBlueprint* Blueprint, const FString& VariableName, FString& OutVariableType,
                        TSharedPtr<FJsonObject>& OutAdditionalInfo);

    /**
     * Find a node by ID in a Blueprint
     * @param Blueprint - Target Blueprint
     * @param NodeId - ID of the node to find
     * @return Found node or nullptr
     */
    UEdGraphNode* FindNodeById(UBlueprint* Blueprint, const FString& NodeId);

    /**
     * Find a graph in a Blueprint by name
     * @param Blueprint - Target Blueprint
     * @param GraphName - Name of the graph to find (empty for EventGraph)
     * @return Found graph or nullptr
     */
    UEdGraph* FindGraphInBlueprint(UBlueprint* Blueprint, const FString& GraphName = TEXT(""));

private:
    /** Private constructor for singleton pattern */
    FBlueprintNodeQueryService() = default;

    /**
     * Generate a unique node ID for tracking
     * @param Node - The node to generate ID for
     * @return Unique node ID string
     */
    FString GenerateNodeId(UEdGraphNode* Node) const;

    /**
     * Clean TypePromotion node titles to show user-friendly names
     * @param Node - The node to get clean title for
     * @param OriginalTitle - Original node title from GetNodeTitle
     * @return Cleaned user-friendly title
     */
    FString GetCleanTypePromotionTitle(UEdGraphNode* Node, const FString& OriginalTitle) const;
};
