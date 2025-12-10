#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
struct FEdGraphPinType;
struct FBlueprintNodeConnectionParams;

/**
 * Service for handling Blueprint node connections and automatic type casting
 * Extracted from BlueprintNodeService for better code organization
 */
class UNREALMCP_API FBlueprintNodeConnectionService
{
public:
    /**
     * Get the singleton instance
     */
    static FBlueprintNodeConnectionService& Get();

    /**
     * Connect multiple Blueprint nodes
     * @param Blueprint - Target Blueprint
     * @param Connections - Array of connection parameters
     * @param TargetGraph - Graph name to connect nodes in
     * @param OutResults - Array of results for each connection
     * @return true if all connections succeeded
     */
    bool ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections,
                              const FString& TargetGraph, TArray<bool>& OutResults);

    /**
     * Connect two pins on nodes
     * @param SourceNode - Source node
     * @param SourcePinName - Name of the source pin
     * @param TargetNode - Target node
     * @param TargetPinName - Name of the target pin
     * @return true if connection succeeded
     */
    bool ConnectPins(UEdGraphNode* SourceNode, const FString& SourcePinName,
                    UEdGraphNode* TargetNode, const FString& TargetPinName);

    /**
     * Connect two nodes with automatic cast node creation if types don't match
     * @param Graph - The graph containing the nodes
     * @param SourceNode - Source node
     * @param SourcePinName - Name of the source pin
     * @param TargetNode - Target node
     * @param TargetPinName - Name of the target pin
     * @return true if connection succeeded (with or without cast node)
     */
    bool ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                  UEdGraphNode* TargetNode, const FString& TargetPinName);

    /**
     * Find a node by ID or special type identifiers (for Entry/Exit nodes)
     * @param Graph - The graph to search in
     * @param NodeIdOrType - Node ID (GUID) or special type like "FunctionEntry", "FunctionResult"
     * @return Found node or nullptr
     */
    UEdGraphNode* FindNodeByIdOrType(UEdGraph* Graph, const FString& NodeIdOrType);

private:
    /** Private constructor for singleton pattern */
    FBlueprintNodeConnectionService() = default;

    /**
     * Check if two pin types are compatible or need a cast
     */
    bool ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType);

    /**
     * Create a cast node between two incompatible pins
     */
    bool CreateCastNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create an Integer to String conversion node
     */
    bool CreateIntToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a Float to String conversion node
     */
    bool CreateFloatToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a Boolean to String conversion node
     */
    bool CreateBoolToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a String to Integer conversion node
     */
    bool CreateStringToIntCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a String to Float conversion node
     */
    bool CreateStringToFloatCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create an Object to Object dynamic cast node
     */
    bool CreateObjectCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);
};
