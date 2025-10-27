#include "Commands/BlueprintActionCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/BlueprintAction/GetActionsForClassCommand.h"
#include "Commands/BlueprintAction/GetActionsForClassHierarchyCommand.h"
#include "Commands/BlueprintAction/GetActionsForPinCommand.h"
#include "Commands/BlueprintAction/SearchBlueprintActionsCommand.h"
#include "Commands/BlueprintAction/GetNodePinInfoCommand.h"
#include "Services/IBlueprintActionService.h"

// Static member definition
TArray<TSharedPtr<IUnrealMCPCommand>> FBlueprintActionCommandRegistration::RegisteredCommands;

void FBlueprintActionCommandRegistration::RegisterCommands(FUnrealMCPCommandRegistry& Registry, TSharedPtr<IBlueprintActionService> BlueprintActionService)
{
    if (!BlueprintActionService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintActionCommandRegistration::RegisterCommands: BlueprintActionService is null"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintActionCommandRegistration::RegisterCommands: Registering Blueprint Action commands"));

    // Register GetActionsForClass command
    TSharedPtr<IUnrealMCPCommand> GetActionsForClassCommand = MakeShared<FGetActionsForClassCommand>(BlueprintActionService);
    RegisterAndTrackCommand(GetActionsForClassCommand);

    // Register GetActionsForClassHierarchy command
    TSharedPtr<IUnrealMCPCommand> GetActionsForClassHierarchyCommand = MakeShared<FGetActionsForClassHierarchyCommand>(BlueprintActionService);
    RegisterAndTrackCommand(GetActionsForClassHierarchyCommand);

    // Register GetActionsForPin command
    TSharedPtr<IUnrealMCPCommand> GetActionsForPinCommand = MakeShared<FGetActionsForPinCommand>(BlueprintActionService);
    RegisterAndTrackCommand(GetActionsForPinCommand);

    // Register SearchBlueprintActions command
    TSharedPtr<IUnrealMCPCommand> SearchBlueprintActionsCommand = MakeShared<FSearchBlueprintActionsCommand>(BlueprintActionService);
    RegisterAndTrackCommand(SearchBlueprintActionsCommand);

    // Register GetNodePinInfo command
    TSharedPtr<IUnrealMCPCommand> GetNodePinInfoCommand = MakeShared<FGetNodePinInfoCommand>(BlueprintActionService);
    RegisterAndTrackCommand(GetNodePinInfoCommand);

    UE_LOG(LogTemp, Log, TEXT("FBlueprintActionCommandRegistration::RegisterCommands: Successfully registered %d Blueprint Action commands"), RegisteredCommands.Num());
}

void FBlueprintActionCommandRegistration::UnregisterAllBlueprintActionCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintActionCommandRegistration::UnregisterAllBlueprintActionCommands: Starting Blueprint Action command unregistration"));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    int32 UnregisteredCount = 0;
    for (const TSharedPtr<IUnrealMCPCommand>& Command : RegisteredCommands)
    {
        if (Command.IsValid())
        {
            FString CommandName = Command->GetCommandName();
            if (Registry.UnregisterCommand(CommandName))
            {
                UnregisteredCount++;
                UE_LOG(LogTemp, Log, TEXT("FBlueprintActionCommandRegistration::UnregisterAllBlueprintActionCommands: Unregistered command: %s"), *CommandName);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("FBlueprintActionCommandRegistration::UnregisterAllBlueprintActionCommands: Failed to unregister command: %s"), *CommandName);
            }
        }
    }
    
    RegisteredCommands.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintActionCommandRegistration::UnregisterAllBlueprintActionCommands: Unregistered %d Blueprint Action commands"), UnregisteredCount);
}

void FBlueprintActionCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintActionCommandRegistration::RegisterAndTrackCommand: Command is null"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommands.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("FBlueprintActionCommandRegistration::RegisterAndTrackCommand: Successfully registered command: %s"), *Command->GetCommandName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintActionCommandRegistration::RegisterAndTrackCommand: Failed to register command: %s"), *Command->GetCommandName());
    }
}
