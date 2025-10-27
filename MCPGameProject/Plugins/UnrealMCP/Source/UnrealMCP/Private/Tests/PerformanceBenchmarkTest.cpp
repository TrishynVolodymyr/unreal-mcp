#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/UnrealMCPMainDispatcher.h"
#include "Factories/ComponentFactory.h"
#include "Factories/WidgetFactory.h"
#include "Services/ComponentService.h"
#include "Misc/DateTime.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformMemory.h"

// Forward declarations
void BenchmarkCommandRegistryPerformance();
void BenchmarkComponentFactoryPerformance();
void BenchmarkWidgetFactoryPerformance();
void BenchmarkServiceLayerPerformance();
void BenchmarkEndToEndCommandPerformance();

/**
 * Performance benchmark test for the refactored MCP system
 * Measures execution times and memory usage of various operations
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void RunPerformanceBenchmarkTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Performance Benchmark Test Started ==="));
    
    // Get initial memory stats
    FPlatformMemoryStats InitialMemory = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Initial Memory Usage: %.2f MB"), 
           InitialMemory.UsedPhysical / (1024.0f * 1024.0f));
    
    // Benchmark 1: Command Registry Performance
    BenchmarkCommandRegistryPerformance();
    
    // Benchmark 2: Component Factory Performance
    BenchmarkComponentFactoryPerformance();
    
    // Benchmark 3: Widget Factory Performance
    BenchmarkWidgetFactoryPerformance();
    
    // Benchmark 4: Service Layer Performance
    BenchmarkServiceLayerPerformance();
    
    // Benchmark 5: End-to-End Command Performance
    BenchmarkEndToEndCommandPerformance();
    
    // Get final memory stats
    FPlatformMemoryStats FinalMemory = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Final Memory Usage: %.2f MB"), 
           FinalMemory.UsedPhysical / (1024.0f * 1024.0f));
    
    float MemoryDelta = (FinalMemory.UsedPhysical - InitialMemory.UsedPhysical) / (1024.0f * 1024.0f);
    UE_LOG(LogTemp, Warning, TEXT("Memory Delta: %.2f MB"), MemoryDelta);
    
    UE_LOG(LogTemp, Warning, TEXT("=== Performance Benchmark Test Completed ==="));
}

/**
 * Benchmark command registry performance
 */
void BenchmarkCommandRegistryPerformance()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Command Registry Performance Benchmark ---"));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    // Test 1: Command lookup performance
    const int32 NumLookups = 10000;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumLookups; ++i)
    {
        Registry.IsCommandRegistered(TEXT("create_blueprint"));
        Registry.IsCommandRegistered(TEXT("add_component_to_blueprint"));
        Registry.IsCommandRegistered(TEXT("compile_blueprint"));
        Registry.IsCommandRegistered(TEXT("create_umg_widget_blueprint"));
        Registry.IsCommandRegistered(TEXT("add_widget_component_to_widget"));
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double LookupsPerSecond = (NumLookups * 5) / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Command lookups: %.0f lookups/second (%.4f ms avg)"), 
           LookupsPerSecond, Duration.GetTotalMilliseconds() / (NumLookups * 5));
    
    // Test 2: Get all commands performance
    StartTime = FDateTime::Now();
    for (int32 i = 0; i < 1000; ++i)
    {
        TArray<FString> Commands = Registry.GetRegisteredCommandNames();
    }
    EndTime = FDateTime::Now();
    Duration = EndTime - StartTime;
    double GetAllPerSecond = 1000 / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Get all commands: %.0f calls/second (%.4f ms avg)"), 
           GetAllPerSecond, Duration.GetTotalMilliseconds() / 1000);
}

/**
 * Benchmark component factory performance
 */
void BenchmarkComponentFactoryPerformance()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Component Factory Performance Benchmark ---"));
    
    FComponentFactory& Factory = FComponentFactory::Get();
    
    // Test 1: Component class lookup performance
    const int32 NumLookups = 10000;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumLookups; ++i)
    {
        Factory.GetComponentClass(TEXT("StaticMeshComponent"));
        Factory.GetComponentClass(TEXT("PointLightComponent"));
        Factory.GetComponentClass(TEXT("SphereComponent"));
        Factory.GetComponentClass(TEXT("BoxComponent"));
        Factory.GetComponentClass(TEXT("CapsuleComponent"));
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double LookupsPerSecond = (NumLookups * 5) / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Component class lookups: %.0f lookups/second (%.4f ms avg)"), 
           LookupsPerSecond, Duration.GetTotalMilliseconds() / (NumLookups * 5));
    
    // Test 2: Get available types performance
    StartTime = FDateTime::Now();
    for (int32 i = 0; i < 1000; ++i)
    {
        TArray<FString> Types = Factory.GetAvailableTypes();
    }
    EndTime = FDateTime::Now();
    Duration = EndTime - StartTime;
    double GetTypesPerSecond = 1000 / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Get available types: %.0f calls/second (%.4f ms avg)"), 
           GetTypesPerSecond, Duration.GetTotalMilliseconds() / 1000);
}

/**
 * Benchmark widget factory performance
 */
void BenchmarkWidgetFactoryPerformance()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Widget Factory Performance Benchmark ---"));
    
    FWidgetFactory& Factory = FWidgetFactory::Get();
    
    // Test 1: Get available widget types performance
    const int32 NumCalls = 5000;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumCalls; ++i)
    {
        TArray<FString> Types = Factory.GetAvailableWidgetTypes();
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double CallsPerSecond = NumCalls / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Get widget types: %.0f calls/second (%.4f ms avg)"), 
           CallsPerSecond, Duration.GetTotalMilliseconds() / NumCalls);
}

/**
 * Benchmark service layer performance
 */
void BenchmarkServiceLayerPerformance()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Service Layer Performance Benchmark ---"));
    
    FComponentService& ComponentService = FComponentService::Get();
    
    // Test 1: Component class resolution performance
    const int32 NumCalls = 5000;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumCalls; ++i)
    {
        ComponentService.GetComponentClass(TEXT("StaticMeshComponent"));
        ComponentService.IsValidComponentType(TEXT("PointLightComponent"));
        ComponentService.GetComponentClass(TEXT("SphereComponent"));
        ComponentService.IsValidComponentType(TEXT("BoxComponent"));
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double CallsPerSecond = (NumCalls * 4) / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Service layer calls: %.0f calls/second (%.4f ms avg)"), 
           CallsPerSecond, Duration.GetTotalMilliseconds() / (NumCalls * 4));
}

/**
 * Benchmark end-to-end command performance
 */
void BenchmarkEndToEndCommandPerformance()
{
    UE_LOG(LogTemp, Warning, TEXT("--- End-to-End Command Performance Benchmark ---"));
    
    FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
    
    // Test 1: Simple command execution performance
    TSharedPtr<FJsonObject> TestParams = MakeShareable(new FJsonObject);
    
    const int32 NumCommands = 100;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumCommands; ++i)
    {
        // Test get available commands (lightweight operation)
        TSharedPtr<FJsonObject> Response = Dispatcher.GetAvailableCommands();
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double CommandsPerSecond = NumCommands / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Lightweight commands: %.0f commands/second (%.4f ms avg)"), 
           CommandsPerSecond, Duration.GetTotalMilliseconds() / NumCommands);
    
    // Test 2: Blueprint creation performance (heavier operation)
    StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < 10; ++i)
    {
        TSharedPtr<FJsonObject> CreateParams = MakeShareable(new FJsonObject);
        CreateParams->SetStringField(TEXT("name"), FString::Printf(TEXT("BP_PerfTest_%d"), i));
        CreateParams->SetStringField(TEXT("parent_class"), TEXT("Actor"));
        CreateParams->SetStringField(TEXT("folder_path"), TEXT("PerfTest"));
        
        TSharedPtr<FJsonObject> Response = Dispatcher.HandleCommand(TEXT("create_blueprint"), CreateParams);
    }
    
    EndTime = FDateTime::Now();
    Duration = EndTime - StartTime;
    double HeavyCommandsPerSecond = 10 / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Heavy commands (Blueprint creation): %.2f commands/second (%.2f ms avg)"), 
           HeavyCommandsPerSecond, Duration.GetTotalMilliseconds() / 10);
}

/**
 * Memory usage benchmark
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void RunMemoryUsageBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Memory Usage Benchmark Started ==="));
    
    FPlatformMemoryStats InitialStats = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Initial Memory: %.2f MB"), 
           InitialStats.UsedPhysical / (1024.0f * 1024.0f));
    
    // Create many JSON objects to test memory management
    TArray<TSharedPtr<FJsonObject>> TestObjects;
    TestObjects.Reserve(10000);
    
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < 10000; ++i)
    {
        TSharedPtr<FJsonObject> TestObj = MakeShareable(new FJsonObject);
        TestObj->SetStringField(TEXT("test_field"), FString::Printf(TEXT("test_value_%d"), i));
        TestObj->SetNumberField(TEXT("test_number"), i);
        TestObj->SetBoolField(TEXT("test_bool"), i % 2 == 0);
        
        // Add nested object
        TSharedPtr<FJsonObject> NestedObj = MakeShareable(new FJsonObject);
        NestedObj->SetStringField(TEXT("nested_field"), FString::Printf(TEXT("nested_%d"), i));
        TestObj->SetObjectField(TEXT("nested"), NestedObj);
        
        TestObjects.Add(TestObj);
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan CreationTime = EndTime - StartTime;
    
    FPlatformMemoryStats PeakStats = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Peak Memory: %.2f MB"), 
           PeakStats.UsedPhysical / (1024.0f * 1024.0f));
    
    UE_LOG(LogTemp, Warning, TEXT("Object creation time: %.2f ms"), CreationTime.GetTotalMilliseconds());
    
    // Clear objects and measure cleanup
    StartTime = FDateTime::Now();
    TestObjects.Empty();
    EndTime = FDateTime::Now();
    FTimespan CleanupTime = EndTime - StartTime;
    
    // Force garbage collection
    GEngine->ForceGarbageCollection(true);
    
    FPlatformMemoryStats FinalStats = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Final Memory: %.2f MB"), 
           FinalStats.UsedPhysical / (1024.0f * 1024.0f));
    
    float MemoryDelta = (FinalStats.UsedPhysical - InitialStats.UsedPhysical) / (1024.0f * 1024.0f);
    UE_LOG(LogTemp, Warning, TEXT("Memory Delta: %.2f MB"), MemoryDelta);
    UE_LOG(LogTemp, Warning, TEXT("Cleanup time: %.2f ms"), CleanupTime.GetTotalMilliseconds());
    
    if (MemoryDelta < 5.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("✓ Memory management: EXCELLENT (delta < 5MB)"));
    }
    else if (MemoryDelta < 20.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("✓ Memory management: GOOD (delta < 20MB)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ Memory management: NEEDS ATTENTION (delta > 20MB)"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== Memory Usage Benchmark Completed ==="));
}