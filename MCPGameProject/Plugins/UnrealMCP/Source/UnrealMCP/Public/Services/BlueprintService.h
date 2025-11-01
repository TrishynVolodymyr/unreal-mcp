#pragma once

#include "CoreMinimal.h"
#include "Services/IBlueprintService.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"

/**
 * Cache statistics for monitoring performance
 */
struct UNREALMCP_API FBlueprintCacheStats
{
    /** Total number of cache requests */
    int32 TotalRequests = 0;
    
    /** Number of cache hits */
    int32 CacheHits = 0;
    
    /** Number of cache misses */
    int32 CacheMisses = 0;
    
    /** Number of cached blueprints */
    int32 CachedCount = 0;
    
    /** Number of invalidated entries */
    int32 InvalidatedCount = 0;
    
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
        InvalidatedCount = 0;
    }
};

/**
 * Blueprint cache for performance optimization
 * Thread-safe caching of frequently accessed blueprints with statistics and warming
 */
class UNREALMCP_API FBlueprintCache
{
public:
    /**
     * Get a cached blueprint by name
     * @param BlueprintName - Name of the blueprint to retrieve
     * @return Cached blueprint or nullptr if not found/invalid
     */
    UBlueprint* GetBlueprint(const FString& BlueprintName);
    
    /**
     * Cache a blueprint for future lookups
     * @param BlueprintName - Name of the blueprint
     * @param Blueprint - Blueprint to cache
     */
    void CacheBlueprint(const FString& BlueprintName, UBlueprint* Blueprint);
    
    /**
     * Invalidate a cached blueprint (call when blueprint is modified)
     * @param BlueprintName - Name of the blueprint to invalidate
     */
    void InvalidateBlueprint(const FString& BlueprintName);
    
    /**
     * Clear all cached blueprints
     */
    void ClearCache();
    
    /**
     * Warm the cache with frequently used blueprints
     * @param BlueprintNames - Array of blueprint names to preload
     */
    void WarmCache(const TArray<FString>& BlueprintNames);
    
    /**
     * Warm the cache with common blueprint types
     * Preloads commonly used blueprint classes like PlayerController, GameMode, etc.
     */
    void WarmCacheWithCommonBlueprints();
    
    /**
     * Get cache statistics for monitoring
     * @return Current cache statistics
     */
    FBlueprintCacheStats GetCacheStats() const;
    
    /**
     * Reset cache statistics
     */
    void ResetCacheStats();
    
    /**
     * Get the number of cached blueprints
     * @return Number of currently cached blueprints
     */
    int32 GetCachedCount() const;
    
    /**
     * Check if a blueprint is cached
     * @param BlueprintName - Name of the blueprint to check
     * @return true if the blueprint is cached and valid
     */
    bool IsCached(const FString& BlueprintName) const;

private:
    /** Map of blueprint names to weak object pointers */
    TMap<FString, TWeakObjectPtr<UBlueprint>> CachedBlueprints;
    
    /** Cache statistics for monitoring */
    mutable FBlueprintCacheStats CacheStats;
    
    /** Critical section for thread safety */
    mutable FCriticalSection CacheLock;
    
    /**
     * Update cache statistics (must be called with lock held)
     * @param bWasHit - Whether this was a cache hit or miss
     */
    void UpdateStats(bool bWasHit) const;
    
    /**
     * Clean up invalid entries from cache (must be called with lock held)
     * @return Number of entries cleaned up
     */
    int32 CleanupInvalidEntries();
};

/**
 * Concrete implementation of IBlueprintService
 * Provides Blueprint creation, modification, and management functionality
 * with proper error handling, caching, and logging
 */
class UNREALMCP_API FBlueprintService : public IBlueprintService
{
public:
    /**
     * Get the singleton instance of the blueprint service
     * @return Reference to the singleton instance
     */
    static FBlueprintService& Get();
    
    // IBlueprintService interface implementation
    virtual UBlueprint* CreateBlueprint(const FBlueprintCreationParams& Params) override;
    virtual bool AddComponentToBlueprint(UBlueprint* Blueprint, const FComponentCreationParams& Params, FString& OutErrorMessage) override;
    virtual bool CompileBlueprint(UBlueprint* Blueprint, FString& OutError) override;
    virtual UBlueprint* FindBlueprint(const FString& BlueprintName) override;
    virtual bool AddVariableToBlueprint(UBlueprint* Blueprint, const FString& VariableName, const FString& VariableType, bool bIsExposed = false) override;
    virtual bool SetBlueprintProperty(UBlueprint* Blueprint, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutErrorMessage) override;
    virtual bool SetPhysicsProperties(UBlueprint* Blueprint, const FString& ComponentName, const TMap<FString, float>& PhysicsParams) override;
    virtual bool GetBlueprintComponents(UBlueprint* Blueprint, TArray<TPair<FString, FString>>& OutComponents) override;
    virtual bool SetStaticMeshProperties(UBlueprint* Blueprint, const FString& ComponentName, const FString& StaticMeshPath) override;
    virtual bool SetPawnProperties(UBlueprint* Blueprint, const TMap<FString, FString>& PawnParams) override;
    virtual bool AddInterfaceToBlueprint(UBlueprint* Blueprint, const FString& InterfaceName) override;
    virtual UBlueprint* CreateBlueprintInterface(const FString& InterfaceName, const FString& FolderPath) override;
    virtual bool CreateCustomBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TSharedPtr<FJsonObject>& FunctionParams) override;
    virtual bool SpawnBlueprintActor(UBlueprint* Blueprint, const FString& ActorName, const FVector& Location, const FRotator& Rotation) override;
    virtual bool CallBlueprintFunction(UBlueprint* Blueprint, const FString& FunctionName, const TArray<FString>& Parameters) override;

    /**
     * Convert string type to Blueprint pin type
     * Supports all Blueprint types including primitives, objects, structs, arrays, and custom types
     * @param TypeString - String representation of the type (e.g., "Actor", "Float", "Vector", "String[]", custom struct names)
     * @param OutPinType - Output pin type structure
     * @return true if conversion was successful
     */
    bool ConvertStringToPinType(const FString& TypeString, FEdGraphPinType& OutPinType) const;

private:
    /** Private constructor for singleton pattern */
    FBlueprintService() = default;
    
    /** Blueprint cache for performance optimization */
    FBlueprintCache BlueprintCache;
    
    /**
     * Resolve parent class from string representation
     * @param ParentClassName - String name of the parent class
     * @return UClass pointer or nullptr if not found
     */
    UClass* ResolveParentClass(const FString& ParentClassName) const;
    
    /**
     * Create directory structure for blueprint path
     * @param FolderPath - Folder path to create
     * @param OutError - Error message if creation fails
     * @return true if directories were created successfully
     */
    bool CreateDirectoryStructure(const FString& FolderPath, FString& OutError) const;
    
    /**
     * Normalize and validate blueprint path
     * @param InputPath - Input path to normalize
     * @param OutNormalizedPath - Normalized output path
     * @param OutError - Error message if validation fails
     * @return true if path is valid
     */
    bool NormalizeBlueprintPath(const FString& InputPath, FString& OutNormalizedPath, FString& OutError) const;
    
    /**
     * Get variable type from string representation
     * @param TypeString - String representation of the type
     * @return UClass/UStruct pointer for the type, or nullptr if not found
     */
    UObject* ResolveVariableType(const FString& TypeString) const;
};
