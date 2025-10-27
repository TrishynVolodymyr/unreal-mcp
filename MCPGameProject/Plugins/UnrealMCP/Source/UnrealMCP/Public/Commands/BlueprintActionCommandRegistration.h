#pragma once

#include "CoreMinimal.h"

class FUnrealMCPCommandRegistry;
class IBlueprintActionService;

/**
 * Registration utility for Blueprint Action commands
 * Handles registration and cleanup of all Blueprint Action-related commands
 */
class UNREALMCP_API FBlueprintActionCommandRegistration
{
public:
    /**
     * Register all Blueprint Action commands with the registry
     * @param Registry - The command registry to register with
     * @param BlueprintActionService - The blueprint action service instance to use
     */
    static void RegisterCommands(FUnrealMCPCommandRegistry& Registry, TSharedPtr<IBlueprintActionService> BlueprintActionService);

    /**
     * Unregister all Blueprint Action commands from the registry
     */
    static void UnregisterAllBlueprintActionCommands();

private:
    /** Array to track registered commands for cleanup */
    static TArray<TSharedPtr<class IUnrealMCPCommand>> RegisteredCommands;

    /**
     * Register and track a command for later cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};