#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"

// Forward declarations
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_Event;
class UBlueprint;

/**
 * Represents a graph warning (disconnected cast exec, orphaned node, etc.)
 */
struct UNREALMCP_API FGraphWarning
{
    FString Type;       // e.g., "disconnected_cast_exec", "orphaned_node"
    FString NodeId;
    FString NodeTitle;
    FString GraphName;
    FString Message;

    FGraphWarning() = default;
    FGraphWarning(const FString& InType, const FString& InNodeId, const FString& InTitle,
                  const FString& InGraph, const FString& InMessage)
        : Type(InType), NodeId(InNodeId), NodeTitle(InTitle), GraphName(InGraph), Message(InMessage) {}
};

/**
 * Blueprint graph manipulation utilities
 * Extracted from UnrealMCPCommonUtils for better code organization
 */
class UNREALMCP_API FGraphUtils
{
public:
    /**
     * Connect two graph nodes by pin names
     * @param Graph - Graph containing the nodes
     * @param SourceNode - Source node
     * @param SourcePinName - Name of source pin
     * @param TargetNode - Target node
     * @param TargetPinName - Name of target pin
     * @return true if connection succeeded
     */
    static bool ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                  UEdGraphNode* TargetNode, const FString& TargetPinName);

    /**
     * Find a pin on a node by name and direction
     * @param Node - Node to search
     * @param PinName - Name of pin to find
     * @param Direction - Pin direction filter (EGPD_MAX for any direction)
     * @return Found pin or nullptr
     */
    static UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction = EGPD_MAX);

    /**
     * Find an existing event node in a graph by event name
     * @param Graph - Graph to search
     * @param EventName - Name of event to find
     * @return Found event node or nullptr
     */
    static UK2Node_Event* FindExistingEventNode(UEdGraph* Graph, const FString& EventName);

    /**
     * Get a reliable unique identifier for a node
     * Uses NodeGuid if valid, otherwise generates an ID from object's unique ID
     * This handles the case where some node types (FunctionEntry, FunctionResult, DynamicCast)
     * may have uninitialized GUIDs.
     * @param Node - The node to get ID for
     * @return String ID that can be used for node operations
     */
    static FString GetReliableNodeId(UEdGraphNode* Node);

    /**
     * Detect graph warnings in a specific graph (cast nodes with disconnected exec, etc.)
     * @param Graph - Graph to check
     * @param OutWarnings - Array to receive warnings
     */
    static void DetectGraphWarnings(UEdGraph* Graph, TArray<FGraphWarning>& OutWarnings);

    /**
     * Detect graph warnings in all graphs of a Blueprint
     * @param Blueprint - Blueprint to check
     * @param OutWarnings - Array to receive warnings
     */
    static void DetectBlueprintWarnings(UBlueprint* Blueprint, TArray<FGraphWarning>& OutWarnings);
};
