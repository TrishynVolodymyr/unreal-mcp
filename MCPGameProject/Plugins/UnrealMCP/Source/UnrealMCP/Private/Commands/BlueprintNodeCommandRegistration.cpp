#include "Commands/BlueprintNodeCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/BlueprintNode/ConnectBlueprintNodesCommand.h"
#include "Commands/BlueprintNode/AddBlueprintInputActionNodeCommand.h"
#include "Commands/BlueprintNode/FindBlueprintNodesCommand.h"
#include "Commands/BlueprintNode/AddBlueprintVariableCommand.h"
#include "Commands/BlueprintNode/GetVariableInfoCommand.h"
#include "Commands/BlueprintNode/AddBlueprintEventNodeCommand.h"
#include "Commands/BlueprintNode/AddBlueprintFunctionNodeCommand.h"
#include "Commands/BlueprintNode/AddBlueprintCustomEventNodeCommand.h"
#include "Commands/BlueprintNode/AddEnhancedInputActionNodeCommand.h"
#include "Services/BlueprintNodeService.h"

// Static member definition
TArray<FString> FBlueprintNodeCommandRegistration::RegisteredCommandNames;

void FBlueprintNodeCommandRegistration::RegisterAllBlueprintNodeCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintNodeCommandRegistration::RegisterAllBlueprintNodeCommands: Starting Blueprint Node command registration"));
    
    // Clear any existing registrations
    RegisteredCommandNames.Empty();
    
    // Register individual commands
    RegisterConnectBlueprintNodesCommand();
    RegisterAddBlueprintInputActionNodeCommand();
    RegisterFindBlueprintNodesCommand();
    RegisterAddBlueprintVariableCommand();
    RegisterGetVariableInfoCommand();
    RegisterAddBlueprintEventNodeCommand();
    RegisterAddBlueprintFunctionNodeCommand();
    RegisterAddBlueprintCustomEventNodeCommand();
    RegisterAddEnhancedInputActionNodeCommand();
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintNodeCommandRegistration::RegisterAllBlueprintNodeCommands: Registered %d Blueprint Node commands"), 
        RegisteredCommandNames.Num());
}

void FBlueprintNodeCommandRegistration::UnregisterAllBlueprintNodeCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintNodeCommandRegistration::UnregisterAllBlueprintNodeCommands: Starting Blueprint Node command unregistration"));
    
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
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintNodeCommandRegistration::UnregisterAllBlueprintNodeCommands: Unregistered %d Blueprint Node commands"), 
        UnregisteredCount);
}

void FBlueprintNodeCommandRegistration::RegisterConnectBlueprintNodesCommand()
{
    TSharedPtr<FConnectBlueprintNodesCommand> Command = MakeShared<FConnectBlueprintNodesCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddBlueprintInputActionNodeCommand()
{
    TSharedPtr<FAddBlueprintInputActionNodeCommand> Command = MakeShared<FAddBlueprintInputActionNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterFindBlueprintNodesCommand()
{
    TSharedPtr<FFindBlueprintNodesCommand> Command = MakeShared<FFindBlueprintNodesCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddBlueprintVariableCommand()
{
    TSharedPtr<FAddBlueprintVariableCommand> Command = MakeShared<FAddBlueprintVariableCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterGetVariableInfoCommand()
{
    TSharedPtr<FGetVariableInfoCommand> Command = MakeShared<FGetVariableInfoCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddBlueprintEventNodeCommand()
{
    TSharedPtr<FAddBlueprintEventNodeCommand> Command = MakeShared<FAddBlueprintEventNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddBlueprintFunctionNodeCommand()
{
    TSharedPtr<FAddBlueprintFunctionNodeCommand> Command = MakeShared<FAddBlueprintFunctionNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddBlueprintCustomEventNodeCommand()
{
    TSharedPtr<FAddBlueprintCustomEventNodeCommand> Command = MakeShared<FAddBlueprintCustomEventNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddEnhancedInputActionNodeCommand()
{
    TSharedPtr<FAddEnhancedInputActionNodeCommand> Command = MakeShared<FAddEnhancedInputActionNodeCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintNodeCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }
    
    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintNodeCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintNodeCommandRegistration::RegisterAndTrackCommand: Registered and tracked command '%s'"), *CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintNodeCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}