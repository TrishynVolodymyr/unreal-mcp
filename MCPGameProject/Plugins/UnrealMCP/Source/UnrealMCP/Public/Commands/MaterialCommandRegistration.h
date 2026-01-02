#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration system for Material commands
 * Handles registration and cleanup of all material-related commands
 */
class UNREALMCP_API FMaterialCommandRegistration
{
public:
    /**
     * Register all material commands with the command registry
     * This should be called during plugin startup
     */
    static void RegisterAllCommands();

    /**
     * Unregister all material commands from the command registry
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
