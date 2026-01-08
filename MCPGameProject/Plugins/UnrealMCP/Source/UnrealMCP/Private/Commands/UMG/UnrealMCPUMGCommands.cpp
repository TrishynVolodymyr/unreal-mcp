#include "Commands/UMG/UnrealMCPUMGCommands.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPMainDispatcher.h"

FUnrealMCPUMGCommands::FUnrealMCPUMGCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
    // All UMG commands now route through the new main dispatcher and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(CommandName, Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("create_umg_widget_blueprint"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("bind_widget_component_event"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_text_block_widget_component_binding"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetSwitcherToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_widget_switcher_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddThrobberToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_throbber_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddExpandableAreaToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_expandable_area_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddMenuAnchorToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_menu_anchor_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddRichTextBlockToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_rich_text_block_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSafeZoneToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_safe_zone_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddInvalidationBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_invalidation_box_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddInputKeySelectorToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_input_key_selector_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddMultiLineEditableTextToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_multi_line_editable_text_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSizeBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_size_box_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetContainerDimensions(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_widget_container_component_dimensions"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_image_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_checkbox_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_slider_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_progress_bar_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddBorderToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_border_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_scroll_box_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddSpacerToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_spacer_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCheckComponentExists(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("check_widget_component_exists"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetPlacement(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_widget_component_placement"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetAsChild(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_child_widget_component_to_parent"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetComponent(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("add_widget_component_to_widget"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("set_widget_component_property"), Params);
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetComponentLayout(const TSharedPtr<FJsonObject>& Params)
{
    // Route through new architecture - all UMG commands now use the service layer and command registry
    return FUnrealMCPMainDispatcher::Get().HandleCommand(TEXT("get_widget_component_layout"), Params);
}

