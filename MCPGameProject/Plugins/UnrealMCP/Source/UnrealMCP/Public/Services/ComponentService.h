#pragma once

#include "CoreMinimal.h"
#include "Services/IComponentService.h"
#include "Engine/Blueprint.h"

/**
 * Component type cache statistics for monitoring performance
 */
struct UNREALMCP_API FComponentTypeCacheStats
{
    /** Total number of cache requests */
    int32 TotalRequests = 0;
    
    /** Number of cache hits */
    int32 CacheHits = 0;
    
    /** Number of cache misses */
    int32 CacheMisses = 0;
    
    /** Number of cached component types */
    int32 CachedCount = 0;
    
    /** Number of cache refreshes */
    int32 RefreshCount = 0;
    
    /** Cache hit ratio (0.0 to 1.0) */
    float GetHitRatio() const
    {
        return TotalRequests > 0 ? static_cast<float>(CacheHits) / static_cast<float>(TotalRequests) : 0.0f;
    }
    
    /** Reset all statistics */
    void Reset()
    {
        TotalRequests = 0;
        CacheHits = 0;
        CacheMisses = 0;
        CachedCount = 0;
        RefreshCount = 0;
    }
};

/**
 * Component type cache for performance optimization
 * Thread-safe caching of component class lookups with automatic refresh and lazy loading
 */
class UNREALMCP_API FComponentTypeCache
{
public:
    /**
     * Get a cached component class by type string
     * @param ComponentType - String representation of component type
     * @return Cached UClass pointer or nullptr if not found
     */
    UClass* GetComponentClass(const FString& ComponentType);
    
    /**
     * Cache a component class for future lookups
     * @param ComponentType - String representation of component type
     * @param ComponentClass - UClass pointer to cache
     */
    void CacheComponentClass(const FString& ComponentType, UClass* ComponentClass);
    
    /**
     * Refresh the cache with current module state
     * Called automatically when modules are loaded/unloaded
     */
    void RefreshCache();
    
    /**
     * Preload common component types into cache
     */
    void PreloadCommonComponentTypes();
    
    /**
     * Check if a component type is cached
     * @param ComponentType - String representation of component type
     * @return true if the component type is cached
     */
    bool IsCached(const FString& ComponentType) const;
    
    /**
     * Clear all cached component types
     */
    void ClearCache();
    
    /**
     * Get cache statistics for monitoring
     * @return Current cache statistics
     */
    FComponentTypeCacheStats GetCacheStats() const;
    
    /**
     * Reset cache statistics
     */
    void ResetCacheStats();
    
    /**
     * Get the number of cached component types
     * @return Number of currently cached component types
     */
    int32 GetCachedCount() const;

private:
    /** Map of component type strings to weak class pointers */
    TMap<FString, TWeakObjectPtr<UClass>> CachedComponentClasses;
    
    /** Cache statistics for monitoring */
    mutable FComponentTypeCacheStats CacheStats;
    
    /** Critical section for thread safety */
    mutable FCriticalSection CacheLock;
    
    /** Flag to track if cache needs refresh */
    bool bNeedsRefresh = true;
    
    /**
     * Update cache statistics (must be called with lock held)
     * @param bWasHit - Whether this was a cache hit or miss
     */
    void UpdateStats(bool bWasHit) const;
    
    /**
     * Lazy load component class if not cached
     * @param ComponentType - String representation of component type
     * @return UClass pointer or nullptr if not found
     */
    UClass* LazyLoadComponentClass(const FString& ComponentType);
    
    /**
     * Get supported component types mapping
     * @return Map of component type strings to their full class names
     */
    TMap<FString, FString> GetSupportedComponentTypes() const;
    
    /**
     * Resolve component class from type string (internal implementation)
     * @param ComponentType - String representation of component type
     * @return UClass pointer or nullptr if not found
     */
    UClass* ResolveComponentClassInternal(const FString& ComponentType) const;
};

/**
 * Concrete implementation of IComponentService
 * Provides component creation, modification, and management functionality
 */
class UNREALMCP_API FComponentService : public IComponentService
{
public:
    /**
     * Get the singleton instance of the component service
     * @return Reference to the singleton instance
     */
    static FComponentService& Get();
    
    // IComponentService interface implementation
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params, FString& OutErrorMessage) override;
    virtual bool RemoveComponentFromBlueprint(UBlueprint* Blueprint, const FString& ComponentName) override;
    virtual UObject* FindComponentInBlueprint(UBlueprint* Blueprint, const FString& ComponentName) override;
    virtual TArray<TPair<FString, FString>> GetBlueprintComponents(UBlueprint* Blueprint) override;
    virtual bool IsValidComponentType(const FString& ComponentType) override;
    virtual UClass* GetComponentClass(const FString& ComponentType) override;
    
    /**
     * Set physics properties on a component
     * @param Blueprint - Target blueprint
     * @param ComponentName - Name of the component
     * @param PhysicsParams - Physics parameters to set
     * @return True if successful
     */
    bool SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& PhysicsParams);
    
    /**
     * Set static mesh properties on a component
     * @param Blueprint - Target blueprint
     * @param ComponentName - Name of the component
     * @param StaticMeshPath - Path to the static mesh asset
     * @return True if successful
     */
    bool SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath);

private:
    /** Private constructor for singleton pattern */
    FComponentService() = default;
    
    /** Component type cache for performance optimization */
    FComponentTypeCache ComponentTypeCache;
    
    /**
     * Get supported component types mapping
     * @return Map of component type strings to their full class names
     */
    TMap<FString, FString> GetSupportedComponentTypes() const;
    
    /**
     * Resolve component class from type string
     * @param ComponentType - String representation of component type
     * @return UClass pointer or nullptr if not found
     */
    UClass* ResolveComponentClass(const FString& ComponentType) const;
    
    /**
     * Set transform properties on a scene component
     * @param SceneComponent - Target scene component
     * @param Location - Location array
     * @param Rotation - Rotation array
     * @param Scale - Scale array
     */
    void SetComponentTransform(class USceneComponent* SceneComponent, 
                              const TArray<float>& Location,
                              const TArray<float>& Rotation,
                              const TArray<float>& Scale) const;
};
