#pragma once

#include "CoreMinimal.h"

/**
 * Static class responsible for registering all UMG-related commands
 * with the command registry system
 */
class UNREALMCP_API FUMGCommandRegistration
{
public:
    /**
     * Register all UMG commands with the command registry
     * This should be called during module startup
     */
    static void RegisterAllUMGCommands();
    
    /**
     * Unregister all UMG commands from the command registry
     * This should be called during module shutdown
     */
    static void UnregisterAllUMGCommands();

private:
    /** Array of registered command names for cleanup */
    static TArray<FString> RegisteredCommandNames;
    
    /**
     * Register individual UMG commands
     */
    static void RegisterCreateWidgetBlueprintCommand();
    static void RegisterBindWidgetEventCommand();
    static void RegisterSetTextBlockBindingCommand();
    static void RegisterAddWidgetComponentCommand();
    static void RegisterSetWidgetPropertyCommand();
    static void RegisterSetWidgetPlacementCommand();
    static void RegisterCaptureWidgetScreenshotCommand();
    static void RegisterGetWidgetBlueprintMetadataCommand();
    static void RegisterCreateWidgetInputHandlerCommand();
    static void RegisterRemoveWidgetFunctionGraphCommand();

    // Widget-specific add commands
    static void RegisterAddWidgetSwitcherCommand();
    static void RegisterAddThrobberCommand();
    static void RegisterAddExpandableAreaCommand();
    static void RegisterAddMenuAnchorCommand();
    static void RegisterAddRichTextBlockCommand();
    static void RegisterAddSafeZoneCommand();
    static void RegisterAddInvalidationBoxCommand();
    static void RegisterAddInputKeySelectorCommand();
    static void RegisterAddMultiLineEditableTextCommand();
    static void RegisterAddSizeBoxCommand();
    static void RegisterAddImageCommand();
    static void RegisterAddCheckBoxCommand();
    static void RegisterAddSliderCommand();
    static void RegisterAddProgressBarCommand();
    static void RegisterAddBorderCommand();
    static void RegisterAddScrollBoxCommand();
    static void RegisterAddSpacerCommand();
    
    // Parent-child widget commands
    static void RegisterAddChildWidgetCommand();
    static void RegisterCreateParentChildWidgetCommand();
    
    /**
     * Helper to register a command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};