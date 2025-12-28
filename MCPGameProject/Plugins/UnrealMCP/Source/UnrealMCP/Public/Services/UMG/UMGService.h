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
    /**
     * Get the singleton instance of the UMG service
     * @return Reference to the singleton instance
     */
    static FUMGService& Get();
    
    virtual ~FUMGService();

    // IUMGService interface
    virtual UWidgetBlueprint* CreateWidgetBlueprint(const FString& Name, const FString& ParentClass = TEXT("UserWidget"), const FString& Path = TEXT("/Game/Widgets")) override;
    virtual bool DoesWidgetBlueprintExist(const FString& Name, const FString& Path = TEXT("/Game/Widgets")) override;
    virtual UWidget* AddWidgetComponent(const FString& BlueprintName, const FString& ComponentName,
                                       const FString& ComponentType, const FVector2D& Position,
                                       const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs,
                                       FString* OutError = nullptr) override;
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

    virtual bool AddChildWidgetComponentToParent(const FString& BlueprintName, const FString& ParentComponentName,
                                               const FString& ChildComponentName, bool bCreateParentIfMissing = false,
                                               const FString& ParentComponentType = TEXT("Border"),
                                               const FVector2D& ParentPosition = FVector2D(0.0f, 0.0f),
                                               const FVector2D& ParentSize = FVector2D(300.0f, 200.0f)) override;

    virtual bool CreateParentAndChildWidgetComponents(const FString& BlueprintName, const FString& ParentComponentName,
                                                    const FString& ChildComponentName, const FString& ParentComponentType = TEXT("Border"),
                                                    const FString& ChildComponentType = TEXT("TextBlock"),
                                                    const FVector2D& ParentPosition = FVector2D(0.0f, 0.0f),
                                                    const FVector2D& ParentSize = FVector2D(300.0f, 200.0f),
                                                    const TSharedPtr<FJsonObject>& ChildAttributes = nullptr) override;

    virtual bool GetWidgetComponentLayout(const FString& BlueprintName, TSharedPtr<FJsonObject>& OutLayoutInfo) override;

    virtual bool CaptureWidgetScreenshot(const FString& BlueprintName, int32 Width, int32 Height,
                                        const FString& Format, TSharedPtr<FJsonObject>& OutScreenshotData) override;

    virtual bool CreateWidgetInputHandler(const FString& WidgetName, const FString& ComponentName,
                                         const FString& InputType, const FString& InputEvent,
                                         const FString& Trigger, const FString& HandlerName,
                                         FString& OutActualHandlerName) override;

    virtual bool RemoveWidgetFunctionGraph(const FString& WidgetName, const FString& FunctionName) override;

    virtual bool ReorderWidgetChildren(const FString& WidgetName, const FString& ContainerName,
                                      const TArray<FString>& ChildOrder) override;

private:
    /** Private constructor for singleton pattern */
    FUMGService();
    
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
     * Set a single property on a widget component (delegates to PropertyService)
     */
    bool SetWidgetProperty(UWidget* Widget, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue) const;

    /**
     * Create an event binding (delegates to WidgetBindingService)
     */
    bool CreateEventBinding(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, const FString& WidgetVarName, const FString& EventName, const FString& FunctionName) const;

    /**
     * Create a text block binding function (delegates to WidgetBindingService)
     */
    bool CreateTextBlockBindingFunction(UWidgetBlueprint* WidgetBlueprint, const FString& TextBlockName, const FString& BindingName, const FString& VariableType) const;

    /**
     * Set widget placement in canvas panel slot
     */
    bool SetCanvasSlotPlacement(UWidget* Widget, const FVector2D* Position, const FVector2D* Size, const FVector2D* Alignment) const;

    /**
     * Set a property on the widget's slot (HorizontalBoxSlot, VerticalBoxSlot, etc.)
     * Supports properties: SizeRule, VerticalAlignment, HorizontalAlignment, Padding
     * @param Widget - The widget whose slot to modify
     * @param PropertyName - Name of the slot property (without "Slot." prefix)
     * @param PropertyValue - Value to set
     * @param OutError - Error message if setting fails
     * @return true if property was set successfully
     */
    bool SetSlotProperty(UWidget* Widget, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) const;

    /**
     * Add a widget as a child to another widget (parent must be a panel widget)
     */
    bool AddWidgetToParent(UWidget* ChildWidget, UWidget* ParentWidget) const;
};
