#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration class for DataTable-related MCP commands
 * Handles registration and cleanup of all DataTable commands
 */
class UNREALMCP_API FDataTableCommandRegistration
{
public:
    /**
     * Register all DataTable commands with the command registry
     */
    static void RegisterAllCommands();
    
    /**
     * Unregister all DataTable commands from the command registry
     */
    static void UnregisterAllCommands();

private:
    /**
     * Register a single command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command);
    
    /** Array of registered commands for cleanup */
    static TArray<TSharedPtr<IUnrealMCPCommand>> RegisteredCommands;
};
