#include "Commands/AnimationCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/Animation/CreateAnimationBlueprintCommand.h"
#include "Commands/Animation/LinkAnimationLayerCommand.h"
#include "Commands/Animation/CreateAnimStateMachineCommand.h"
#include "Commands/Animation/AddAnimStateCommand.h"
#include "Commands/Animation/AddAnimTransitionCommand.h"
#include "Commands/Animation/AddAnimVariableCommand.h"
#include "Commands/Animation/GetAnimBlueprintMetadataCommand.h"
#include "Commands/Animation/ConfigureAnimSlotCommand.h"
#include "Commands/Animation/ConnectAnimGraphNodesCommand.h"
#include "Services/AnimationBlueprintService.h"

// Static member definition
TArray<FString> FAnimationCommandRegistration::RegisteredCommandNames;

void FAnimationCommandRegistration::RegisterAllAnimationCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FAnimationCommandRegistration::RegisterAllAnimationCommands: Starting Animation command registration"));

    // Clear any existing registrations
    RegisteredCommandNames.Empty();

    // Register individual commands
    RegisterCreateAnimationBlueprintCommand();
    RegisterLinkAnimationLayerCommand();
    RegisterCreateAnimStateMachineCommand();
    RegisterAddAnimStateCommand();
    RegisterAddAnimTransitionCommand();
    RegisterAddAnimVariableCommand();
    RegisterGetAnimBlueprintMetadataCommand();
    RegisterConfigureAnimSlotCommand();
    RegisterConnectAnimGraphNodesCommand();

    UE_LOG(LogTemp, Log, TEXT("FAnimationCommandRegistration::RegisterAllAnimationCommands: Registered %d Animation commands"),
        RegisteredCommandNames.Num());
}

void FAnimationCommandRegistration::UnregisterAllAnimationCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FAnimationCommandRegistration::UnregisterAllAnimationCommands: Starting Animation command unregistration"));

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

    UE_LOG(LogTemp, Log, TEXT("FAnimationCommandRegistration::UnregisterAllAnimationCommands: Unregistered %d Animation commands"),
        UnregisteredCount);
}

void FAnimationCommandRegistration::RegisterCreateAnimationBlueprintCommand()
{
    TSharedPtr<FCreateAnimationBlueprintCommand> Command = MakeShared<FCreateAnimationBlueprintCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterLinkAnimationLayerCommand()
{
    TSharedPtr<FLinkAnimationLayerCommand> Command = MakeShared<FLinkAnimationLayerCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterCreateAnimStateMachineCommand()
{
    TSharedPtr<FCreateAnimStateMachineCommand> Command = MakeShared<FCreateAnimStateMachineCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterAddAnimStateCommand()
{
    TSharedPtr<FAddAnimStateCommand> Command = MakeShared<FAddAnimStateCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterAddAnimTransitionCommand()
{
    TSharedPtr<FAddAnimTransitionCommand> Command = MakeShared<FAddAnimTransitionCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterAddAnimVariableCommand()
{
    TSharedPtr<FAddAnimVariableCommand> Command = MakeShared<FAddAnimVariableCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterGetAnimBlueprintMetadataCommand()
{
    TSharedPtr<FGetAnimBlueprintMetadataCommand> Command = MakeShared<FGetAnimBlueprintMetadataCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterConfigureAnimSlotCommand()
{
    TSharedPtr<FConfigureAnimSlotCommand> Command = MakeShared<FConfigureAnimSlotCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterConnectAnimGraphNodesCommand()
{
    TSharedPtr<FConnectAnimGraphNodesCommand> Command = MakeShared<FConnectAnimGraphNodesCommand>(FAnimationBlueprintService::Get());
    RegisterAndTrackCommand(Command);
}

void FAnimationCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }

    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
        UE_LOG(LogTemp, Verbose, TEXT("FAnimationCommandRegistration::RegisterAndTrackCommand: Registered and tracked command '%s'"), *CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}
