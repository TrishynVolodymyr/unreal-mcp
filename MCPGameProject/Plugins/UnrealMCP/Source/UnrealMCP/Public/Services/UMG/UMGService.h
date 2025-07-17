#pragma once

#include "CoreMinimal.h"
#include "Services/UMG/IUMGService.h"
#include "Json.h"

// Forward declarations
class FWidgetComponentService;
class FWidgetValidationService;
class UWidgetBlueprint;
class UWidget;

/**
 * Implementation of IUMGService for UMG (Widget Blueprint) operations
 * Handles creation and modification of UMG Widget Blueprints and their components
 */
class UNREALMCP_API FUMGService : public IUMGService
{
public:
    FUMGService();
    virtual ~FUMGService() = default;

    // IUMGService interface
    virtual UWidgetBlueprint* CreateWidgetBlueprint(const FString& Name, const FString& ParentClass = TEXT("UserWidget"), const FString& Path = TEXT("/Game/Widgets")) override;
    virtual bool DoesWidgetBlueprintExist(const FString& Name, const FString& Path = TEXT("/Game/Widgets")) override;
    virtual UWidget* AddWidgetComponent(const FString& BlueprintName, const FString& ComponentName, 
                                       const FString& ComponentType, const FVector2D& Position, 
                                       const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs) override;
    virtual bool SetWidgetProperties(const FString& BlueprintName, const FString& ComponentName, 
                                    const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, 
                                    TArray<FString>& OutFailedProperties) override;
    virtual bool BindWidgetEvent(const FString& BlueprintName, const FString& ComponentName, 
                                const FString& EventName, const FString& FunctionName, 
                                FString& OutActualFunctionName) override;
    virtual bool SetTextBlockBinding(const FString& BlueprintName, const FString& TextBlockName, 
                                    const FString& BindingName, const FString& VariableType = TEXT("Text")) override;
    virtual bool DoesWidgetComponentExist(const FString& BlueprintName, const FString& ComponentName) override;
    virtual bool SetWidgetPlacement(const FString& BlueprintName, const FString& ComponentName, 
                                   const FVector2D* Position = nullptr, const FVector2D* Size = nullptr, 
                                   const FVector2D* Alignment = nullptr) override;
    virtual bool GetWidgetContainerDimensions(const FString& BlueprintName, const FString& ContainerName, 
                                             FVector2D& OutDimensions) override;

private:
    /** Widget component service for creating widget components */
    TUniquePtr<FWidgetComponentService> WidgetComponentService;

    /** Widget validation service for validating operations */
    TUniquePtr<FWidgetValidationService> ValidationService;

    /**
     * Find a Widget Blueprint by name or path
     * @param BlueprintNameOrPath - Name or path of the widget blueprint
     * @return Found widget blueprint or nullptr if not found
     */
    UWidgetBlueprint* FindWidgetBlueprint(const FString& BlueprintNameOrPath) const;

    /**
     * Create a widget blueprint using the factory pattern
     * @param Name - Name of the widget blueprint
     * @param ParentClass - Parent class for the widget
     * @param Path - Path where to create the blueprint
     * @return Created widget blueprint or nullptr if creation failed
     */
    UWidgetBlueprint* CreateWidgetBlueprintInternal(const FString& Name, UClass* ParentClass, const FString& Path) const;

    /**
     * Find a parent class by name
     * @param ParentClassName - Name of the parent class
     * @return Found class or nullptr if not found
     */
    UClass* FindParentClass(const FString& ParentClassName) const;

    /**
     * Set a single property on a widget component
     * @param Widget - Widget to modify
     * @param PropertyName - Name of the property to set
     * @param PropertyValue - Value to set
     * @return true if the property was set successfully
     */
    bool SetWidgetProperty(UWidget* Widget, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue) const;

    /**
     * Create an event binding for a widget component
     * @param WidgetBlueprint - Widget blueprint containing the component
     * @param Widget - Widget component to bind event to
     * @param EventName - Name of the event to bind
     * @param FunctionName - Name of the function to create/bind
     * @return true if the event was bound successfully
     */
    bool CreateEventBinding(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, const FString& EventName, const FString& FunctionName) const;

    /**
     * Create a text block binding function
     * @param WidgetBlueprint - Widget blueprint containing the text block
     * @param BindingName - Name of the binding property
     * @param VariableType - Type of the binding variable
     * @return true if the binding was created successfully
     */
    bool CreateTextBlockBindingFunction(UWidgetBlueprint* WidgetBlueprint, const FString& BindingName, const FString& VariableType) const;

    /**
     * Set widget placement in canvas panel slot
     * @param Widget - Widget to modify placement for
     * @param Position - New position (optional)
     * @param Size - New size (optional)
     * @param Alignment - New alignment (optional)
     * @return true if placement was set successfully
     */
    bool SetCanvasSlotPlacement(UWidget* Widget, const FVector2D* Position, const FVector2D* Size, const FVector2D* Alignment) const;
};