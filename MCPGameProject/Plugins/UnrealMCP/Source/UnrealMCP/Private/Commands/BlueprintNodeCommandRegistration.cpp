#include "Commands/BlueprintNodeCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/BlueprintNode/ConnectBlueprintNodesCommand.h"
// #include "Commands/BlueprintNode/AddBlueprintInputActionNodeCommand.h"  // REMOVED: Use create_node_by_action_name instead
#include "Commands/BlueprintNode/GetBlueprintGraphsCommand.h"
#include "Commands/BlueprintNode/FindBlueprintNodesCommand.h"
#include "Commands/BlueprintNode/AddBlueprintVariableNodeCommand.h"
#include "Commands/BlueprintNode/GetVariableInfoCommand.h"
#include "Commands/BlueprintNode/AddBlueprintEventNodeCommand.h"
#include "Commands/BlueprintNode/AddBlueprintFunctionNodeCommand.h"
#include "Commands/BlueprintNode/AddBlueprintCustomEventNodeCommand.h"
#include "Commands/BlueprintNode/CreateNodeByActionNameCommand.h"
// #include "Commands/BlueprintNode/AddEnhancedInputActionNodeCommand.h"  // REMOVED: Use create_node_by_action_name instead
#include "Services/BlueprintNodeService.h"
#include "Services/BlueprintActionService.h"

// Static member definition
TArray<FString> FBlueprintNodeCommandRegistration::RegisteredCommandNames;

void FBlueprintNodeCommandRegistration::RegisterAllBlueprintNodeCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintNodeCommandRegistration::RegisterAllBlueprintNodeCommands: Starting Blueprint Node command registration"));
    
    // Clear any existing registrations
    RegisteredCommandNames.Empty();
    
    // Register individual commands
    RegisterConnectBlueprintNodesCommand();
    // RegisterAddBlueprintInputActionNodeCommand();  // REMOVED: Use create_node_by_action_name instead
    RegisterGetBlueprintGraphsCommand();
    RegisterFindBlueprintNodesCommand();
    RegisterAddBlueprintVariableCommand();
    RegisterGetVariableInfoCommand();
    RegisterAddBlueprintEventNodeCommand();
    RegisterAddBlueprintFunctionNodeCommand();
    RegisterAddBlueprintCustomEventNodeCommand();
    RegisterCreateNodeByActionNameCommand();
    // RegisterAddEnhancedInputActionNodeCommand(); // REMOVED: Use create_node_by_action_name instead
    
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
    // Use the singleton instance wrapped in a shared pointer
    TSharedPtr<IBlueprintNodeService> Service = TSharedPtr<IBlueprintNodeService>(&FBlueprintNodeService::Get(), [](IBlueprintNodeService*){});
    TSharedPtr<FConnectBlueprintNodesCommand> Command = MakeShared<FConnectBlueprintNodesCommand>(Service);
    RegisterAndTrackCommand(Command);
}

// REMOVED: Input Action nodes now created via Blueprint Action system
// void FBlueprintNodeCommandRegistration::RegisterAddBlueprintInputActionNodeCommand()
// {
//     TSharedPtr<FAddBlueprintInputActionNodeCommand> Command = MakeShared<FAddBlueprintInputActionNodeCommand>(FBlueprintNodeService::Get());
//     RegisterAndTrackCommand(Command);
// }

void FBlueprintNodeCommandRegistration::RegisterGetBlueprintGraphsCommand()
{
    // Use service layer pattern with singleton service
    TSharedPtr<FGetBlueprintGraphsCommand> Command = MakeShared<FGetBlueprintGraphsCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterFindBlueprintNodesCommand()
{
    // Use service layer pattern with singleton service
    TSharedPtr<FFindBlueprintNodesCommand> Command = MakeShared<FFindBlueprintNodesCommand>(FBlueprintNodeService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintNodeCommandRegistration::RegisterAddBlueprintVariableCommand()
{
    TSharedPtr<FAddBlueprintVariableNodeCommand> Command = MakeShared<FAddBlueprintVariableNodeCommand>(FBlueprintNodeService::Get());
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

void FBlueprintNodeCommandRegistration::RegisterCreateNodeByActionNameCommand()
{
    // Create a new instance of the Blueprint Action Service
    TSharedPtr<IBlueprintActionService> ActionService = MakeShared<FBlueprintActionService>();
    TSharedPtr<FCreateNodeByActionNameCommand> Command = MakeShared<FCreateNodeByActionNameCommand>(ActionService);
    RegisterAndTrackCommand(Command);
}

// REMOVED: Enhanced Input Action nodes now created via Blueprint Action system
// void FBlueprintNodeCommandRegistration::RegisterAddEnhancedInputActionNodeCommand()
// {
//     TSharedPtr<FAddEnhancedInputActionNodeCommand> Command = MakeShared<FAddEnhancedInputActionNodeCommand>(FBlueprintNodeService::Get());
//     RegisterAndTrackCommand(Command);
// }

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
