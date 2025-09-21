#pragma once

#include "CoreMinimal.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Concrete implementation of Blueprint Node service operations
 * Handles Blueprint node creation, connection, and management
 */
class UNREALMCP_API FBlueprintNodeService : public IBlueprintNodeService
{
public:
    /**
     * Get the singleton instance of the Blueprint Node service
     * @return Reference to the singleton instance
     */
    static FBlueprintNodeService& Get();
    
    // IBlueprintNodeService interface
    virtual bool ConnectBlueprintNodes(UBlueprint* Blueprint, const TArray<FBlueprintNodeConnectionParams>& Connections, const FString& TargetGraph, TArray<bool>& OutResults) override;
    virtual bool AddInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames) override;
    virtual bool FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FString>& OutNodeIds) override;
    virtual bool AddVariableNode(UBlueprint* Blueprint, const FString& VariableName, bool bIsGetter, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool GetVariableInfo(UBlueprint* Blueprint, const FString& VariableName, FString& OutVariableType, TSharedPtr<FJsonObject>& OutAdditionalInfo) override;
    virtual bool AddEventNode(UBlueprint* Blueprint, const FString& EventType, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool AddFunctionCallNode(UBlueprint* Blueprint, const FString& FunctionName, const FString& ClassName, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool AddCustomEventNode(UBlueprint* Blueprint, const FString& EventName, const FVector2D& Position, FString& OutNodeId) override;
    // virtual bool AddEnhancedInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId) override;  // REMOVED: Use create_node_by_action_name instead

private:
    /** Private constructor for singleton pattern */
    FBlueprintNodeService() = default;
    
    /**
     * Find a graph in a Blueprint by name
     * @param Blueprint - Target Blueprint
     * @param GraphName - Name of the graph to find (empty for EventGraph)
     * @return Found graph or nullptr
     */
    UEdGraph* FindGraphInBlueprint(UBlueprint* Blueprint, const FString& GraphName = TEXT("")) const;
    
    /**
     * Generate a unique node ID for tracking
     * @param Node - The node to generate ID for
     * @return Unique node ID string
     */
    FString GenerateNodeId(UEdGraphNode* Node) const;
    
    /**
     * Find a node by ID in a Blueprint
     * @param Blueprint - Target Blueprint
     * @param NodeId - ID of the node to find
     * @return Found node or nullptr
     */
    UEdGraphNode* FindNodeById(UBlueprint* Blueprint, const FString& NodeId) const;
    
    /**
     * Connect two pins on nodes
     * @param SourceNode - Source node
     * @param SourcePinName - Name of the source pin
     * @param TargetNode - Target node
     * @param TargetPinName - Name of the target pin
     * @return true if connection succeeded
     */
    bool ConnectPins(UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName) const;
    
    /**
     * Connect two nodes with automatic cast node creation if types don't match
     * @param Graph - The graph containing the nodes
     * @param SourceNode - Source node
     * @param SourcePinName - Name of the source pin
     * @param TargetNode - Target node
     * @param TargetPinName - Name of the target pin
     * @return true if connection succeeded (with or without cast node)
     */
    bool ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName);
    
    /**
     * Check if two pin types are compatible or need a cast
     * @param SourcePinType - Type of the source pin
     * @param TargetPinType - Type of the target pin
     * @return true if types are compatible without cast
     */
    bool ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType) const;
    
    /**
     * Create a cast node between two incompatible pins
     * @param Graph - The graph to create the cast node in
     * @param SourcePin - Source pin
     * @param TargetPin - Target pin
     * @return true if cast node was created and connected successfully
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
};