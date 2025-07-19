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
};
