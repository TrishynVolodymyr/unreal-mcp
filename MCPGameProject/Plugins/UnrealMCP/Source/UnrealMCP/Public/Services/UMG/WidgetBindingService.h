#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Service for creating event and property bindings in Widget Blueprints
 * Handles delegate event bindings and text block property bindings
 */
class UNREALMCP_API FWidgetBindingService
{
public:
    /**
     * Create an event binding for a widget component using UK2Node_ComponentBoundEvent
     * @param WidgetBlueprint - Widget blueprint containing the component
     * @param Widget - Widget component to bind event to
     * @param WidgetVarName - Variable name of the widget (must be exposed as variable)
     * @param EventName - Name of the event to bind (e.g., "OnClicked")
     * @param FunctionName - Name of the function to create/bind
     * @return true if the event was bound successfully
     */
    static bool CreateEventBinding(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget,
                                   const FString& WidgetVarName, const FString& EventName,
                                   const FString& FunctionName);

    /**
     * Create a text block binding function
     * @param WidgetBlueprint - Widget blueprint containing the text block
     * @param TextBlockName - Name of the text block widget component
     * @param BindingName - Name of the variable to bind to
     * @param VariableType - Type of the binding variable
     * @return true if the binding was created successfully
     */
    static bool CreateTextBlockBindingFunction(UWidgetBlueprint* WidgetBlueprint,
                                               const FString& TextBlockName,
                                               const FString& BindingName,
                                               const FString& VariableType);
};
