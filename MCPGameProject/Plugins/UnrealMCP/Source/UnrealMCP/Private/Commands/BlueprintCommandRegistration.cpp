#include "Commands/BlueprintCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/Blueprint/CreateBlueprintCommand.h"
#include "Commands/Blueprint/AddComponentToBlueprintCommand.h"
#include "Commands/Blueprint/AddBlueprintVariableCommand.h"
#include "Commands/Blueprint/SetComponentPropertyCommand.h"
#include "Commands/Blueprint/CompileBlueprintCommand.h"
#include "Commands/Blueprint/SetPhysicsPropertiesCommand.h"
#include "Commands/Blueprint/SetBlueprintPropertyCommand.h"
#include "Commands/Blueprint/SetStaticMeshPropertiesCommand.h"
#include "Commands/Blueprint/SetPawnPropertiesCommand.h"
#include "Commands/Blueprint/CallBlueprintFunctionCommand.h"
#include "Commands/Blueprint/CreateBlueprintInterfaceCommand.h"
#include "Commands/Blueprint/AddInterfaceToBlueprintCommand.h"
#include "Commands/Blueprint/CreateCustomBlueprintFunctionCommand.h"
#include "Commands/Blueprint/GetBlueprintMetadataCommand.h"
#include "Commands/Blueprint/ModifyBlueprintFunctionPropertiesCommand.h"
#include "Services/BlueprintService.h"

// Static member definition
TArray<FString> FBlueprintCommandRegistration::RegisteredCommandNames;

void FBlueprintCommandRegistration::RegisterAllBlueprintCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCommandRegistration::RegisterAllBlueprintCommands: Starting Blueprint command registration"));
    
    // Clear any existing registrations
    RegisteredCommandNames.Empty();
    
    // Register individual commands
    RegisterCreateBlueprintCommand();
    RegisterAddComponentToBlueprintCommand();
    RegisterAddBlueprintVariableCommand();
    RegisterSetComponentPropertyCommand();
    RegisterCompileBlueprintCommand();
    RegisterSetPhysicsPropertiesCommand();
    RegisterSetBlueprintPropertyCommand();
    RegisterSetStaticMeshPropertiesCommand();
    RegisterSetPawnPropertiesCommand();
    RegisterCallBlueprintFunctionCommand();
    RegisterCreateBlueprintInterfaceCommand();
    RegisterAddInterfaceToBlueprintCommand();
    RegisterCreateCustomBlueprintFunctionCommand();
    RegisterGetBlueprintMetadataCommand();
    RegisterModifyBlueprintFunctionPropertiesCommand();

    UE_LOG(LogTemp, Log, TEXT("FBlueprintCommandRegistration::RegisterAllBlueprintCommands: Registered %d Blueprint commands"),
        RegisteredCommandNames.Num());
}

void FBlueprintCommandRegistration::UnregisterAllBlueprintCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCommandRegistration::UnregisterAllBlueprintCommands: Starting Blueprint command unregistration"));
    
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
    
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCommandRegistration::UnregisterAllBlueprintCommands: Unregistered %d Blueprint commands"), 
        UnregisteredCount);
}

void FBlueprintCommandRegistration::RegisterCreateBlueprintCommand()
{
    TSharedPtr<FCreateBlueprintCommand> Command = MakeShared<FCreateBlueprintCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterAddComponentToBlueprintCommand()
{
    TSharedPtr<FAddComponentToBlueprintCommand> Command = MakeShared<FAddComponentToBlueprintCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterSetComponentPropertyCommand()
{
    TSharedPtr<FSetComponentPropertyCommand> Command = MakeShared<FSetComponentPropertyCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterCompileBlueprintCommand()
{
    TSharedPtr<FCompileBlueprintCommand> Command = MakeShared<FCompileBlueprintCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterSetPhysicsPropertiesCommand()
{
    TSharedPtr<FSetPhysicsPropertiesCommand> Command = MakeShared<FSetPhysicsPropertiesCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterSetBlueprintPropertyCommand()
{
    TSharedPtr<FSetBlueprintPropertyCommand> Command = MakeShared<FSetBlueprintPropertyCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterSetStaticMeshPropertiesCommand()
{
    TSharedPtr<FSetStaticMeshPropertiesCommand> Command = MakeShared<FSetStaticMeshPropertiesCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterSetPawnPropertiesCommand()
{
    TSharedPtr<FSetPawnPropertiesCommand> Command = MakeShared<FSetPawnPropertiesCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterAddBlueprintVariableCommand()
{
    TSharedPtr<FAddBlueprintVariableCommand> Command = MakeShared<FAddBlueprintVariableCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterCallBlueprintFunctionCommand()
{
    TSharedPtr<FCallBlueprintFunctionCommand> Command = MakeShared<FCallBlueprintFunctionCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterCreateBlueprintInterfaceCommand()
{
    TSharedPtr<FCreateBlueprintInterfaceCommand> Command = MakeShared<FCreateBlueprintInterfaceCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterAddInterfaceToBlueprintCommand()
{
    TSharedPtr<FAddInterfaceToBlueprintCommand> Command = MakeShared<FAddInterfaceToBlueprintCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterCreateCustomBlueprintFunctionCommand()
{
    TSharedPtr<FCreateCustomBlueprintFunctionCommand> Command = MakeShared<FCreateCustomBlueprintFunctionCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterGetBlueprintMetadataCommand()
{
    TSharedPtr<FGetBlueprintMetadataCommand> Command = MakeShared<FGetBlueprintMetadataCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterModifyBlueprintFunctionPropertiesCommand()
{
    TSharedPtr<FModifyBlueprintFunctionPropertiesCommand> Command = MakeShared<FModifyBlueprintFunctionPropertiesCommand>(FBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FBlueprintCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }
    
    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCommandRegistration::RegisterAndTrackCommand: Registered and tracked command '%s'"), *CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FBlueprintCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}
