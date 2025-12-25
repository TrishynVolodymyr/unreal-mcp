#include "Commands/GraphManipulationCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/GraphManipulation/DisconnectNodeCommand.h"
#include "Commands/GraphManipulation/DeleteNodeCommand.h"
#include "Commands/GraphManipulation/ReplaceNodeCommand.h"
#include "Commands/GraphManipulation/SetNodePinValueCommand.h"
#include "Commands/GraphManipulation/AutoArrangeNodesCommand.h"
#include "Commands/GraphManipulation/DeleteOrphanedNodesCommand.h"
#include "Services/BlueprintNodeService.h"
#include "Services/BlueprintService.h"

// Static member definition
TArray<FString> FGraphManipulationCommandRegistration::RegisteredCommandNames;

void FGraphManipulationCommandRegistration::RegisterAllGraphManipulationCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FGraphManipulationCommandRegistration::RegisterAllGraphManipulationCommands: Starting Graph Manipulation command registration"));
    
    // Clear any existing registrations
    RegisteredCommandNames.Empty();
    
    // Register individual commands
    RegisterDisconnectNodeCommand();
    RegisterDeleteNodeCommand();
    RegisterReplaceNodeCommand();
    RegisterSetNodePinValueCommand();
    RegisterAutoArrangeNodesCommand();
    RegisterDeleteOrphanedNodesCommand();

    UE_LOG(LogTemp, Log, TEXT("FGraphManipulationCommandRegistration::RegisterAllGraphManipulationCommands: Registered %d Graph Manipulation commands"), 
        RegisteredCommandNames.Num());
}

void FGraphManipulationCommandRegistration::UnregisterAllGraphManipulationCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FGraphManipulationCommandRegistration::UnregisterAllGraphManipulationCommands: Starting Graph Manipulation command unregistration"));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    int32 UnregisteredCount = 0;
    for (const FString& CommandName : RegisteredCommandNames)
    {
        if (Registry.UnregisterCommand(CommandName))
        {
            UnregisteredCount++;
        }
    }
    
    RegisteredCommandNames.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("FGraphManipulationCommandRegistration::UnregisterAllGraphManipulationCommands: Unregistered %d Graph Manipulation commands"), 
        UnregisteredCount);
}

void FGraphManipulationCommandRegistration::RegisterDisconnectNodeCommand()
{
    TSharedPtr<FDisconnectNodeCommand> Command = MakeShared<FDisconnectNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FGraphManipulationCommandRegistration::RegisterDeleteNodeCommand()
{
    TSharedPtr<FDeleteNodeCommand> Command = MakeShared<FDeleteNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FGraphManipulationCommandRegistration::RegisterReplaceNodeCommand()
{
    TSharedPtr<FReplaceNodeCommand> Command = MakeShared<FReplaceNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FGraphManipulationCommandRegistration::RegisterSetNodePinValueCommand()
{
    TSharedPtr<FSetNodePinValueCommand> Command = MakeShared<FSetNodePinValueCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FGraphManipulationCommandRegistration::RegisterAutoArrangeNodesCommand()
{
    TSharedPtr<FAutoArrangeNodesCommand> Command = MakeShared<FAutoArrangeNodesCommand>();
    RegisterAndTrackCommand(Command);
}

void FGraphManipulationCommandRegistration::RegisterDeleteOrphanedNodesCommand()
{
    TSharedPtr<FDeleteOrphanedNodesCommand> Command = MakeShared<FDeleteOrphanedNodesCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FGraphManipulationCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FGraphManipulationCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }
    
    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FGraphManipulationCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
        UE_LOG(LogTemp, Verbose, TEXT("FGraphManipulationCommandRegistration::RegisterAndTrackCommand: Registered and tracked command '%s'"), *CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FGraphManipulationCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}
