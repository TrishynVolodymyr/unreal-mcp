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
     * @return Created widget component or nullptr if creation failed
     */
    virtual UWidget* AddWidgetComponent(const FString& BlueprintName, const FString& ComponentName, 
                                       const FString& ComponentType, const FVector2D& Position, 
                                       const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs) = 0;

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
};
