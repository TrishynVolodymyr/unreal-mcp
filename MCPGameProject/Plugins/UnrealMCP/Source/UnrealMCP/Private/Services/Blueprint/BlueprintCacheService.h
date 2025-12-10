#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"

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
