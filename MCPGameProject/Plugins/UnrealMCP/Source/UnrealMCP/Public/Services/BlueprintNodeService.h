#pragma once

#include "CoreMinimal.h"
#include "Services/IBlueprintNodeService.h"
#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Services/BlueprintNode/BlueprintNodeQueryService.h"

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
    virtual bool ConnectNodesWithAutoCast(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, UEdGraphNode* TargetNode, const FString& TargetPinName) override;
    virtual bool AddInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool GetBlueprintGraphs(UBlueprint* Blueprint, TArray<FString>& OutGraphNames) override;
    virtual bool FindBlueprintNodes(UBlueprint* Blueprint, const FString& NodeType, const FString& EventType, const FString& TargetGraph, TArray<FBlueprintNodeInfo>& OutNodeInfos) override;
    virtual bool AddVariableNode(UBlueprint* Blueprint, const FString& VariableName, bool bIsGetter, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool GetVariableInfo(UBlueprint* Blueprint, const FString& VariableName, FString& OutVariableType, TSharedPtr<FJsonObject>& OutAdditionalInfo) override;
    virtual bool AddEventNode(UBlueprint* Blueprint, const FString& EventType, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool AddFunctionCallNode(UBlueprint* Blueprint, const FString& FunctionName, const FString& ClassName, const FVector2D& Position, FString& OutNodeId) override;
    virtual bool AddCustomEventNode(UBlueprint* Blueprint, const FString& EventName, const FVector2D& Position, FString& OutNodeId) override;
    // virtual bool AddEnhancedInputActionNode(UBlueprint* Blueprint, const FString& ActionName, const FVector2D& Position, FString& OutNodeId) override;  // REMOVED: Use create_node_by_action_name instead

private:
    /** Private constructor for singleton pattern */
    FBlueprintNodeService() = default;
};