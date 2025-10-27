#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"

/**
 * Factory class for creating and managing UMG widget types
 * Implements the Factory pattern to centralize widget creation logic
 * and provide type-safe widget instantiation for UMG components
 */
class UNREALMCP_API FWidgetFactory
{
public:
    /**
     * Get the singleton instance of the widget factory
     * @return Reference to the singleton instance
     */
    static FWidgetFactory& Get();

    /**
     * Register a widget type with the factory
     * @param TypeName - String identifier for the widget type (e.g., "TextBlock")
     * @param WidgetClass - UClass pointer for the widget type
     */
    void RegisterWidgetType(const FString& TypeName, UClass* WidgetClass);

    /**
     * Get the UClass for a registered widget type
     * @param TypeName - String identifier for the widget type
     * @return UClass pointer for the widget type, or nullptr if not found
     */
    UClass* GetWidgetClass(const FString& TypeName) const;

    /**
     * Get all available widget type names
     * @return Array of registered widget type names
     */
    TArray<FString> GetAvailableWidgetTypes() const;

    /**
     * Create a widget instance of the specified type
     * @param TypeName - String identifier for the widget type
     * @param WidgetName - Name to assign to the created widget (optional)
     * @return Pointer to the created widget, or nullptr if creation failed
     */
    UWidget* CreateWidget(const FString& TypeName, const FString& WidgetName = TEXT(""));

    /**
     * Initialize the factory with default UMG widget types
     * This method registers commonly used UMG widget types
     */
    void InitializeDefaultWidgetTypes();

private:
    /** Private constructor for singleton pattern */
    FWidgetFactory() = default;

    /** Map of widget type names to their corresponding UClass pointers */
    TMap<FString, UClass*> WidgetTypeMap;

    /** Flag to track if default types have been initialized */
    bool bDefaultTypesInitialized = false;

    /** Critical section for thread safety */
    mutable FCriticalSection WidgetMapLock;
};
