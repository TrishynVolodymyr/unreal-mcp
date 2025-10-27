#pragma once

#include "CoreMinimal.h"
#include "Services/ObjectPool.h"
#include "Services/PoolableObjects.h"

/**
 * Statistics for all object pools combined
 */
struct UNREALMCP_API FObjectPoolManagerStats
{
    /** Statistics for JSON object pool */
    FObjectPoolStats JsonObjectStats;
    
    /** Statistics for MCP response pool */
    FObjectPoolStats MCPResponseStats;
    
    /** Statistics for parameter validator pool */
    FObjectPoolStats ParameterValidatorStats;
    
    /** Statistics for JSON value pool */
    FObjectPoolStats JsonValueStats;
    
    /** Total number of requests across all pools */
    int32 GetTotalRequests() const
    {
        return JsonObjectStats.TotalRequests + MCPResponseStats.TotalRequests + 
               ParameterValidatorStats.TotalRequests + JsonValueStats.TotalRequests;
    }
    
    /** Total number of hits across all pools */
    int32 GetTotalHits() const
    {
        return JsonObjectStats.PoolHits + MCPResponseStats.PoolHits + 
               ParameterValidatorStats.PoolHits + JsonValueStats.PoolHits;
    }
    
    /** Overall hit ratio across all pools */
    float GetOverallHitRatio() const
    {
        int32 TotalRequests = GetTotalRequests();
        return TotalRequests > 0 ? static_cast<float>(GetTotalHits()) / static_cast<float>(TotalRequests) : 0.0f;
    }
    
    /** Total number of pooled objects across all pools */
    int32 GetTotalPooledObjects() const
    {
        return JsonObjectStats.PooledCount + MCPResponseStats.PooledCount + 
               ParameterValidatorStats.PooledCount + JsonValueStats.PooledCount;
    }
};

/**
 * Manager for all object pools in the MCP system
 * Provides centralized access to different object pools with monitoring and configuration
 */
class UNREALMCP_API FObjectPoolManager
{
public:
    /**
     * Get the singleton instance of the object pool manager
     * @return Reference to the singleton instance
     */
    static FObjectPoolManager& Get();
    
    /**
     * Initialize all object pools with default settings
     */
    void Initialize();
    
    /**
     * Shutdown and cleanup all object pools
     */
    void Shutdown();
    
    /**
     * Get a JSON object from the pool
     * @return Shared pointer to poolable JSON object
     */
    TSharedPtr<FPoolableJsonObject> GetJsonObject();
    
    /**
     * Return a JSON object to the pool
     * @param Object - Object to return to pool
     */
    void ReturnJsonObject(TSharedPtr<FPoolableJsonObject> Object);
    
    /**
     * Get an MCP response from the pool
     * @return Shared pointer to poolable MCP response
     */
    TSharedPtr<FPoolableMCPResponse> GetMCPResponse();
    
    /**
     * Return an MCP response to the pool
     * @param Response - Response to return to pool
     */
    void ReturnMCPResponse(TSharedPtr<FPoolableMCPResponse> Response);
    
    /**
     * Get a parameter validator from the pool
     * @return Shared pointer to poolable parameter validator
     */
    TSharedPtr<FPoolableParameterValidator> GetParameterValidator();
    
    /**
     * Return a parameter validator to the pool
     * @param Validator - Validator to return to pool
     */
    void ReturnParameterValidator(TSharedPtr<FPoolableParameterValidator> Validator);
    
    /**
     * Get a JSON value from the pool
     * @return Shared pointer to poolable JSON value
     */
    TSharedPtr<FPoolableJsonValue> GetJsonValue();
    
    /**
     * Return a JSON value to the pool
     * @param Value - Value to return to pool
     */
    void ReturnJsonValue(TSharedPtr<FPoolableJsonValue> Value);
    
    /**
     * Get combined statistics for all pools
     * @return Combined statistics
     */
    FObjectPoolManagerStats GetCombinedStats() const;
    
    /**
     * Reset statistics for all pools
     */
    void ResetAllStats();
    
    /**
     * Clear all object pools
     */
    void ClearAllPools();
    
    /**
     * Configure pool sizes for all pools
     * @param JsonObjectPoolSize - Max size for JSON object pool
     * @param MCPResponsePoolSize - Max size for MCP response pool
     * @param ParameterValidatorPoolSize - Max size for parameter validator pool
     * @param JsonValuePoolSize - Max size for JSON value pool
     */
    void ConfigurePoolSizes(int32 JsonObjectPoolSize = 50, 
                           int32 MCPResponsePoolSize = 100, 
                           int32 ParameterValidatorPoolSize = 30,
                           int32 JsonValuePoolSize = 200);
    
    /**
     * Get current pool sizes
     * @param OutJsonObjectPoolSize - Current JSON object pool size
     * @param OutMCPResponsePoolSize - Current MCP response pool size
     * @param OutParameterValidatorPoolSize - Current parameter validator pool size
     * @param OutJsonValuePoolSize - Current JSON value pool size
     */
    void GetPoolSizes(int32& OutJsonObjectPoolSize, 
                     int32& OutMCPResponsePoolSize, 
                     int32& OutParameterValidatorPoolSize,
                     int32& OutJsonValuePoolSize) const;
    
    /**
     * Check if pools are initialized
     * @return true if pools are initialized
     */
    bool IsInitialized() const { return bInitialized; }

private:
    /** Private constructor for singleton pattern */
    FObjectPoolManager() = default;
    
    /** Destructor */
    ~FObjectPoolManager();
    
    /** JSON object pool */
    TUniquePtr<TObjectPool<FPoolableJsonObject>> JsonObjectPool;
    
    /** MCP response pool */
    TUniquePtr<TObjectPool<FPoolableMCPResponse>> MCPResponsePool;
    
    /** Parameter validator pool */
    TUniquePtr<TObjectPool<FPoolableParameterValidator>> ParameterValidatorPool;
    
    /** JSON value pool */
    TUniquePtr<TObjectPool<FPoolableJsonValue>> JsonValuePool;
    
    /** Whether pools are initialized */
    bool bInitialized = false;
    
    /** Critical section for thread safety */
    mutable FCriticalSection ManagerLock;
};
