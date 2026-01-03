#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration system for Niagara VFX commands
 * Handles registration and cleanup of all Niagara-related commands
 */
class UNREALMCP_API FNiagaraCommandRegistration
{
public:
    /**
     * Register all Niagara commands with the command registry
     * This should be called during plugin startup
     */
    static void RegisterAllCommands();

    /**
     * Unregister all Niagara commands from the command registry
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
