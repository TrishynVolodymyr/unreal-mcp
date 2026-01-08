#pragma once

#include "CoreMinimal.h"

/**
 * Static class responsible for registering all Animation Blueprint-related commands
 * with the command registry system
 */
class UNREALMCP_API FAnimationCommandRegistration
{
public:
    /**
     * Register all Animation Blueprint commands with the command registry
     * This should be called during module startup
     */
    static void RegisterAllAnimationCommands();

    /**
     * Unregister all Animation Blueprint commands from the command registry
     * This should be called during module shutdown
     */
    static void UnregisterAllAnimationCommands();

private:
    /** Array of registered command names for cleanup */
    static TArray<FString> RegisteredCommandNames;

    /**
     * Register individual Animation Blueprint commands
     */
    static void RegisterCreateAnimationBlueprintCommand();
    static void RegisterLinkAnimationLayerCommand();
    static void RegisterCreateAnimStateMachineCommand();
    static void RegisterAddAnimStateCommand();
    static void RegisterAddAnimTransitionCommand();
    static void RegisterAddAnimVariableCommand();
    static void RegisterGetAnimBlueprintMetadataCommand();
    static void RegisterConfigureAnimSlotCommand();
    static void RegisterConnectAnimGraphNodesCommand();

    /**
     * Helper to register a command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};
