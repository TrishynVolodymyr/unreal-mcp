#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration class for Sound-related MCP commands
 * Handles registration of all sound/audio commands with the command registry
 */
class UNREALMCP_API FSoundCommandRegistration
{
public:
    /**
     * Register all sound commands with the command registry
     * Called during plugin initialization
     */
    static void RegisterAllCommands();

    /**
     * Unregister all sound commands
     * Called during plugin shutdown
     */
    static void UnregisterAllCommands();

private:
    /**
     * Helper to register and track a command
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command);

    /** Array of registered commands for cleanup */
    static TArray<TSharedPtr<IUnrealMCPCommand>> RegisteredCommands;
};
