#include "Services/Blueprint/BlueprintCacheService.h"
#include "Utils/UnrealMCPCommonUtils.h"

// Blueprint Cache Implementation
UBlueprint* FBlueprintCache::GetBlueprint(const FString& BlueprintName)
{
    FScopeLock Lock(&CacheLock);

    // Update statistics
    UpdateStats(false); // Assume miss initially

    if (TWeakObjectPtr<UBlueprint>* CachedPtr = CachedBlueprints.Find(BlueprintName))
    {
        if (CachedPtr->IsValid())
        {
            // Update to hit since we found a valid entry
            CacheStats.CacheHits++;
            CacheStats.CacheMisses--;

            UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Cache hit for blueprint '%s'"), *BlueprintName);
            return CachedPtr->Get();
        }
        else
        {
            // Remove invalid entry
            CachedBlueprints.Remove(BlueprintName);
            CacheStats.CachedCount = CachedBlueprints.Num();
            UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Removed invalid cache entry for blueprint '%s'"), *BlueprintName);
        }
    }

    return nullptr;
}

void FBlueprintCache::CacheBlueprint(const FString& BlueprintName, UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return;
    }

    FScopeLock Lock(&CacheLock);
    CachedBlueprints.Add(BlueprintName, Blueprint);
    CacheStats.CachedCount = CachedBlueprints.Num();
    UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Cached blueprint '%s'"), *BlueprintName);
}

void FBlueprintCache::InvalidateBlueprint(const FString& BlueprintName)
{
    FScopeLock Lock(&CacheLock);
    if (CachedBlueprints.Remove(BlueprintName) > 0)
    {
        CacheStats.InvalidatedCount++;
        CacheStats.CachedCount = CachedBlueprints.Num();
        UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Invalidated cache for blueprint '%s'"), *BlueprintName);
    }
}

void FBlueprintCache::ClearCache()
{
    FScopeLock Lock(&CacheLock);
    int32 ClearedCount = CachedBlueprints.Num();
    CachedBlueprints.Empty();
    CacheStats.CachedCount = 0;
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cleared %d cached blueprints"), ClearedCount);
}

void FBlueprintCache::WarmCache(const TArray<FString>& BlueprintNames)
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Warming cache with %d blueprints"), BlueprintNames.Num());

    for (const FString& BlueprintName : BlueprintNames)
    {
        // Check if already cached
        if (IsCached(BlueprintName))
        {
            continue;
        }

        // Try to find and cache the blueprint
        UBlueprint* FoundBlueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
        if (FoundBlueprint)
        {
            CacheBlueprint(BlueprintName, FoundBlueprint);
            UE_LOG(LogTemp, Verbose, TEXT("FBlueprintCache: Warmed cache with blueprint '%s'"), *BlueprintName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FBlueprintCache: Could not find blueprint '%s' for cache warming"), *BlueprintName);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cache warming complete. %d blueprints cached"), GetCachedCount());
}

void FBlueprintCache::WarmCacheWithCommonBlueprints()
{
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Warming cache with common blueprints"));

    // Common blueprint names that are frequently used
    TArray<FString> CommonBlueprints = {
        TEXT("BP_PlayerController"),
        TEXT("BP_GameMode"),
        TEXT("BP_GameState"),
        TEXT("BP_PlayerState"),
        TEXT("BP_Character"),
        TEXT("BP_Pawn"),
        TEXT("BP_Actor"),
        TEXT("BP_HUD"),
        TEXT("BP_Widget"),
        TEXT("BP_UserWidget"),
        TEXT("ThirdPersonCharacter"),
        TEXT("BP_ThirdPersonCharacter"),
        TEXT("FirstPersonCharacter"),
        TEXT("BP_FirstPersonCharacter")
    };

    WarmCache(CommonBlueprints);
}

FBlueprintCacheStats FBlueprintCache::GetCacheStats() const
{
    FScopeLock Lock(&CacheLock);
    FBlueprintCacheStats StatsCopy = CacheStats;
    StatsCopy.CachedCount = CachedBlueprints.Num();
    return StatsCopy;
}

void FBlueprintCache::ResetCacheStats()
{
    FScopeLock Lock(&CacheLock);
    CacheStats.Reset();
    CacheStats.CachedCount = CachedBlueprints.Num();
    UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cache statistics reset"));
}

int32 FBlueprintCache::GetCachedCount() const
{
    FScopeLock Lock(&CacheLock);
    return CachedBlueprints.Num();
}

bool FBlueprintCache::IsCached(const FString& BlueprintName) const
{
    FScopeLock Lock(&CacheLock);

    if (const TWeakObjectPtr<UBlueprint>* CachedPtr = CachedBlueprints.Find(BlueprintName))
    {
        return CachedPtr->IsValid();
    }

    return false;
}

void FBlueprintCache::UpdateStats(bool bWasHit) const
{
    // This method assumes the lock is already held
    CacheStats.TotalRequests++;
    if (bWasHit)
    {
        CacheStats.CacheHits++;
    }
    else
    {
        CacheStats.CacheMisses++;
    }
}

int32 FBlueprintCache::CleanupInvalidEntries()
{
    // This method assumes the lock is already held
    int32 CleanedCount = 0;

    for (auto It = CachedBlueprints.CreateIterator(); It; ++It)
    {
        if (!It.Value().IsValid())
        {
            It.RemoveCurrent();
            CleanedCount++;
        }
    }

    if (CleanedCount > 0)
    {
        CacheStats.CachedCount = CachedBlueprints.Num();
        UE_LOG(LogTemp, Log, TEXT("FBlueprintCache: Cleaned up %d invalid cache entries"), CleanedCount);
    }

    return CleanedCount;
}
