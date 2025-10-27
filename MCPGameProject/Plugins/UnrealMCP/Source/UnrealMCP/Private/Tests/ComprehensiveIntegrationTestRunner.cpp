#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Misc/DateTime.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Commands/UnrealMCPMainDispatcher.h"
#include "Services/ComponentService.h"
#include "Factories/ComponentFactory.h"
#include "Factories/WidgetFactory.h"

// Forward declarations of test functions
void TestRefactoredSystemIntegration();
void TestComponentFactoryIntegration();
void RunMCPLoadTest(int32 NumCommands, int32 ConcurrentThreads);
void TestMCPErrorRecovery();

// Forward declarations of benchmark functions
void BenchmarkCommandRegistry();
void BenchmarkServiceLayer();
void BenchmarkFactories();

/**
 * Comprehensive test runner for all integration tests
 * This function runs all integration tests in sequence and reports results
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void RunComprehensiveIntegrationTests()
{
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== COMPREHENSIVE INTEGRATION TESTS ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    
    FDateTime StartTime = FDateTime::Now();
    int32 TestsPassed = 0;
    int32 TestsFailed = 0;
    
    // Test 1: Refactored System Integration
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("TEST 1: Refactored System Integration"));
    UE_LOG(LogTemp, Warning, TEXT("--------------------------------------"));
    try
    {
        TestRefactoredSystemIntegration();
        TestsPassed++;
        UE_LOG(LogTemp, Warning, TEXT("✓ PASSED: Refactored System Integration"));
    }
    catch (...)
    {
        TestsFailed++;
        UE_LOG(LogTemp, Error, TEXT("✗ FAILED: Refactored System Integration - Exception thrown"));
    }
    
    // Test 2: Component Factory Integration
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("TEST 2: Component Factory Integration"));
    UE_LOG(LogTemp, Warning, TEXT("-------------------------------------"));
    try
    {
        TestComponentFactoryIntegration();
        TestsPassed++;
        UE_LOG(LogTemp, Warning, TEXT("✓ PASSED: Component Factory Integration"));
    }
    catch (...)
    {
        TestsFailed++;
        UE_LOG(LogTemp, Error, TEXT("✗ FAILED: Component Factory Integration - Exception thrown"));
    }
    
    // Test 3: Error Recovery
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("TEST 3: Error Recovery"));
    UE_LOG(LogTemp, Warning, TEXT("----------------------"));
    try
    {
        TestMCPErrorRecovery();
        TestsPassed++;
        UE_LOG(LogTemp, Warning, TEXT("✓ PASSED: Error Recovery"));
    }
    catch (...)
    {
        TestsFailed++;
        UE_LOG(LogTemp, Error, TEXT("✗ FAILED: Error Recovery - Exception thrown"));
    }
    
    // Test 4: Load Testing (Light version for integration)
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("TEST 4: Load Testing (Light)"));
    UE_LOG(LogTemp, Warning, TEXT("----------------------------"));
    try
    {
        // Run a lighter load test for integration testing
        RunMCPLoadTest(20, 2); // 20 commands, 2 threads
        TestsPassed++;
        UE_LOG(LogTemp, Warning, TEXT("✓ PASSED: Load Testing"));
    }
    catch (...)
    {
        TestsFailed++;
        UE_LOG(LogTemp, Error, TEXT("✗ FAILED: Load Testing - Exception thrown"));
    }
    
    // Calculate results
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    int32 TotalTests = TestsPassed + TestsFailed;
    float SuccessRate = TotalTests > 0 ? (TestsPassed * 100.0f / TotalTests) : 0.0f;
    
    // Report final results
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("===        FINAL RESULTS             ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("Total Tests Run: %d"), TotalTests);
    UE_LOG(LogTemp, Warning, TEXT("Tests Passed: %d"), TestsPassed);
    UE_LOG(LogTemp, Warning, TEXT("Tests Failed: %d"), TestsFailed);
    UE_LOG(LogTemp, Warning, TEXT("Success Rate: %.1f%%"), SuccessRate);
    UE_LOG(LogTemp, Warning, TEXT("Total Duration: %.2f seconds"), Duration.GetTotalSeconds());
    
    if (TestsFailed == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎉 ALL TESTS PASSED! 🎉"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("⚠️  SOME TESTS FAILED - CHECK LOGS ⚠️"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

/**
 * Run specific performance benchmarks
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void RunPerformanceBenchmarks()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Performance Benchmarks ==="));
    
    // Benchmark 1: Command Registry Performance
    BenchmarkCommandRegistry();
    
    // Benchmark 2: Service Layer Performance
    BenchmarkServiceLayer();
    
    // Benchmark 3: Factory Performance
    BenchmarkFactories();
    
    UE_LOG(LogTemp, Warning, TEXT("=== Performance Benchmarks Completed ==="));
}

/**
 * Benchmark command registry performance
 */
void BenchmarkCommandRegistry()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Command Registry Benchmark ---"));
    
    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    
    // Benchmark command lookup
    const int32 NumLookups = 1000;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumLookups; ++i)
    {
        Registry.IsCommandRegistered(TEXT("create_blueprint"));
        Registry.IsCommandRegistered(TEXT("add_component_to_blueprint"));
        Registry.IsCommandRegistered(TEXT("compile_blueprint"));
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double LookupsPerSecond = NumLookups * 3 / Duration.GetTotalSeconds(); // 3 lookups per iteration
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Command lookups: %.0f lookups/second"), LookupsPerSecond);
    
    // Benchmark getting all commands
    StartTime = FDateTime::Now();
    for (int32 i = 0; i < 100; ++i)
    {
        TArray<FString> Commands = Registry.GetRegisteredCommandNames();
    }
    EndTime = FDateTime::Now();
    Duration = EndTime - StartTime;
    double GetAllPerSecond = 100 / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Get all commands: %.0f calls/second"), GetAllPerSecond);
}

/**
 * Benchmark service layer performance
 */
void BenchmarkServiceLayer()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Service Layer Benchmark ---"));
    
    // Benchmark component service
    FComponentService& ComponentService = FComponentService::Get();
    
    const int32 NumCalls = 500;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumCalls; ++i)
    {
        // Test component class resolution instead
        ComponentService.GetComponentClass(TEXT("StaticMeshComponent"));
        ComponentService.IsValidComponentType(TEXT("PointLightComponent"));
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double CallsPerSecond = (NumCalls * 2) / Duration.GetTotalSeconds(); // 2 calls per iteration
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Component service calls: %.0f calls/second"), CallsPerSecond);
}

/**
 * Benchmark factory performance
 */
void BenchmarkFactories()
{
    UE_LOG(LogTemp, Warning, TEXT("--- Factory Benchmark ---"));
    
    FComponentFactory& ComponentFactory = FComponentFactory::Get();
    FWidgetFactory& WidgetFactory = FWidgetFactory::Get();
    
    // Benchmark component factory
    const int32 NumLookups = 1000;
    FDateTime StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumLookups; ++i)
    {
        ComponentFactory.GetComponentClass(TEXT("StaticMeshComponent"));
        ComponentFactory.GetComponentClass(TEXT("PointLightComponent"));
    }
    
    FDateTime EndTime = FDateTime::Now();
    FTimespan Duration = EndTime - StartTime;
    double ComponentLookupsPerSecond = NumLookups * 2 / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Component factory lookups: %.0f lookups/second"), ComponentLookupsPerSecond);
    
    // Benchmark widget factory
    StartTime = FDateTime::Now();
    
    for (int32 i = 0; i < NumLookups; ++i)
    {
        WidgetFactory.GetAvailableWidgetTypes();
    }
    
    EndTime = FDateTime::Now();
    Duration = EndTime - StartTime;
    double WidgetCallsPerSecond = NumLookups / Duration.GetTotalSeconds();
    
    UE_LOG(LogTemp, Warning, TEXT("✓ Widget factory calls: %.0f calls/second"), WidgetCallsPerSecond);
}

/**
 * Test memory usage and cleanup
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void TestMemoryUsageAndCleanup()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Memory Usage and Cleanup Test ==="));
    
    // Get initial memory stats
    FPlatformMemoryStats InitialStats = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Initial Memory Usage: %.2f MB"), 
           InitialStats.UsedPhysical / (1024.0f * 1024.0f));
    
    // Create many objects to test memory management
    TArray<TSharedPtr<FJsonObject>> TestObjects;
    TestObjects.Reserve(1000);
    
    for (int32 i = 0; i < 1000; ++i)
    {
        TSharedPtr<FJsonObject> TestObj = MakeShareable(new FJsonObject);
        TestObj->SetStringField(TEXT("test_field"), FString::Printf(TEXT("test_value_%d"), i));
        TestObj->SetNumberField(TEXT("test_number"), i);
        TestObjects.Add(TestObj);
    }
    
    FPlatformMemoryStats PeakStats = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Peak Memory Usage: %.2f MB"), 
           PeakStats.UsedPhysical / (1024.0f * 1024.0f));
    
    // Clear objects
    TestObjects.Empty();
    
    // Force garbage collection
    GEngine->ForceGarbageCollection(true);
    
    FPlatformMemoryStats FinalStats = FPlatformMemory::GetStats();
    UE_LOG(LogTemp, Warning, TEXT("Final Memory Usage: %.2f MB"), 
           FinalStats.UsedPhysical / (1024.0f * 1024.0f));
    
    float MemoryDelta = (FinalStats.UsedPhysical - InitialStats.UsedPhysical) / (1024.0f * 1024.0f);
    UE_LOG(LogTemp, Warning, TEXT("Memory Delta: %.2f MB"), MemoryDelta);
    
    if (MemoryDelta < 10.0f) // Allow some tolerance
    {
        UE_LOG(LogTemp, Warning, TEXT("✓ Memory cleanup: GOOD (delta < 10MB)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ Memory cleanup: POTENTIAL LEAK (delta > 10MB)"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== Memory Test Completed ==="));
}