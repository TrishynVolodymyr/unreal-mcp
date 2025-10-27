#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

/**
 * Factory class for creating and managing Unreal Engine component types
 * Implements the Factory pattern to centralize component creation logic
 * and provide type-safe component instantiation
 */
class UNREALMCP_API FComponentFactory
{
public:
    /**
     * Get the singleton instance of the component factory
     * @return Reference to the singleton instance
     */
    static FComponentFactory& Get();

    /**
     * Register a component type with the factory
     * @param TypeName - String identifier for the component type (e.g., "StaticMeshComponent")
     * @param ComponentClass - UClass pointer for the component type
     */
    void RegisterComponentType(const FString& TypeName, UClass* ComponentClass);

    /**
     * Get the UClass for a registered component type
     * @param TypeName - String identifier for the component type
     * @return UClass pointer for the component type, or nullptr if not found
     */
    UClass* GetComponentClass(const FString& TypeName) const;

    /**
     * Get all available component type names
     * @return Array of registered component type names
     */
    TArray<FString> GetAvailableTypes() const;

    /**
     * Initialize the factory with default Unreal Engine component types
     * This method registers commonly used component types
     */
    void InitializeDefaultTypes();

private:
    /** Private constructor for singleton pattern */
    FComponentFactory() = default;

    /** Map of component type names to their corresponding UClass pointers */
    TMap<FString, UClass*> ComponentTypeMap;

    /** Flag to track if default types have been initialized */
    bool bDefaultTypesInitialized = false;

    /** Critical section for thread safety */
    mutable FCriticalSection ComponentMapLock;
};
