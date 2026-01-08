#pragma once

#include "CoreMinimal.h"

/**
 * Static class responsible for registering all Blueprint-related commands
 * with the command registry system
 */
class UNREALMCP_API FBlueprintCommandRegistration
{
public:
    /**
     * Register all Blueprint commands with the command registry
     * This should be called during module startup
     */
    static void RegisterAllBlueprintCommands();
    
    /**
     * Unregister all Blueprint commands from the command registry
     * This should be called during module shutdown
     */
    static void UnregisterAllBlueprintCommands();

private:
    /** Array of registered command names for cleanup */
    static TArray<FString> RegisteredCommandNames;
    
    /**
     * Register individual Blueprint commands
     */
    static void RegisterCreateBlueprintCommand();
    static void RegisterAddComponentToBlueprintCommand();
    static void RegisterAddBlueprintVariableCommand();
    static void RegisterSetComponentPropertyCommand();
    static void RegisterCompileBlueprintCommand();
    static void RegisterSetPhysicsPropertiesCommand();
    static void RegisterSetBlueprintPropertyCommand();
    static void RegisterSetStaticMeshPropertiesCommand();
    static void RegisterSetPawnPropertiesCommand();
    static void RegisterCallBlueprintFunctionCommand();
    static void RegisterCreateBlueprintInterfaceCommand();
    static void RegisterAddInterfaceToBlueprintCommand();
    static void RegisterCreateCustomBlueprintFunctionCommand();
    static void RegisterGetBlueprintMetadataCommand();
    static void RegisterModifyBlueprintFunctionPropertiesCommand();
    static void RegisterDeleteBlueprintVariableCommand();

    /**
     * Helper to register a command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};
