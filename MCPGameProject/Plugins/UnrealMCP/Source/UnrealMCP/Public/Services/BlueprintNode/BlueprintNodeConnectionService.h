#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"

// Forward declarations
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
struct FEdGraphPinType;
struct FBlueprintNodeConnectionParams;

/**
 * Information about an auto-inserted node during connection
 */
struct UNREALMCP_API FAutoInsertedNodeInfo
{
    FString NodeId;
    FString NodeTitle;
    FString NodeType;  // e.g., "K2Node_DynamicCast", "K2Node_CallFunction"
    bool bRequiresExecConnection = false;  // True for Cast nodes
    bool bExecConnected = false;  // Whether exec pins are properly connected

    FAutoInsertedNodeInfo() = default;
    FAutoInsertedNodeInfo(const FString& InNodeId, const FString& InTitle, const FString& InType, bool bNeedsExec)
        : NodeId(InNodeId), NodeTitle(InTitle), NodeType(InType), bRequiresExecConnection(bNeedsExec), bExecConnected(false) {}
};

/**
 * Enhanced connection result with detailed info about what happened
 */
struct UNREALMCP_API FConnectionResultInfo
{
    bool bSuccess = false;
    FString SourceNodeId;
    FString TargetNodeId;
    TArray<FAutoInsertedNodeInfo> AutoInsertedNodes;
    FString ErrorMessage;

    bool HasWarnings() const
    {
        for (const FAutoInsertedNodeInfo& Node : AutoInsertedNodes)
        {
            if (Node.bRequiresExecConnection && !Node.bExecConnected)
            {
                return true;
            }
        }
        return false;
    }
};

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
     * Connect multiple Blueprint nodes (legacy version for backward compatibility)
     * @param Blueprint - Target Blueprint
     * @param Connections - Array of connection parameters
     * @param TargetGraph - Graph name to connect nodes in
     * @param OutResults - Array of results for each connection
     * @return true if all connections succeeded
     */
    bool ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections,
                              const FString& TargetGraph, TArray<bool>& OutResults);

    /**
     * Connect multiple Blueprint nodes with enhanced result information
     * Reports auto-inserted nodes (casts, conversions) and their exec pin status
     * @param Blueprint - Target Blueprint
     * @param Connections - Array of connection parameters
     * @param TargetGraph - Graph name to connect nodes in
     * @param OutResults - Array of enhanced results for each connection
     * @return true if all connections succeeded
     */
    bool ConnectBlueprintNodesEnhanced(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections,
                                       const FString& TargetGraph, TArray<FConnectionResultInfo>& OutResults);

    /**
     * Check if two pins can be connected (uses Unreal's schema validation)
     * This is the same validation Unreal uses in the UI when you try to connect pins.
     * @param SourcePin - Source pin (output)
     * @param TargetPin - Target pin (input)
     * @return FPinConnectionResponse containing whether connection is allowed and the reason
     */
    FPinConnectionResponse CanConnectPins(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Check if two pins can be connected by node and pin names
     * @param SourceNode - Source node
     * @param SourcePinName - Name of the source pin
     * @param TargetNode - Target node
     * @param TargetPinName - Name of the target pin
     * @param OutResponse - Response with details about why connection failed (if applicable)
     * @return true if pins can be connected
     */
    bool CanConnectPinsByName(UEdGraphNode* SourceNode, const FString& SourcePinName,
                              UEdGraphNode* TargetNode, const FString& TargetPinName,
                              FPinConnectionResponse& OutResponse);

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
     * @param OutAutoInsertedNodes - Optional array to receive info about auto-inserted nodes
     * @param OutErrorMessage - Optional string to receive detailed error message on failure
     * @return true if connection succeeded (with or without cast node)
     */
    bool ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                  UEdGraphNode* TargetNode, const FString& TargetPinName,
                                  TArray<FAutoInsertedNodeInfo>* OutAutoInsertedNodes = nullptr,
                                  FString* OutErrorMessage = nullptr);

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
    bool CreateObjectCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin,
                         FAutoInsertedNodeInfo* OutNodeInfo = nullptr);
};
