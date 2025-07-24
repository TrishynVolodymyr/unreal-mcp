#pragma once

#include "CoreMinimal.h"

/**
 * Statistics for monitoring object pool performance
 */
struct UNREALMCP_API FObjectPoolStats
{
    /** Total number of objects requested from pool */
    int32 TotalRequests = 0;
    
    /** Number of objects served from pool (reused) */
    int32 PoolHits = 0;
    
    /** Number of objects created new */
    int32 PoolMisses = 0;
    
    /** Current number of objects in pool */
    int32 PooledCount = 0;
    
    /** Maximum number of objects ever in pool */
    int32 MaxPooledCount = 0;
    
    /** Total number of objects returned to pool */
    int32 TotalReturns = 0;
    
    /** Number of objects discarded due to pool being full */
    int32 DiscardedCount = 0;
    
    /** Pool hit ratio (0.0 to 1.0) */
    float GetHitRatio() const
    {
        return TotalRequests > 0 ? static_cast<float>(PoolHits) / static_cast<float>(TotalRequests) : 0.0f;
    }
    
    /** Reset all statistics */
    void Reset()
    {
        TotalRequests = 0;
        PoolHits = 0;
        PoolMisses = 0;
        PooledCount = 0;
        MaxPooledCount = 0;
        TotalReturns = 0;
        DiscardedCount = 0;
    }
};

/**
 * Generic thread-safe object pool template
 * Provides efficient reuse of frequently created objects with automatic cleanup
 * 
 * @param T - Type of objects to pool (must have default constructor and Reset() method)
 */
template<typename T>
class UNREALMCP_API TObjectPool
{
public:
    /**
     * Constructor
     * @param InMaxPoolSize - Maximum number of objects to keep in pool
     * @param InInitialPoolSize - Number of objects to pre-allocate
     */
    explicit TObjectPool(int32 InMaxPoolSize = 50, int32 InInitialPoolSize = 10)
        : MaxPoolSize(InMaxPoolSize)
    {
        // Pre-allocate initial objects
        for (int32 i = 0; i < InInitialPoolSize; ++i)
        {
            TSharedPtr<T> NewObject = MakeShared<T>();
            AvailableObjects.Add(NewObject);
        }
        
        Stats.PooledCount = AvailableObjects.Num();
        Stats.MaxPooledCount = Stats.PooledCount;
        
        UE_LOG(LogTemp, Log, TEXT("TObjectPool: Created pool with %d pre-allocated objects (max: %d)"), 
            InInitialPoolSize, InMaxPoolSize);
    }
    
    /**
     * Destructor - cleans up all pooled objects
     */
    ~TObjectPool()
    {
        FScopeLock Lock(&PoolLock);
        int32 CleanedCount = AvailableObjects.Num();
        AvailableObjects.Empty();
        UE_LOG(LogTemp, Log, TEXT("TObjectPool: Destroyed pool, cleaned up %d objects"), CleanedCount);
    }
    
    /**
     * Get an object from the pool (reused if available, created if not)
     * @return Shared pointer to object ready for use
     */
    TSharedPtr<T> GetObject()
    {
        FScopeLock Lock(&PoolLock);
        
        Stats.TotalRequests++;
        
        if (AvailableObjects.Num() > 0)
        {
            // Reuse existing object
            TSharedPtr<T> ReusedObject = AvailableObjects.Pop();
            Stats.PoolHits++;
            Stats.PooledCount = AvailableObjects.Num();
            
            // Reset the object to clean state
            if (ReusedObject.IsValid())
            {
                ResetObject(ReusedObject.Get());
            }
            
            UE_LOG(LogTemp, VeryVerbose, TEXT("TObjectPool: Reused object from pool (%d remaining)"), AvailableObjects.Num());
            return ReusedObject;
        }
        else
        {
            // Create new object
            TSharedPtr<T> NewObject = MakeShared<T>();
            Stats.PoolMisses++;
            
            UE_LOG(LogTemp, VeryVerbose, TEXT("TObjectPool: Created new object (pool empty)"));
            return NewObject;
        }
    }
    
    /**
     * Return an object to the pool for reuse
     * @param Object - Object to return to pool
     */
    void ReturnObject(TSharedPtr<T> Object)
    {
        if (!Object.IsValid())
        {
            return;
        }
        
        FScopeLock Lock(&PoolLock);
        
        Stats.TotalReturns++;
        
        if (AvailableObjects.Num() < MaxPoolSize)
        {
            // Add to pool for reuse
            AvailableObjects.Add(Object);
            Stats.PooledCount = AvailableObjects.Num();
            Stats.MaxPooledCount = FMath::Max(Stats.MaxPooledCount, Stats.PooledCount);
            
            UE_LOG(LogTemp, VeryVerbose, TEXT("TObjectPool: Returned object to pool (%d total)"), AvailableObjects.Num());
        }
        else
        {
            // Pool is full, discard object
            Stats.DiscardedCount++;
            UE_LOG(LogTemp, VeryVerbose, TEXT("TObjectPool: Discarded object (pool full at %d)"), MaxPoolSize);
        }
    }
    
    /**
     * Clear all objects from the pool
     */
    void ClearPool()
    {
        FScopeLock Lock(&PoolLock);
        int32 ClearedCount = AvailableObjects.Num();
        AvailableObjects.Empty();
        Stats.PooledCount = 0;
        UE_LOG(LogTemp, Log, TEXT("TObjectPool: Cleared pool, removed %d objects"), ClearedCount);
    }
    
    /**
     * Get current pool statistics
     * @return Copy of current statistics
     */
    FObjectPoolStats GetStats() const
    {
        FScopeLock Lock(&PoolLock);
        FObjectPoolStats StatsCopy = Stats;
        StatsCopy.PooledCount = AvailableObjects.Num();
        return StatsCopy;
    }
    
    /**
     * Reset pool statistics
     */
    void ResetStats()
    {
        FScopeLock Lock(&PoolLock);
        Stats.Reset();
        Stats.PooledCount = AvailableObjects.Num();
        Stats.MaxPooledCount = Stats.PooledCount;
        UE_LOG(LogTemp, Log, TEXT("TObjectPool: Statistics reset"));
    }
    
    /**
     * Get current number of available objects in pool
     * @return Number of objects ready for reuse
     */
    int32 GetAvailableCount() const
    {
        FScopeLock Lock(&PoolLock);
        return AvailableObjects.Num();
    }
    
    /**
     * Get maximum pool size
     * @return Maximum number of objects that can be pooled
     */
    int32 GetMaxPoolSize() const
    {
        return MaxPoolSize;
    }
    
    /**
     * Set maximum pool size (will trim pool if necessary)
     * @param NewMaxSize - New maximum pool size
     */
    void SetMaxPoolSize(int32 NewMaxSize)
    {
        FScopeLock Lock(&PoolLock);
        MaxPoolSize = NewMaxSize;
        
        // Trim pool if it's now too large
        while (AvailableObjects.Num() > MaxPoolSize)
        {
            AvailableObjects.Pop();
            Stats.DiscardedCount++;
        }
        
        Stats.PooledCount = AvailableObjects.Num();
        UE_LOG(LogTemp, Log, TEXT("TObjectPool: Set max pool size to %d (current: %d)"), NewMaxSize, Stats.PooledCount);
    }

private:
    /** Array of available objects for reuse */
    TArray<TSharedPtr<T>> AvailableObjects;
    
    /** Maximum number of objects to keep in pool */
    int32 MaxPoolSize;
    
    /** Pool statistics */
    mutable FObjectPoolStats Stats;
    
    /** Critical section for thread safety */
    mutable FCriticalSection PoolLock;
    
    /**
     * Reset an object to clean state for reuse
     * Calls Reset() method if available, otherwise does nothing
     * @param Object - Object to reset
     */
    void ResetObject(T* Object)
    {
        if (Object)
        {
            // Call Reset() method - all poolable objects must implement this
            Object->Reset();
        }
    }
};
