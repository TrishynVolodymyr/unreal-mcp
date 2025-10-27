#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration system for Editor commands
 * Handles registration and cleanup of all editor-related commands
 */
class UNREALMCP_API FEditorCommandRegistration
{
public:
    /**
     * Register all editor commands with the command registry
     * This should be called during plugin startup
     */
    static void RegisterAllCommands();
    
    /**
     * Unregister all editor commands from the command registry
     * This should be called during plugin shutdown
     */
    static void UnregisterAllCommands();

private:
    /** Array to track registered commands for cleanup */
    static TArray<TSharedPtr<class IUnrealMCPCommand>> RegisteredCommands;
    
    /**
     * Register a command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};
