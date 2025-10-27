#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Validation result structure for widget operations
 */
struct UNREALMCP_API FWidgetValidationResult
{
    bool bIsValid = false;
    FString ErrorMessage;
    TArray<FString> Warnings;

    FWidgetValidationResult() = default;
    
    FWidgetValidationResult(bool bValid, const FString& Error = FString())
        : bIsValid(bValid), ErrorMessage(Error)
    {
    }

    static FWidgetValidationResult Success()
    {
        return FWidgetValidationResult(true);
    }

    static FWidgetValidationResult Error(const FString& Message)
    {
        return FWidgetValidationResult(false, Message);
    }

    void AddWarning(const FString& Warning)
    {
        Warnings.Add(Warning);
    }
};

/**
 * Service for validating UMG widget operations and parameters
 * Provides comprehensive validation for widget creation, modification, and hierarchy
 */
class UNREALMCP_API FWidgetValidationService
{
public:
    FWidgetValidationService();

    /**
     * Validate widget blueprint creation parameters
     * @param Name - Widget blueprint name
     * @param ParentClass - Parent class name
     * @param Path - Creation path
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetBlueprintCreation(const FString& Name, const FString& ParentClass, const FString& Path) const;

    /**
     * Validate widget component creation parameters
     * @param BlueprintName - Target blueprint name
     * @param ComponentName - Component name
     * @param ComponentType - Component type
     * @param Position - Component position
     * @param Size - Component size
     * @param Kwargs - Additional parameters
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetComponentCreation(const FString& BlueprintName, const FString& ComponentName, 
                                                           const FString& ComponentType, const FVector2D& Position, 
                                                           const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs) const;

    /**
     * Validate widget property setting parameters
     * @param BlueprintName - Target blueprint name
     * @param ComponentName - Component name
     * @param Properties - Properties to set
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetPropertySetting(const FString& BlueprintName, const FString& ComponentName, 
                                                         const TSharedPtr<FJsonObject>& Properties) const;

    /**
     * Validate widget event binding parameters
     * @param BlueprintName - Target blueprint name
     * @param ComponentName - Component name
     * @param EventName - Event name
     * @param FunctionName - Function name
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetEventBinding(const FString& BlueprintName, const FString& ComponentName, 
                                                      const FString& EventName, const FString& FunctionName) const;

    /**
     * Validate widget hierarchy operations
     * @param BlueprintName - Target blueprint name
     * @param ParentComponentName - Parent component name
     * @param ChildComponentName - Child component name
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetHierarchy(const FString& BlueprintName, const FString& ParentComponentName, 
                                                   const FString& ChildComponentName) const;

    /**
     * Validate widget type compatibility
     * @param ComponentType - Component type to validate
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetType(const FString& ComponentType) const;

    /**
     * Validate widget property compatibility
     * @param ComponentType - Component type
     * @param PropertyName - Property name
     * @param PropertyValue - Property value
     * @return Validation result
     */
    FWidgetValidationResult ValidateWidgetProperty(const FString& ComponentType, const FString& PropertyName, 
                                                  const TSharedPtr<FJsonValue>& PropertyValue) const;

private:
    /** Set of valid widget component types */
    TSet<FString> ValidWidgetTypes;

    /** Map of widget types to their valid properties */
    TMap<FString, TSet<FString>> WidgetTypeProperties;

    /** Map of widget types to their valid events */
    TMap<FString, TSet<FString>> WidgetTypeEvents;

    /**
     * Initialize validation data
     */
    void InitializeValidationData();

    /**
     * Validate a name string (no special characters, not empty, etc.)
     * @param Name - Name to validate
     * @param NameType - Type of name for error messages
     * @return Validation result
     */
    FWidgetValidationResult ValidateName(const FString& Name, const FString& NameType) const;

    /**
     * Validate a path string
     * @param Path - Path to validate
     * @return Validation result
     */
    FWidgetValidationResult ValidatePath(const FString& Path) const;

    /**
     * Validate position values
     * @param Position - Position to validate
     * @return Validation result
     */
    FWidgetValidationResult ValidatePosition(const FVector2D& Position) const;

    /**
     * Validate size values
     * @param Size - Size to validate
     * @return Validation result
     */
    FWidgetValidationResult ValidateSize(const FVector2D& Size) const;

    /**
     * Check if a widget blueprint exists
     * @param BlueprintName - Blueprint name to check
     * @return true if blueprint exists
     */
    bool DoesWidgetBlueprintExist(const FString& BlueprintName) const;

    /**
     * Check if a widget component exists in a blueprint
     * @param BlueprintName - Blueprint name
     * @param ComponentName - Component name
     * @return true if component exists
     */
    bool DoesWidgetComponentExist(const FString& BlueprintName, const FString& ComponentName) const;

    /**
     * Get the class for a widget type
     * @param ComponentType - Component type
     * @return Widget class or nullptr if not found
     */
    UClass* GetWidgetClass(const FString& ComponentType) const;

    /**
     * Check if a property exists on a widget class
     * @param WidgetClass - Widget class
     * @param PropertyName - Property name
     * @return true if property exists
     */
    bool DoesPropertyExist(UClass* WidgetClass, const FString& PropertyName) const;

    /**
     * Check if an event exists on a widget class
     * @param WidgetClass - Widget class
     * @param EventName - Event name
     * @return true if event exists
     */
    bool DoesEventExist(UClass* WidgetClass, const FString& EventName) const;
};
