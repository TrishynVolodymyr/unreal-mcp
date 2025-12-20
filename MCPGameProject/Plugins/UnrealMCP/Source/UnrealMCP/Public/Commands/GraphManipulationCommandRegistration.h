#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Registration class for graph manipulation commands.
 * These are low-level commands for direct graph node manipulation.
 */
class FGraphManipulationCommandRegistration
{
public:
    /** Register all graph manipulation commands */
    static void RegisterAllGraphManipulationCommands();
    
    /** Unregister all graph manipulation commands */
    static void UnregisterAllGraphManipulationCommands();

private:
    /** Register individual commands */
    static void RegisterDisconnectNodeCommand();
    static void RegisterDeleteNodeCommand();
    static void RegisterReplaceNodeCommand();
    static void RegisterSetNodePinValueCommand();
    static void RegisterAutoArrangeNodesCommand();
    
    /** Helper to register and track a command */
    static void RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command);
    
    /** Array to track registered command names for cleanup */
    static TArray<FString> RegisteredCommandNames;
};
