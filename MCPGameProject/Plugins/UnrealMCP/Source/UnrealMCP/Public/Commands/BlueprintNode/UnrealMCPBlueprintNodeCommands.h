#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint Node-related MCP commands
 */
class UNREALMCP_API FUnrealMCPBlueprintNodeCommands
{
public:
    FUnrealMCPBlueprintNodeCommands();

    // Handle blueprint node commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Specific blueprint node command handlers
    TSharedPtr<FJsonObject> HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params);
    // TSharedPtr<FJsonObject> HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params);  // REMOVED: Use get_blueprint_metadata with fields=["graph_nodes"] and node_type/event_type filters instead
    TSharedPtr<FJsonObject> HandleAddBlueprintCustomEventNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetVariableInfo(const TSharedPtr<FJsonObject>& Params);
    
    // Enhanced Input specific handlers - REMOVED: Use create_node_by_action_name instead
    // TSharedPtr<FJsonObject> HandleAddEnhancedInputActionNode(const TSharedPtr<FJsonObject>& Params);
}; 
