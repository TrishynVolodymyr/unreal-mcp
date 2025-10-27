#include "Services/ObjectPoolManager.h"

FObjectPoolManager& FObjectPoolManager::Get()
{
    static FObjectPoolManager Instance;
    return Instance;
}

FObjectPoolManager::~FObjectPoolManager()
{
    Shutdown();
}

void FObjectPoolManager::Initialize()
{
    FScopeLock Lock(&ManagerLock);
    
    if (bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("FObjectPoolManager::Initialize: Already initialized"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::Initialize: Initializing object pools"));
    
    // Create object pools with default sizes
    JsonObjectPool = MakeUnique<TObjectPool<FPoolableJsonObject>>(50, 10);
    MCPResponsePool = MakeUnique<TObjectPool<FPoolableMCPResponse>>(100, 20);
    ParameterValidatorPool = MakeUnique<TObjectPool<FPoolableParameterValidator>>(30, 5);
    JsonValuePool = MakeUnique<TObjectPool<FPoolableJsonValue>>(200, 50);
    
    bInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::Initialize: Object pools initialized successfully"));
}

void FObjectPoolManager::Shutdown()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::Shutdown: Shutting down object pools"));
    
    // Get final statistics before shutdown
    FObjectPoolManagerStats FinalStats = GetCombinedStats();
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::Shutdown: Final stats - Total Requests: %d, Total Hits: %d, Hit Ratio: %.2f%%, Total Pooled: %d"),
        FinalStats.GetTotalRequests(), FinalStats.GetTotalHits(), FinalStats.GetOverallHitRatio() * 100.0f, FinalStats.GetTotalPooledObjects());
    
    // Destroy pools
    JsonObjectPool.Reset();
    MCPResponsePool.Reset();
    ParameterValidatorPool.Reset();
    JsonValuePool.Reset();
    
    bInitialized = false;
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::Shutdown: Object pools shut down successfully"));
}

TSharedPtr<FPoolableJsonObject> FObjectPoolManager::GetJsonObject()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !JsonObjectPool.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FObjectPoolManager::GetJsonObject: Pool not initialized"));
        return MakeShared<FPoolableJsonObject>();
    }
    
    return JsonObjectPool->GetObject();
}

void FObjectPoolManager::ReturnJsonObject(TSharedPtr<FPoolableJsonObject> Object)
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !JsonObjectPool.IsValid())
    {
        return;
    }
    
    JsonObjectPool->ReturnObject(Object);
}

TSharedPtr<FPoolableMCPResponse> FObjectPoolManager::GetMCPResponse()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !MCPResponsePool.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FObjectPoolManager::GetMCPResponse: Pool not initialized"));
        return MakeShared<FPoolableMCPResponse>();
    }
    
    return MCPResponsePool->GetObject();
}

void FObjectPoolManager::ReturnMCPResponse(TSharedPtr<FPoolableMCPResponse> Response)
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !MCPResponsePool.IsValid())
    {
        return;
    }
    
    MCPResponsePool->ReturnObject(Response);
}

TSharedPtr<FPoolableParameterValidator> FObjectPoolManager::GetParameterValidator()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !ParameterValidatorPool.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FObjectPoolManager::GetParameterValidator: Pool not initialized"));
        return MakeShared<FPoolableParameterValidator>();
    }
    
    return ParameterValidatorPool->GetObject();
}

void FObjectPoolManager::ReturnParameterValidator(TSharedPtr<FPoolableParameterValidator> Validator)
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !ParameterValidatorPool.IsValid())
    {
        return;
    }
    
    ParameterValidatorPool->ReturnObject(Validator);
}

TSharedPtr<FPoolableJsonValue> FObjectPoolManager::GetJsonValue()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !JsonValuePool.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FObjectPoolManager::GetJsonValue: Pool not initialized"));
        return MakeShared<FPoolableJsonValue>();
    }
    
    return JsonValuePool->GetObject();
}

void FObjectPoolManager::ReturnJsonValue(TSharedPtr<FPoolableJsonValue> Value)
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized || !JsonValuePool.IsValid())
    {
        return;
    }
    
    JsonValuePool->ReturnObject(Value);
}

FObjectPoolManagerStats FObjectPoolManager::GetCombinedStats() const
{
    FScopeLock Lock(&ManagerLock);
    
    FObjectPoolManagerStats CombinedStats;
    
    if (bInitialized)
    {
        if (JsonObjectPool.IsValid())
        {
            CombinedStats.JsonObjectStats = JsonObjectPool->GetStats();
        }
        
        if (MCPResponsePool.IsValid())
        {
            CombinedStats.MCPResponseStats = MCPResponsePool->GetStats();
        }
        
        if (ParameterValidatorPool.IsValid())
        {
            CombinedStats.ParameterValidatorStats = ParameterValidatorPool->GetStats();
        }
        
        if (JsonValuePool.IsValid())
        {
            CombinedStats.JsonValueStats = JsonValuePool->GetStats();
        }
    }
    
    return CombinedStats;
}

void FObjectPoolManager::ResetAllStats()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::ResetAllStats: Resetting statistics for all pools"));
    
    if (JsonObjectPool.IsValid())
    {
        JsonObjectPool->ResetStats();
    }
    
    if (MCPResponsePool.IsValid())
    {
        MCPResponsePool->ResetStats();
    }
    
    if (ParameterValidatorPool.IsValid())
    {
        ParameterValidatorPool->ResetStats();
    }
    
    if (JsonValuePool.IsValid())
    {
        JsonValuePool->ResetStats();
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::ResetAllStats: All pool statistics reset"));
}

void FObjectPoolManager::ClearAllPools()
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::ClearAllPools: Clearing all object pools"));
    
    if (JsonObjectPool.IsValid())
    {
        JsonObjectPool->ClearPool();
    }
    
    if (MCPResponsePool.IsValid())
    {
        MCPResponsePool->ClearPool();
    }
    
    if (ParameterValidatorPool.IsValid())
    {
        ParameterValidatorPool->ClearPool();
    }
    
    if (JsonValuePool.IsValid())
    {
        JsonValuePool->ClearPool();
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::ClearAllPools: All pools cleared"));
}

void FObjectPoolManager::ConfigurePoolSizes(int32 JsonObjectPoolSize, 
                                           int32 MCPResponsePoolSize, 
                                           int32 ParameterValidatorPoolSize,
                                           int32 JsonValuePoolSize)
{
    FScopeLock Lock(&ManagerLock);
    
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("FObjectPoolManager::ConfigurePoolSizes: Pools not initialized"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::ConfigurePoolSizes: Configuring pool sizes - JSON: %d, Response: %d, Validator: %d, Value: %d"),
        JsonObjectPoolSize, MCPResponsePoolSize, ParameterValidatorPoolSize, JsonValuePoolSize);
    
    if (JsonObjectPool.IsValid())
    {
        JsonObjectPool->SetMaxPoolSize(JsonObjectPoolSize);
    }
    
    if (MCPResponsePool.IsValid())
    {
        MCPResponsePool->SetMaxPoolSize(MCPResponsePoolSize);
    }
    
    if (ParameterValidatorPool.IsValid())
    {
        ParameterValidatorPool->SetMaxPoolSize(ParameterValidatorPoolSize);
    }
    
    if (JsonValuePool.IsValid())
    {
        JsonValuePool->SetMaxPoolSize(JsonValuePoolSize);
    }
    
    UE_LOG(LogTemp, Log, TEXT("FObjectPoolManager::ConfigurePoolSizes: Pool sizes configured successfully"));
}

void FObjectPoolManager::GetPoolSizes(int32& OutJsonObjectPoolSize, 
                                     int32& OutMCPResponsePoolSize, 
                                     int32& OutParameterValidatorPoolSize,
                                     int32& OutJsonValuePoolSize) const
{
    FScopeLock Lock(&ManagerLock);
    
    OutJsonObjectPoolSize = 0;
    OutMCPResponsePoolSize = 0;
    OutParameterValidatorPoolSize = 0;
    OutJsonValuePoolSize = 0;
    
    if (bInitialized)
    {
        if (JsonObjectPool.IsValid())
        {
            OutJsonObjectPoolSize = JsonObjectPool->GetMaxPoolSize();
        }
        
        if (MCPResponsePool.IsValid())
        {
            OutMCPResponsePoolSize = MCPResponsePool->GetMaxPoolSize();
        }
        
        if (ParameterValidatorPool.IsValid())
        {
            OutParameterValidatorPoolSize = ParameterValidatorPool->GetMaxPoolSize();
        }
        
        if (JsonValuePool.IsValid())
        {
            OutJsonValuePoolSize = JsonValuePool->GetMaxPoolSize();
        }
    }
}
