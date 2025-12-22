#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Interface for UMG (Widget Blueprint) operations
 * Provides a standardized way to create and modify UMG Widget Blueprints and their components
 */
class UNREALMCP_API IUMGService
{
public:
    virtual ~IUMGService() = default;

    /**
     * Create a new UMG Widget Blueprint
     * @param Name - Name of the widget blueprint
     * @param ParentClass - Parent class name (default: UserWidget)
     * @param Path - Path where to create the blueprint (default: /Game/Widgets)
     * @return Created widget blueprint or nullptr if creation failed
     */
    virtual UWidgetBlueprint* CreateWidgetBlueprint(const FString& Name, const FString& ParentClass = TEXT("UserWidget"), const FString& Path = TEXT("/Game/Widgets")) = 0;

    /**
     * Check if a widget blueprint exists
     * @param Name - Name of the widget blueprint
     * @param Path - Path where to check for the blueprint
     * @return true if the blueprint exists
     */
    virtual bool DoesWidgetBlueprintExist(const FString& Name, const FString& Path = TEXT("/Game/Widgets")) = 0;

    /**
     * Add a widget component to a widget blueprint
     * @param BlueprintName - Name of the target widget blueprint
     * @param ComponentName - Name for the new component
     * @param ComponentType - Type of component to create
     * @param Position - Position in the canvas
     * @param Size - Size of the component
     * @param Kwargs - Additional parameters for the component
     * @param OutError - Output error message if creation failed (optional)
     * @return Created widget component or nullptr if creation failed
     */
    virtual UWidget* AddWidgetComponent(const FString& BlueprintName, const FString& ComponentName,
                                       const FString& ComponentType, const FVector2D& Position,
                                       const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs,
                                       FString* OutError = nullptr) = 0;

    /**
     * Set properties on a widget component
     * @param BlueprintName - Name of the target widget blueprint
     * @param ComponentName - Name of the component to modify
     * @param Properties - Properties to set
     * @param OutSuccessProperties - List of successfully set properties
     * @param OutFailedProperties - List of properties that failed to set
     * @return true if at least one property was set successfully
     */
    virtual bool SetWidgetProperties(const FString& BlueprintName, const FString& ComponentName, 
                                    const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, 
                                    TArray<FString>& OutFailedProperties) = 0;

    /**
     * Bind an event to a widget component
     * @param BlueprintName - Name of the target widget blueprint
     * @param ComponentName - Name of the component
     * @param EventName - Name of the event to bind
     * @param FunctionName - Name of the function to create/bind (optional)
     * @param OutActualFunctionName - The actual function name that was created/bound
     * @return true if the event was bound successfully
     */
    virtual bool BindWidgetEvent(const FString& BlueprintName, const FString& ComponentName, 
                                const FString& EventName, const FString& FunctionName, 
                                FString& OutActualFunctionName) = 0;

    /**
     * Set up text block binding for dynamic updates
     * @param BlueprintName - Name of the target widget blueprint
     * @param TextBlockName - Name of the text block widget
     * @param BindingName - Name of the binding property
     * @param VariableType - Type of the binding variable
     * @return true if the binding was set up successfully
     */
    virtual bool SetTextBlockBinding(const FString& BlueprintName, const FString& TextBlockName, 
                                    const FString& BindingName, const FString& VariableType = TEXT("Text")) = 0;

    /**
     * Check if a widget component exists in a blueprint
     * @param BlueprintName - Name of the target widget blueprint
     * @param ComponentName - Name of the component to check
     * @return true if the component exists
     */
    virtual bool DoesWidgetComponentExist(const FString& BlueprintName, const FString& ComponentName) = 0;

    /**
     * Set the placement (position/size) of a widget component
     * @param BlueprintName - Name of the target widget blueprint
     * @param ComponentName - Name of the component to modify
     * @param Position - New position (optional)
     * @param Size - New size (optional)
     * @param Alignment - New alignment (optional)
     * @return true if the placement was set successfully
     */
    virtual bool SetWidgetPlacement(const FString& BlueprintName, const FString& ComponentName, 
                                   const FVector2D* Position = nullptr, const FVector2D* Size = nullptr, 
                                   const FVector2D* Alignment = nullptr) = 0;

    /**
     * Get the dimensions of a container widget
     * @param BlueprintName - Name of the target widget blueprint
     * @param ContainerName - Name of the container widget (optional, defaults to root canvas)
     * @param OutDimensions - Output dimensions
     * @return true if the dimensions were retrieved successfully
     */
    virtual bool GetWidgetContainerDimensions(const FString& BlueprintName, const FString& ContainerName, 
                                             FVector2D& OutDimensions) = 0;

    /**
     * Add a widget component as a child to another component
     * @param BlueprintName - Name of the target widget blueprint
     * @param ParentComponentName - Name of the parent component
     * @param ChildComponentName - Name of the child component to add to the parent
     * @param bCreateParentIfMissing - Whether to create the parent component if it doesn't exist
     * @param ParentComponentType - Type of parent component to create if needed
     * @param ParentPosition - Position of the parent component if created
     * @param ParentSize - Size of the parent component if created
     * @return true if the child was added successfully
     */
    virtual bool AddChildWidgetComponentToParent(const FString& BlueprintName, const FString& ParentComponentName,
                                               const FString& ChildComponentName, bool bCreateParentIfMissing = false,
                                               const FString& ParentComponentType = TEXT("Border"),
                                               const FVector2D& ParentPosition = FVector2D(0.0f, 0.0f),
                                               const FVector2D& ParentSize = FVector2D(300.0f, 200.0f)) = 0;

    /**
     * Create a new parent widget component with a new child component
     * @param BlueprintName - Name of the target widget blueprint
     * @param ParentComponentName - Name for the new parent component
     * @param ChildComponentName - Name for the new child component
     * @param ParentComponentType - Type of parent component to create
     * @param ChildComponentType - Type of child component to create
     * @param ParentPosition - Position of the parent component
     * @param ParentSize - Size of the parent component
     * @param ChildAttributes - Additional attributes for the child component
     * @return true if both components were created successfully
     */
    virtual bool CreateParentAndChildWidgetComponents(const FString& BlueprintName, const FString& ParentComponentName,
                                                    const FString& ChildComponentName, const FString& ParentComponentType = TEXT("Border"),
                                                    const FString& ChildComponentType = TEXT("TextBlock"),
                                                    const FVector2D& ParentPosition = FVector2D(0.0f, 0.0f),
                                                    const FVector2D& ParentSize = FVector2D(300.0f, 200.0f),
                                                    const TSharedPtr<FJsonObject>& ChildAttributes = nullptr) = 0;

    /**
     * Get hierarchical layout information for all components within a UMG Widget Blueprint
     * @param BlueprintName - Name of the target widget blueprint
     * @param OutLayoutInfo - Output JSON object containing hierarchical component layout information
     * @return true if layout information was retrieved successfully
     */
    virtual bool GetWidgetComponentLayout(const FString& BlueprintName, TSharedPtr<FJsonObject>& OutLayoutInfo) = 0;

    /**
     * Capture a screenshot of a widget blueprint preview
     * Renders the widget to a texture and returns as base64-encoded image data
     * @param BlueprintName - Name of the target widget blueprint
     * @param Width - Width of the screenshot in pixels
     * @param Height - Height of the screenshot in pixels
     * @param Format - Image format ("png" or "jpg")
     * @param OutScreenshotData - Output JSON object containing base64-encoded image and metadata
     * @return true if screenshot was captured successfully
     */
    virtual bool CaptureWidgetScreenshot(const FString& BlueprintName, int32 Width, int32 Height,
                                        const FString& Format, TSharedPtr<FJsonObject>& OutScreenshotData) = 0;

    /**
     * Create an input event handler in a Widget Blueprint
     *
     * This method creates handlers for input events not exposed as standard delegates,
     * such as right mouse button clicks, keyboard events, touch events, etc.
     *
     * It works by:
     * 1. Creating a custom event function in the Widget Blueprint
     * 2. Overriding the appropriate input handler (OnMouseButtonDown, OnKeyDown, etc.)
     * 3. Adding logic to check for the specific input and call the custom event
     *
     * @param WidgetName - Name of the target Widget Blueprint
     * @param ComponentName - Name of the widget component (optional - if empty, handles at widget level)
     * @param InputType - Type of input: "MouseButton", "Key", "Touch", "Focus", "Drag"
     * @param InputEvent - Specific input event:
     *                     MouseButton: LeftMouseButton, RightMouseButton, MiddleMouseButton, ThumbMouseButton, ThumbMouseButton2
     *                     Key: Any key name (Enter, Escape, SpaceBar, A-Z, F1-F12, etc.)
     *                     Touch: Touch, Pinch, Swipe
     *                     Focus: FocusReceived, FocusLost
     *                     Drag: DragDetected, DragEnter, DragLeave, DragOver, Drop
     * @param Trigger - When to trigger: "Pressed", "Released", "DoubleClick"
     * @param HandlerName - Name of the function to create
     * @param OutActualHandlerName - The actual handler function name that was created
     * @return true if the input handler was created successfully
     */
    virtual bool CreateWidgetInputHandler(const FString& WidgetName, const FString& ComponentName,
                                         const FString& InputType, const FString& InputEvent,
                                         const FString& Trigger, const FString& HandlerName,
                                         FString& OutActualHandlerName) = 0;

    /**
     * Remove a function graph from a Widget Blueprint
     * Use this to clean up broken/corrupt function graphs
     *
     * @param WidgetName - Name of the target Widget Blueprint
     * @param FunctionName - Name of the function graph to remove
     * @return true if the function graph was removed successfully
     */
    virtual bool RemoveWidgetFunctionGraph(const FString& WidgetName, const FString& FunctionName) = 0;
};
