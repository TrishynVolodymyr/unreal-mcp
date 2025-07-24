#include "Commands/UMGCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/UMG/CreateWidgetBlueprintCommand.h"
#include "Commands/UMG/BindWidgetEventCommand.h"
#include "Commands/UMG/AddWidgetComponentCommand.h"
#include "Commands/UMG/SetWidgetPropertyCommand.h"
#include "Services/UMG/UMGService.h"

// Static member definition
TArray<FString> FUMGCommandRegistration::RegisteredCommandNames;

void FUMGCommandRegistration::RegisterAllUMGCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FUMGCommandRegistration::RegisterAllUMGCommands: Starting UMG command registration"));
    
    // Clear any existing registrations
    RegisteredCommandNames.Empty();
    
    // Register existing implemented commands
    RegisterCreateWidgetBlueprintCommand();
    RegisterBindWidgetEventCommand();
    RegisterAddWidgetComponentCommand();
    RegisterSetWidgetPropertyCommand();
    
    // TODO: Register remaining 24 UMG commands when their classes are implemented
    // For now, we'll register the core commands that exist
    
    UE_LOG(LogTemp, Log, TEXT("FUMGCommandRegistration::RegisterAllUMGCommands: Registered %d UMG commands"), 
        RegisteredCommandNames.Num());
}

void FUMGCommandRegistration::UnregisterAllUMGCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FUMGCommandRegistration::UnregisterAllUMGCommands: Starting UMG command unregistration"));
    
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
    
    UE_LOG(LogTemp, Log, TEXT("FUMGCommandRegistration::UnregisterAllUMGCommands: Unregistered %d UMG commands"), 
        UnregisteredCount);
}

void FUMGCommandRegistration::RegisterCreateWidgetBlueprintCommand()
{
    TSharedPtr<FCreateWidgetBlueprintCommand> Command = MakeShared<FCreateWidgetBlueprintCommand>(FUMGService::Get());
    RegisterAndTrackCommand(Command);
}

void FUMGCommandRegistration::RegisterBindWidgetEventCommand()
{
    TSharedPtr<FBindWidgetEventCommand> Command = MakeShared<FBindWidgetEventCommand>(FUMGService::Get());
    RegisterAndTrackCommand(Command);
}

void FUMGCommandRegistration::RegisterAddWidgetComponentCommand()
{
    TSharedPtr<FAddWidgetComponentCommand> Command = MakeShared<FAddWidgetComponentCommand>(FUMGService::Get());
    RegisterAndTrackCommand(Command);
}

void FUMGCommandRegistration::RegisterSetWidgetPropertyCommand()
{
    TSharedPtr<FSetWidgetPropertyCommand> Command = MakeShared<FSetWidgetPropertyCommand>(FUMGService::Get());
    RegisterAndTrackCommand(Command);
}

// Placeholder implementations for remaining commands - these will need actual command classes
void FUMGCommandRegistration::RegisterSetTextBlockBindingCommand()
{
    // TODO: Implement FSetTextBlockBindingCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterSetTextBlockBindingCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterCheckWidgetComponentExistsCommand()
{
    // TODO: Implement FCheckWidgetComponentExistsCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterCheckWidgetComponentExistsCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterSetWidgetPlacementCommand()
{
    // TODO: Implement FSetWidgetPlacementCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterSetWidgetPlacementCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterGetWidgetContainerDimensionsCommand()
{
    // TODO: Implement FGetWidgetContainerDimensionsCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterGetWidgetContainerDimensionsCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterGetWidgetComponentLayoutCommand()
{
    // TODO: Implement FGetWidgetComponentLayoutCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterGetWidgetComponentLayoutCommand: Command class not yet implemented"));
}

// Widget-specific add commands - placeholders
void FUMGCommandRegistration::RegisterAddWidgetSwitcherCommand()
{
    // TODO: Implement FAddWidgetSwitcherCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddWidgetSwitcherCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddThrobberCommand()
{
    // TODO: Implement FAddThrobberCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddThrobberCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddExpandableAreaCommand()
{
    // TODO: Implement FAddExpandableAreaCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddExpandableAreaCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddMenuAnchorCommand()
{
    // TODO: Implement FAddMenuAnchorCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddMenuAnchorCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddRichTextBlockCommand()
{
    // TODO: Implement FAddRichTextBlockCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddRichTextBlockCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddSafeZoneCommand()
{
    // TODO: Implement FAddSafeZoneCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddSafeZoneCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddInvalidationBoxCommand()
{
    // TODO: Implement FAddInvalidationBoxCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddInvalidationBoxCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddInputKeySelectorCommand()
{
    // TODO: Implement FAddInputKeySelectorCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddInputKeySelectorCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddMultiLineEditableTextCommand()
{
    // TODO: Implement FAddMultiLineEditableTextCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddMultiLineEditableTextCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddSizeBoxCommand()
{
    // TODO: Implement FAddSizeBoxCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddSizeBoxCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddImageCommand()
{
    // TODO: Implement FAddImageCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddImageCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddCheckBoxCommand()
{
    // TODO: Implement FAddCheckBoxCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddCheckBoxCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddSliderCommand()
{
    // TODO: Implement FAddSliderCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddSliderCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddProgressBarCommand()
{
    // TODO: Implement FAddProgressBarCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddProgressBarCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddBorderCommand()
{
    // TODO: Implement FAddBorderCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddBorderCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddScrollBoxCommand()
{
    // TODO: Implement FAddScrollBoxCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddScrollBoxCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddSpacerCommand()
{
    // TODO: Implement FAddSpacerCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddSpacerCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAddChildWidgetCommand()
{
    // TODO: Implement FAddChildWidgetCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterAddChildWidgetCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterCreateParentChildWidgetCommand()
{
    // TODO: Implement FCreateParentChildWidgetCommand class
    UE_LOG(LogTemp, Warning, TEXT("FUMGCommandRegistration::RegisterCreateParentChildWidgetCommand: Command class not yet implemented"));
}

void FUMGCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }
    
    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
        UE_LOG(LogTemp, Verbose, TEXT("FUMGCommandRegistration::RegisterAndTrackCommand: Registered and tracked command '%s'"), *CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}