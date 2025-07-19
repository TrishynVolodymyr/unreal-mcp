#include "Commands/BlueprintCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/CreateBlueprintCommand.h"
#include "Commands/AddComponentToBlueprintCommand.h"
#include "Commands/SetComponentPropertyCommand.h"
#include "Commands/CompileBlueprintCommand.h"
#include "Commands/SetPhysicsPropertiesCommand.h"
#include "Commands/SetBlueprintPropertyCommand.h"
#include "Commands/ListBlueprintComponentsCommand.h"
#include "Commands/SetStaticMeshPropertiesCommand.h"
#include "Commands/SetPawnPropertiesCommand.h"
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
    RegisterSetComponentPropertyCommand();
    RegisterCompileBlueprintCommand();
    RegisterSetPhysicsPropertiesCommand();
    RegisterSetBlueprintPropertyCommand();
    RegisterListBlueprintComponentsCommand();
    RegisterSetStaticMeshPropertiesCommand();
    RegisterSetPawnPropertiesCommand();
    
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

void FBlueprintCommandRegistration::RegisterListBlueprintComponentsCommand()
{
    TSharedPtr<FListBlueprintComponentsCommand> Command = MakeShared<FListBlueprintComponentsCommand>(FBlueprintService::Get());
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