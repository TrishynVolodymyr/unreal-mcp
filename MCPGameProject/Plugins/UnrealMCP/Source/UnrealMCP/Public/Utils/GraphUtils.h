#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"

// Forward declarations
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_Event;

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
};
