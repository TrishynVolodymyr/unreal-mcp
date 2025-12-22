#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UWidgetBlueprint;
class UEdGraph;

/**
 * Service for creating input event handlers in Widget Blueprints
 * Handles mouse button, keyboard, touch, focus, and drag events
 */
class UNREALMCP_API FWidgetInputHandlerService
{
public:
    /**
     * Create an input event handler in a Widget Blueprint
     *
     * This method creates handlers for input events not exposed as standard delegates,
     * such as right mouse button clicks, keyboard events, touch events, etc.
     *
     * @param WidgetBlueprint - Target Widget Blueprint
     * @param ComponentName - Name of the widget component (optional - if empty, handles at widget level)
     * @param InputType - Type of input: "MouseButton", "Key", "Touch", "Focus", "Drag"
     * @param InputEvent - Specific input event (e.g., "RightMouseButton", "Enter", etc.)
     * @param Trigger - When to trigger: "Pressed", "Released", "DoubleClick"
     * @param HandlerName - Name of the function to create
     * @param OutActualHandlerName - The actual handler function name that was created
     * @return true if the input handler was created successfully
     */
    static bool CreateWidgetInputHandler(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName,
                                        const FString& InputType, const FString& InputEvent,
                                        const FString& Trigger, const FString& HandlerName,
                                        FString& OutActualHandlerName);

    /**
     * Remove a function graph from a Widget Blueprint
     * Use this to clean up broken/corrupt function graphs
     *
     * @param WidgetBlueprint - Target Widget Blueprint
     * @param FunctionName - Name of the function graph to remove
     * @return true if the function graph was removed successfully
     */
    static bool RemoveWidgetFunctionGraph(UWidgetBlueprint* WidgetBlueprint, const FString& FunctionName);

private:
    /**
     * Get or create the input override function for handling input events
     * @param WidgetBlueprint - Target widget blueprint
     * @param InputType - Type of input event (MouseButton, Key, etc.)
     * @param Trigger - When to trigger (Pressed, Released, etc.)
     * @return The override function graph, or nullptr if creation failed
     */
    static UEdGraph* GetOrCreateInputOverrideFunction(UWidgetBlueprint* WidgetBlueprint, const FString& InputType, const FString& Trigger);

    /**
     * Create a custom event in the widget blueprint's event graph
     * @param WidgetBlueprint - Target widget blueprint
     * @param HandlerName - Name of the custom event to create
     * @return true if the event was created successfully
     */
    static bool CreateCustomInputEvent(UWidgetBlueprint* WidgetBlueprint, const FString& HandlerName);

    /**
     * Add input checking logic to connect the override to the custom event
     * @param WidgetBlueprint - Target widget blueprint
     * @param OverrideGraph - The override function graph
     * @param InputType - Type of input event
     * @param InputEvent - Specific input event to check for
     * @param Trigger - Trigger type (Pressed, Released, etc.)
     * @param HandlerName - Name of the custom event to call
     * @param ComponentName - Name of the component to check (optional)
     * @return true if logic was added successfully
     */
    static bool AddInputCheckingLogic(UWidgetBlueprint* WidgetBlueprint, UEdGraph* OverrideGraph,
                                     const FString& InputType, const FString& InputEvent,
                                     const FString& Trigger, const FString& HandlerName,
                                     const FString& ComponentName);

    /**
     * Get the key name for a given input event string
     * @param InputEvent - The input event string (e.g., "RightMouseButton", "Enter")
     * @return The FKey name for use with input system
     */
    static FName GetKeyNameForInputEvent(const FString& InputEvent);

    /**
     * Get the override function name for a given input type and trigger
     * @param InputType - Type of input (MouseButton, Key, etc.)
     * @param Trigger - Trigger type (Pressed, Released, etc.)
     * @return The function name to override (e.g., "OnMouseButtonDown")
     */
    static FString GetOverrideFunctionName(const FString& InputType, const FString& Trigger);
};
