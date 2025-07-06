#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class FWidgetComponentService;

/**
 * Handles UMG (Widget Blueprint) related MCP commands
 * Responsible for creating and modifying UMG Widget Blueprints,
 * adding widget components, and managing widget instances in the viewport.
 */
class UNREALMCP_API FUnrealMCPUMGCommands
{
public:
    FUnrealMCPUMGCommands();

    /**
     * Handle UMG-related commands
     * @param CommandType - The type of command to handle
     * @param Params - JSON parameters for the command
     * @return JSON response with results or error
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    /**
     * Create a new UMG Widget Blueprint
     * @param Params - Must include "name" for the blueprint name
     * @return JSON response with the created blueprint details
     */
    TSharedPtr<FJsonObject> HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a widget instance to the game viewport
     * @param Params - Must include:
     *                "blueprint_name" - Name of the Widget Blueprint to instantiate
     *                "z_order" - Z-order for widget display (optional)
     * @return JSON response with the widget instance details
     */
    TSharedPtr<FJsonObject> HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params);

    /**
     * Bind an event to a widget (e.g. button click)
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name of the widget to bind
     *                "event_name" - Name of the event to bind
     * @return JSON response with the binding details
     */
    TSharedPtr<FJsonObject> HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set up text block binding for dynamic updates
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name of the widget to bind
     *                "binding_name" - Name of the binding to set up
     * @return JSON response with the binding details
     */
    TSharedPtr<FJsonObject> HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Widget Switcher to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Widget Switcher
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddWidgetSwitcherToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Throbber widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Throbber widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddThrobberToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add an Expandable Area widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Expandable Area widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddExpandableAreaToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Menu Anchor widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Menu Anchor widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddMenuAnchorToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Rich Text Block widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Rich Text Block widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddRichTextBlockToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Safe Zone widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Safe Zone widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddSafeZoneToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add an Invalidation Box widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Invalidation Box widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddInvalidationBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add an Input Key Selector widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Input Key Selector widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddInputKeySelectorToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Multi-Line Editable Text widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Multi-Line Editable Text widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddMultiLineEditableTextToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Size Box widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Size Box widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddSizeBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get the dimensions of a container widget in a UMG Widget Blueprint
     * @param Params - Must include:
     *                "widget_name" - Name of the target Widget Blueprint
     *                "container_name" - Name of the container widget (optional, defaults to root canvas)
     * @return JSON response with the container dimensions
     */
    TSharedPtr<FJsonObject> HandleGetWidgetContainerDimensions(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add an Image widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Image widget
     *                "brush_asset_path" - Path to the image asset to use (optional)
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a CheckBox widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new CheckBox widget
     *                "is_checked" - Initial checked state (optional)
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Slider widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Slider widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Progress Bar widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Progress Bar widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Border widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Border widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddBorderToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a ScrollBox widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new ScrollBox widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a Spacer widget to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name for the new Spacer widget
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddSpacerToWidget(const TSharedPtr<FJsonObject>& Params);

    /**
     * Check if a component exists in a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "component_name" - Name of the component to check
     * @return JSON response with existence status of the component
     */
    TSharedPtr<FJsonObject> HandleCheckComponentExists(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set the placement (position/size) of a widget component in a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "widget_name" - Name of the component to modify
     *                "position" - [X, Y] new position in the canvas panel (optional)
     *                "size" - [Width, Height] new size for the component (optional)
     *                "alignment" - [X, Y] alignment values (0.0 to 1.0) (optional)
     * @return JSON response with updated placement information
     */
    TSharedPtr<FJsonObject> HandleSetWidgetPlacement(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a widget component as a child to another component
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "parent_component_name" - Name of the parent component
     *                "child_component_name" - Name of the child component to add
     *                "create_parent_if_missing" - Whether to create the parent if it doesn't exist (optional)
     *                "parent_component_type" - Type of parent component to create if needed (optional)
     *                "parent_position" - [X, Y] position of the parent component if created (optional)
     *                "parent_size" - [Width, Height] of the parent component if created (optional)
     * @return JSON response with the component relationship details
     */
    TSharedPtr<FJsonObject> HandleAddWidgetAsChild(const TSharedPtr<FJsonObject>& Params);

    /**
     * Create a new parent widget component with a new child component
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "parent_component_name" - Name for the new parent component
     *                "child_component_name" - Name for the new child component
     *                "parent_component_type" - Type of parent component to create (optional)
     *                "child_component_type" - Type of child component to create (optional)
     *                "parent_position" - [X, Y] position of the parent component (optional)
     *                "parent_size" - [Width, Height] of the parent component (optional)
     *                "child_attributes" - Additional attributes for the child component (optional)
     * @return JSON response with the component creation details
     */
    TSharedPtr<FJsonObject> HandleCreateWidgetComponentWithChild(const TSharedPtr<FJsonObject>& Params);

    /**
     * Unified function to add any type of widget component to a UMG Widget Blueprint
     * @param Params - Must include:
     *                "blueprint_name" - Name of the target Widget Blueprint
     *                "component_name" - Name for the new component
     *                "component_type" - Type of component to add (e.g., "TextBlock", "Button")
     *                "position" - Optional [X, Y] position in the canvas panel
     *                "size" - Optional [Width, Height] of the component
     *                "kwargs" - Additional component-specific parameters
     * @return JSON response with the added widget details
     */
    TSharedPtr<FJsonObject> HandleAddWidgetComponent(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set a property on a specific component within a UMG Widget Blueprint.
     * @param Params - JSON object containing widgetName, componentName, propertyName, and propertyValue.
     * @return JSON response indicating success or failure.
     */
    TSharedPtr<FJsonObject> HandleSetWidgetComponentProperty(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get layout information for all components within a UMG Widget Blueprint.
     * @param Params - Must include:
     *                "widget_name" - Name of the target Widget Blueprint
     * @return JSON response with a list of all widget components and their layout properties.
     *         Each component object contains:
     *         - name: Component name
     *         - type: Component class name
     *         - slot_properties: Layout information based on slot type:
     *           - position: [X, Y] (for CanvasPanelSlot)
     *           - size: [Width, Height] (for CanvasPanelSlot)
     *           - padding: [Left, Top, Right, Bottom] (for BoxSlot, BorderSlot, etc.)
     *           - horizontal_alignment: Alignment value (for BoxSlot, BorderSlot, etc.)
     *           - vertical_alignment: Alignment value (for BoxSlot, BorderSlot, etc.)
     *           - size_rule: Size rule description (for BoxSlot)
     *           - size_value: Associated value for size rule (for BoxSlot)
     *           - z_order: Z-order index (for CanvasPanelSlot)
     */
    TSharedPtr<FJsonObject> HandleGetWidgetComponentLayout(const TSharedPtr<FJsonObject>& Params);
};