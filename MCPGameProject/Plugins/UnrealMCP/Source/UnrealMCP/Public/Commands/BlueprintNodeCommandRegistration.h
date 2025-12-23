#pragma once

#include "CoreMinimal.h"

/**
 * Static class responsible for registering all Blueprint Node-related commands
 * with the command registry system
 */
class UNREALMCP_API FBlueprintNodeCommandRegistration
{
public:
    /**
     * Register all Blueprint Node commands with the command registry
     * This should be called during module startup
     */
    static void RegisterAllBlueprintNodeCommands();
    
    /**
     * Unregister all Blueprint Node commands from the command registry
     * This should be called during module shutdown
     */
    static void UnregisterAllBlueprintNodeCommands();

private:
    /** Array of registered command names for cleanup */
    static TArray<FString> RegisteredCommandNames;
    
    /**
     * Register individual Blueprint Node commands
     */
    static void RegisterConnectBlueprintNodesCommand();
    // static void RegisterAddBlueprintInputActionNodeCommand();  // REMOVED: Use create_node_by_action_name instead
    // static void RegisterGetBlueprintGraphsCommand();  // REMOVED: Use get_blueprint_metadata with fields=["graphs"] instead
    // static void RegisterFindBlueprintNodesCommand();  // REMOVED: Use get_blueprint_metadata with fields=["graph_nodes"] and node_type/event_type filters instead
    static void RegisterAddBlueprintVariableCommand();
    static void RegisterGetVariableInfoCommand();
    static void RegisterAddBlueprintEventNodeCommand();
    static void RegisterAddBlueprintFunctionNodeCommand();
    static void RegisterAddBlueprintCustomEventNodeCommand();
    static void RegisterCreateNodeByActionNameCommand();
    // static void RegisterAddEnhancedInputActionNodeCommand();  // REMOVED: Use create_node_by_action_name instead
    
    /**
     * Helper to register a command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};