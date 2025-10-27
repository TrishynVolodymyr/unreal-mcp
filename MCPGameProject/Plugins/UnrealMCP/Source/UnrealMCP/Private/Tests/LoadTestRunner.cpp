#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Commands/UnrealMCPMainDispatcher.h"
#include "HAL/RunnableThread.h"
#include "HAL/Runnable.h"
#include "Dom/JsonObject.h"
#include "Misc/DateTime.h"
#include "Async/Async.h"

/**
 * Load test runner for testing concurrent command execution
 */
class FMCPLoadTestRunner : public FRunnable
{
public:
    FMCPLoadTestRunner(int32 InNumCommands, int32 InConcurrentThreads)
        : NumCommands(InNumCommands)
        , ConcurrentThreads(InConcurrentThreads)
        , CompletedCommands(0)
        , FailedCommands(0)
        , bStopRequested(false)
    {
    }

    virtual bool Init() override
    {
        StartTime = FDateTime::Now();
        return true;
    }

    virtual uint32 Run() override
    {
        UE_LOG(LogTemp, Warning, TEXT("Starting load test: %d commands across %d threads"), NumCommands, ConcurrentThreads);
        
        // Create test commands
        TArray<TSharedPtr<FJsonObject>> TestCommands;
        CreateTestCommands(TestCommands);
        
        // Execute commands concurrently
        TArray<TFuture<void>> Futures;
        
        int32 CommandsPerThread = NumCommands / ConcurrentThreads;
        int32 RemainingCommands = NumCommands % ConcurrentThreads;
        
        int32 StartIndex = 0;
        for (int32 ThreadIndex = 0; ThreadIndex < ConcurrentThreads; ++ThreadIndex)
        {
            int32 ThreadCommands = CommandsPerThread + (ThreadIndex < RemainingCommands ? 1 : 0);
            int32 EndIndex = StartIndex + ThreadCommands;
            
            TFuture<void> Future = Async(EAsyncExecution::Thread, [this, TestCommands, StartIndex, EndIndex]()
            {
                ExecuteCommandBatch(TestCommands, StartIndex, EndIndex);
            });
            
            Futures.Add(MoveTemp(Future));
            StartIndex = EndIndex;
        }
        
        // Wait for all threads to complete
        for (auto& Future : Futures)
        {
            Future.Wait();
        }
        
        EndTime = FDateTime::Now();
        LogResults();
        
        return 0;
    }

    virtual void Stop() override
    {
        bStopRequested = true;
    }

    virtual void Exit() override
    {
        // Cleanup if needed
    }

private:
    int32 NumCommands;
    int32 ConcurrentThreads;
    FDateTime StartTime;
    FDateTime EndTime;
    
    TAtomic<int32> CompletedCommands;
    TAtomic<int32> FailedCommands;
    TAtomic<bool> bStopRequested;
    
    void CreateTestCommands(TArray<TSharedPtr<FJsonObject>>& OutCommands)
    {
        OutCommands.Reserve(NumCommands);
        
        for (int32 i = 0; i < NumCommands; ++i)
        {
            TSharedPtr<FJsonObject> Command = MakeShareable(new FJsonObject);
            
            // Vary command types for realistic testing
            int32 CommandType = i % 4;
            switch (CommandType)
            {
                case 0: // Create Blueprint
                    Command->SetStringField(TEXT("command"), TEXT("create_blueprint"));
                    Command->SetStringField(TEXT("name"), FString::Printf(TEXT("LoadTestBlueprint_%d"), i));
                    Command->SetStringField(TEXT("parent_class"), TEXT("Actor"));
                    break;
                    
                case 1: // Get available commands
                    Command->SetStringField(TEXT("command"), TEXT("get_available_commands"));
                    break;
                    
                case 2: // Component factory query
                    Command->SetStringField(TEXT("command"), TEXT("get_component_types"));
                    break;
                    
                case 3: // Invalid command (for error handling testing)
                    Command->SetStringField(TEXT("command"), TEXT("invalid_test_command"));
                    break;
            }
            
            OutCommands.Add(Command);
        }
    }
    
    void ExecuteCommandBatch(const TArray<TSharedPtr<FJsonObject>>& Commands, int32 StartIndex, int32 EndIndex)
    {
        FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
        
        for (int32 i = StartIndex; i < EndIndex && !bStopRequested; ++i)
        {
            if (i >= Commands.Num()) break;
            
            const TSharedPtr<FJsonObject>& Command = Commands[i];
            FString CommandName = Command->GetStringField(TEXT("command"));
            
            try
            {
                TSharedPtr<FJsonObject> Response = Dispatcher.HandleCommand(CommandName, Command);
                
                if (Response.IsValid())
                {
                    CompletedCommands++;
                }
                else
                {
                    FailedCommands++;
                    UE_LOG(LogTemp, Warning, TEXT("Load test command %d failed: No response"), i);
                }
            }
            catch (...)
            {
                FailedCommands++;
                UE_LOG(LogTemp, Error, TEXT("Load test command %d threw exception"), i);
            }
        }
    }
    
    void LogResults()
    {
        FTimespan Duration = EndTime - StartTime;
        double TotalSeconds = Duration.GetTotalSeconds();
        double CommandsPerSecond = TotalSeconds > 0 ? NumCommands / TotalSeconds : 0;
        
        UE_LOG(LogTemp, Warning, TEXT("=== Load Test Results ==="));
        UE_LOG(LogTemp, Warning, TEXT("Total Commands: %d"), NumCommands);
        UE_LOG(LogTemp, Warning, TEXT("Concurrent Threads: %d"), ConcurrentThreads);
        UE_LOG(LogTemp, Warning, TEXT("Completed Commands: %d"), CompletedCommands.Load());
        UE_LOG(LogTemp, Warning, TEXT("Failed Commands: %d"), FailedCommands.Load());
        UE_LOG(LogTemp, Warning, TEXT("Total Duration: %.2f seconds"), TotalSeconds);
        UE_LOG(LogTemp, Warning, TEXT("Commands per Second: %.2f"), CommandsPerSecond);
        UE_LOG(LogTemp, Warning, TEXT("Success Rate: %.2f%%"), 
               NumCommands > 0 ? (CompletedCommands.Load() * 100.0f / NumCommands) : 0.0f);
    }
};

/**
 * Run load test with specified parameters
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void RunMCPLoadTest(int32 NumCommands = 100, int32 ConcurrentThreads = 4)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Starting MCP Load Test ==="));
    
    if (NumCommands <= 0 || ConcurrentThreads <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid parameters: NumCommands=%d, ConcurrentThreads=%d"), 
               NumCommands, ConcurrentThreads);
        return;
    }
    
    // Create and run load test
    TUniquePtr<FMCPLoadTestRunner> LoadTestRunner = MakeUnique<FMCPLoadTestRunner>(NumCommands, ConcurrentThreads);
    
    FRunnableThread* LoadTestThread = FRunnableThread::Create(
        LoadTestRunner.Get(), 
        TEXT("MCPLoadTest"), 
        0, 
        TPri_Normal
    );
    
    if (LoadTestThread)
    {
        // Wait for completion
        LoadTestThread->WaitForCompletion();
        delete LoadTestThread;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create load test thread"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== MCP Load Test Completed ==="));
}

/**
 * Test error recovery scenarios
 */
UFUNCTION(BlueprintCallable, Category = "MCP Testing")
void TestMCPErrorRecovery()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Testing MCP Error Recovery ==="));
    
    FUnrealMCPMainDispatcher& Dispatcher = FUnrealMCPMainDispatcher::Get();
    
    // Test 1: Invalid command
    TSharedPtr<FJsonObject> InvalidCommand = MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> Response1 = Dispatcher.HandleCommand(TEXT("completely_invalid_command"), InvalidCommand);
    
    bool bHandledInvalidCommand = Response1.IsValid() && Response1->HasField(TEXT("success")) && 
                                 !Response1->GetBoolField(TEXT("success"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Invalid command handling: %s"), bHandledInvalidCommand ? TEXT("OK") : TEXT("FAILED"));
    
    // Test 2: Malformed parameters
    TSharedPtr<FJsonObject> MalformedParams = MakeShareable(new FJsonObject);
    MalformedParams->SetStringField(TEXT("invalid_field"), TEXT("invalid_value"));
    
    TSharedPtr<FJsonObject> Response2 = Dispatcher.HandleCommand(TEXT("create_blueprint"), MalformedParams);
    bool bHandledMalformed = Response2.IsValid() && Response2->HasField(TEXT("success")) && 
                            !Response2->GetBoolField(TEXT("success"));
    UE_LOG(LogTemp, Warning, TEXT("✓ Malformed parameters handling: %s"), bHandledMalformed ? TEXT("OK") : TEXT("FAILED"));
    
    // Test 3: System should still work after errors
    TSharedPtr<FJsonObject> ValidCommand = MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> Response3 = Dispatcher.GetAvailableCommands();
    bool bSystemRecovered = Response3.IsValid() && Response3->HasField(TEXT("commands"));
    UE_LOG(LogTemp, Warning, TEXT("✓ System recovery after errors: %s"), bSystemRecovered ? TEXT("OK") : TEXT("FAILED"));
    
    UE_LOG(LogTemp, Warning, TEXT("=== MCP Error Recovery Test Completed ==="));
}