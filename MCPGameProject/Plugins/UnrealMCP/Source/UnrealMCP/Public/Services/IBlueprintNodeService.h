#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Dom/JsonObject.h"

/**
 * Information about a Blueprint node
 */
struct UNREALMCP_API FBlueprintNodeInfo
{
    /** Node ID (GUID) */
    FString NodeId;
    
    /** Node title/name */
    FString NodeTitle;
    
    /** Default constructor */
    FBlueprintNodeInfo() = default;
    
    /**
     * Constructor with parameters
     * @param InNodeId - The node ID
     * @param InNodeTitle - The node title
     */
    FBlueprintNodeInfo(const FString& InNodeId, const FString& InNodeTitle)
        : NodeId(InNodeId), NodeTitle(InNodeTitle)
    {
    }
};

/**
 * Parameters for Blueprint node connection operations
 */
struct UNREALMCP_API FBlueprintNodeConnectionParams
{
    /** Source node ID */
    FString SourceNodeId;
    
    /** Source pin name */
    FString SourcePin;
    
    /** Target node ID */
    FString TargetNodeId;
    
    /** Target pin name */
    FString TargetPin;
    
    /** Default constructor */
    FBlueprintNodeConnectionParams() = default;
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for Blueprint node creation operations
 */
struct UNREALMCP_API FBlueprintNodeCreationParams
{
    /** Name of the Blueprint */
    FString BlueprintName;
    
    /** Position in the graph */
    FVector2D Position;
    
    /** Additional parameters as JSON */
    TSharedPtr<FJsonObject> AdditionalParams;
    
    /** Default constructor */
    FBlueprintNodeCreationParams() = default;
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const;
};

/**
 * Abstract service interface for Blueprint node operations
 * This interface provides all the functionality needed to manage Blueprint nodes
 */
class UNREALMCP_API IBlueprintNodeService
{
public:
    virtual ~IBlueprintNodeService() = default;
    
    /**
     * Connect two Blueprint nodes
     * @param Blueprint - Target Blueprint
     * @param Connections - Array of connection parameters (supports batch connections)
     * @param TargetGraph - Name of the graph to search for nodes (defaults to "EventGraph")
     * @param OutResults - Results for each connection attempt
     * @return true if all connections succeeded
     */
    virtual bool ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& TargetGraph, TArray<bool>& OutResults) = 0;
    
    /**
     * Add an input action node to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param ActionName - Name of the input action
     * @param Position - Position in the graph
     * @param OutNodeId - ID of the created node
     * @return true if node was created successfully
     */
    virtual bool AddInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId) = 0;
    
    /**
     * Get list of all available graphs in a Blueprint
     * @param Blueprint - Target Blueprint
     * @param OutGraphNames - Array of graph names
     * @return true if operation was successful
     */
    virtual bool GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames) = 0;
    
    /**
     * Find nodes in a Blueprint's event graph
     * @param Blueprint - Target Blueprint
     * @param NodeType - Optional type of node to find
     * @param EventType - Optional specific event type to find
     * @param TargetGraph - Optional specific graph to search in
     * @param OutNodeInfos - Array of found node information (ID and title)
     * @return true if search completed successfully
     */
    virtual bool FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FBlueprintNodeInfo>& OutNodeInfos) = 0;
    
    /**
     * Add a variable node to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param VariableName - Name of the variable
     * @param bIsGetter - Whether this is a getter (true) or setter (false) node
     * @param Position - Position in the graph
     * @param OutNodeId - ID of the created node
     * @return true if node was created successfully
     */
    virtual bool AddVariableNode(UBlueprint* Blueprint, const FString& VariableName, bool bIsGetter, const FVector2D& Position, FString& OutNodeId) = 0;
    
    /**
     * Get variable information from a Blueprint
     * @param Blueprint - Target Blueprint
     * @param VariableName - Name of the variable to query
     * @param OutVariableType - Type of the variable
     * @param OutAdditionalInfo - Additional variable information
     * @return true if variable information was retrieved successfully
     */
    virtual bool GetVariableInfo(UBlueprint* Blueprint, const FString& VariableName, FString& OutVariableType, TSharedPtr<FJsonObject>& OutAdditionalInfo) = 0;
    
    /**
     * Add an event node to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param EventType - Type of event to add
     * @param Position - Position in the graph
     * @param OutNodeId - ID of the created node
     * @return true if node was created successfully
     */
    virtual bool AddEventNode(UBlueprint* Blueprint, const FString& EventType, const FVector2D& Position, FString& OutNodeId) = 0;
    
    /**
     * Add a function call node to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param FunctionName - Name of the function to call
     * @param ClassName - Optional class name for the function
     * @param Position - Position in the graph
     * @param OutNodeId - ID of the created node
     * @return true if node was created successfully
     */
    virtual bool AddFunctionCallNode(UBlueprint* Blueprint, const FString& FunctionName, const FString& ClassName, const FVector2D& Position, FString& OutNodeId) = 0;
    
    /**
     * Add a custom event node to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param EventName - Name of the custom event
     * @param Position - Position in the graph
     * @param OutNodeId - ID of the created node
     * @return true if node was created successfully
     */
    virtual bool AddCustomEventNode(UBlueprint* Blueprint, const FString& EventName, const FVector2D& Position, FString& OutNodeId) = 0;
    
    /**
     * Add an Enhanced Input action node to a Blueprint
     * @param Blueprint - Target Blueprint
     * @param ActionName - Name of the Enhanced Input action
     * @param Position - Position in the graph
     * @param OutNodeId - ID of the created node
     * @return true if node was created successfully
     */
    // virtual bool AddEnhancedInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId) = 0;  // REMOVED: Use create_node_by_action_name instead
};