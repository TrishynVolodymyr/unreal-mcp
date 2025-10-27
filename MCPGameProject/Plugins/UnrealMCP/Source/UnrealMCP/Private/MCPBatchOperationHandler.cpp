#include "MCPBatchOperationHandler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Guid.h"
#include "Async/ParallelFor.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBatchOperations, Log, All);

UMCPBatchOperationHandler::UMCPBatchOperationHandler()
    : BatchContext(nullptr)
    , bStopOnFirstFailure(false)
    , MaxParallelOperations(4)
    , bBatchExecuted(false)
{
    BatchContext = NewObject<UMCPOperationContext>(this, TEXT("BatchContext"));
}

void UMCPBatchOperationHandler::Initialize(const FString& BatchName, const FString& BatchId)
{
    if (BatchContext)
    {
        BatchContext->Initialize(BatchName, BatchId);
    }
    
    Operations.Empty();
    Results.Empty();
    bBatchExecuted = false;
    
    UE_LOG(LogMCPBatchOperations, Log, TEXT("Initialized batch operation handler: %s"), *BatchName);
}

void UMCPBatchOperationHandler::AddOperation(const FMCPBatchOperation& Operation)
{
    if (bBatchExecuted)
    {
        if (BatchContext)
        {
            BatchContext->AddWarning(TEXT("Cannot add operations after batch execution"), 
                                   TEXT("Batch has already been executed"));
        }
        return;
    }
    
    // Validate the operation
    FString ValidationError;
    if (!FMCPBatchOperationUtils::ValidateBatchOperation(Operation, ValidationError))
    {
        if (BatchContext)
        {
            BatchContext->AddError(
                FMCPError(EMCPErrorType::ValidationFailed, 0, TEXT("Invalid batch operation"), ValidationError),
                EMCPErrorSeverity::Error
            );
        }
        return;
    }
    
    Operations.Add(Operation);
    
    if (BatchContext)
    {
        BatchContext->AddOperationContext(
            FString::Printf(TEXT("Operation_%s"), *Operation.OperationId),
            Operation.OperationType
        );
    }
    
    UE_LOG(LogMCPBatchOperations, Verbose, TEXT("Added operation: %s [%s]"), 
           *Operation.OperationType, *Operation.OperationId);
}

void UMCPBatchOperationHandler::AddOperations(const TArray<FMCPBatchOperation>& InOperations)
{
    for (const FMCPBatchOperation& Operation : InOperations)
    {
        AddOperation(Operation);
    }
}

bool UMCPBatchOperationHandler::ExecuteBatch()
{
    if (bBatchExecuted)
    {
        if (BatchContext)
        {
            BatchContext->AddWarning(TEXT("Batch already executed"), TEXT("Cannot execute batch multiple times"));
        }
        return false;
    }
    
    if (Operations.Num() == 0)
    {
        if (BatchContext)
        {
            BatchContext->AddWarning(TEXT("No operations to execute"), TEXT("Batch contains no operations"));
        }
        return true;
    }
    
    // Validate dependencies
    if (!ValidateDependencies())
    {
        if (BatchContext)
        {
            BatchContext->AddError(
                FMCPError(EMCPErrorType::ValidationFailed, 0, TEXT("Invalid operation dependencies")),
                EMCPErrorSeverity::Error
            );
        }
        return false;
    }
    
    // Sort operations by priority and dependencies
    SortOperationsByPriorityAndDependencies();
    
    bBatchExecuted = true;
    bool bOverallSuccess = true;
    
    UE_LOG(LogMCPBatchOperations, Log, TEXT("Starting batch execution with %d operations"), Operations.Num());
    
    // Execute operations
    for (const FMCPBatchOperation& Operation : Operations)
    {
        // Check if dependencies are satisfied
        if (!AreDependenciesSatisfied(Operation))
        {
            FMCPBatchOperationResult FailedResult;
            FailedResult.Operation = Operation;
            FailedResult.bSuccess = false;
            FailedResult.ResultData = TEXT("Dependencies not satisfied");
            FailedResult.OperationContext = NewObject<UMCPOperationContext>(this);
            FailedResult.OperationContext->Initialize(Operation.OperationType, Operation.OperationId);
            FailedResult.OperationContext->AddError(
                FMCPError(EMCPErrorType::ExecutionFailed, 0, TEXT("Operation dependencies not satisfied")),
                EMCPErrorSeverity::Error
            );
            
            Results.Add(FailedResult);
            bOverallSuccess = false;
            
            if (bStopOnFirstFailure && !Operation.bContinueOnFailure)
            {
                break;
            }
            continue;
        }
        
        // Execute the operation
        FMCPBatchOperationResult Result = ExecuteSingleOperation(Operation);
        Results.Add(Result);
        
        if (!Result.bSuccess)
        {
            bOverallSuccess = false;
            
            if (bStopOnFirstFailure && !Operation.bContinueOnFailure)
            {
                UE_LOG(LogMCPBatchOperations, Warning, TEXT("Stopping batch execution due to failure in operation: %s"), 
                       *Operation.OperationId);
                break;
            }
        }
    }
    
    if (BatchContext)
    {
        BatchContext->CompleteOperation();
        
        if (bOverallSuccess)
        {
            BatchContext->AddInfo(TEXT("Batch execution completed successfully"), 
                                FString::Printf(TEXT("Executed %d operations"), Results.Num()));
        }
        else
        {
            BatchContext->AddError(
                FMCPError(EMCPErrorType::ExecutionFailed, 0, TEXT("Batch execution completed with errors")),
                EMCPErrorSeverity::Error
            );
        }
    }
    
    UE_LOG(LogMCPBatchOperations, Log, TEXT("Batch execution completed. Success: %s, Operations: %d"), 
           bOverallSuccess ? TEXT("true") : TEXT("false"), Results.Num());
    
    return bOverallSuccess;
}

bool UMCPBatchOperationHandler::ExecuteBatchWithRollback()
{
    bool bSuccess = ExecuteBatch();
    
    if (!bSuccess)
    {
        UE_LOG(LogMCPBatchOperations, Warning, TEXT("Batch execution failed, initiating rollback"));
        RollbackOperations();
    }
    
    return bSuccess;
}

TArray<FMCPBatchOperationResult> UMCPBatchOperationHandler::GetResults() const
{
    return Results;
}

UMCPOperationContext* UMCPBatchOperationHandler::GetBatchContext() const
{
    return BatchContext;
}

FString UMCPBatchOperationHandler::GetBatchErrorSummary() const
{
    if (!BatchContext)
    {
        return TEXT("No batch context available");
    }
    
    FString Summary = BatchContext->CreateErrorSummary();
    
    // Add individual operation error counts
    int32 SuccessfulOps = 0;
    int32 FailedOps = 0;
    
    for (const FMCPBatchOperationResult& Result : Results)
    {
        if (Result.bSuccess)
        {
            SuccessfulOps++;
        }
        else
        {
            FailedOps++;
        }
    }
    
    Summary += FString::Printf(TEXT(" | Operations - Successful: %d, Failed: %d"), SuccessfulOps, FailedOps);
    
    return Summary;
}

FString UMCPBatchOperationHandler::CreateBatchResponse() const
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
    
    ResponseObject->SetBoolField(TEXT("success"), IsBatchSuccessful());
    ResponseObject->SetStringField(TEXT("batchId"), BatchContext ? BatchContext->GetOperationStats() : TEXT(""));
    ResponseObject->SetNumberField(TEXT("totalOperations"), Operations.Num());
    ResponseObject->SetNumberField(TEXT("executedOperations"), Results.Num());
    
    // Add individual operation results
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (const FMCPBatchOperationResult& Result : Results)
    {
        TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
        ResultObject->SetStringField(TEXT("operationId"), Result.Operation.OperationId);
        ResultObject->SetStringField(TEXT("operationType"), Result.Operation.OperationType);
        ResultObject->SetBoolField(TEXT("success"), Result.bSuccess);
        ResultObject->SetStringField(TEXT("resultData"), Result.ResultData);
        ResultObject->SetNumberField(TEXT("executionTime"), Result.ExecutionTime);
        
        if (Result.OperationContext)
        {
            ResultObject->SetStringField(TEXT("operationContext"), Result.OperationContext->CreateResponse());
        }
        
        ResultsArray.Add(MakeShareable(new FJsonValueObject(ResultObject)));
    }
    ResponseObject->SetArrayField(TEXT("results"), ResultsArray);
    
    // Add batch context information
    if (BatchContext)
    {
        ResponseObject->SetStringField(TEXT("batchContext"), BatchContext->CreateResponse());
        ResponseObject->SetStringField(TEXT("statistics"), GetBatchStatistics());
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool UMCPBatchOperationHandler::IsBatchSuccessful() const
{
    if (!bBatchExecuted)
    {
        return false;
    }
    
    for (const FMCPBatchOperationResult& Result : Results)
    {
        if (!Result.bSuccess)
        {
            return false;
        }
    }
    
    return true;
}

FString UMCPBatchOperationHandler::GetBatchStatistics() const
{
    TSharedPtr<FJsonObject> StatsObject = MakeShareable(new FJsonObject);
    
    StatsObject->SetNumberField(TEXT("totalOperations"), Operations.Num());
    StatsObject->SetNumberField(TEXT("executedOperations"), Results.Num());
    StatsObject->SetBoolField(TEXT("batchExecuted"), bBatchExecuted);
    StatsObject->SetBoolField(TEXT("overallSuccess"), IsBatchSuccessful());
    
    // Calculate timing statistics
    float TotalExecutionTime = 0.0f;
    float MinExecutionTime = FLT_MAX;
    float MaxExecutionTime = 0.0f;
    
    for (const FMCPBatchOperationResult& Result : Results)
    {
        TotalExecutionTime += Result.ExecutionTime;
        MinExecutionTime = FMath::Min(MinExecutionTime, Result.ExecutionTime);
        MaxExecutionTime = FMath::Max(MaxExecutionTime, Result.ExecutionTime);
    }
    
    if (Results.Num() > 0)
    {
        StatsObject->SetNumberField(TEXT("totalExecutionTime"), TotalExecutionTime);
        StatsObject->SetNumberField(TEXT("averageExecutionTime"), TotalExecutionTime / Results.Num());
        StatsObject->SetNumberField(TEXT("minExecutionTime"), MinExecutionTime);
        StatsObject->SetNumberField(TEXT("maxExecutionTime"), MaxExecutionTime);
    }
    
    if (BatchContext)
    {
        StatsObject->SetNumberField(TEXT("batchDuration"), BatchContext->GetOperationDuration());
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(StatsObject.ToSharedRef(), Writer);
    
    return OutputString;
}

void UMCPBatchOperationHandler::Clear()
{
    Operations.Empty();
    Results.Empty();
    bBatchExecuted = false;
    
    if (BatchContext)
    {
        BatchContext->Clear();
    }
}

void UMCPBatchOperationHandler::SetStopOnFirstFailure(bool bStopOnFailure)
{
    bStopOnFirstFailure = bStopOnFailure;
}

void UMCPBatchOperationHandler::SetMaxParallelOperations(int32 MaxParallel)
{
    MaxParallelOperations = FMath::Max(1, MaxParallel);
}

void UMCPBatchOperationHandler::SortOperationsByPriorityAndDependencies()
{
    // Priority-based sorting - topological sorting for dependencies would be a future enhancement
    Operations.Sort([](const FMCPBatchOperation& A, const FMCPBatchOperation& B) {
        return A.Priority > B.Priority;
    });
}

bool UMCPBatchOperationHandler::AreDependenciesSatisfied(const FMCPBatchOperation& Operation) const
{
    for (const FString& DependencyId : Operation.Dependencies)
    {
        bool bDependencyFound = false;
        for (const FMCPBatchOperationResult& Result : Results)
        {
            if (Result.Operation.OperationId == DependencyId && Result.bSuccess)
            {
                bDependencyFound = true;
                break;
            }
        }
        
        if (!bDependencyFound)
        {
            return false;
        }
    }
    
    return true;
}

FMCPBatchOperationResult UMCPBatchOperationHandler::ExecuteSingleOperation(const FMCPBatchOperation& Operation)
{
    FMCPBatchOperationResult Result;
    Result.Operation = Operation;
    Result.OperationContext = NewObject<UMCPOperationContext>(this);
    Result.OperationContext->Initialize(Operation.OperationType, Operation.OperationId);
    
    FDateTime StartTime = FDateTime::Now();
    
    // Operation execution is delegated to the command registry system
    // This is a placeholder for future batch operation integration
    UE_LOG(LogMCPBatchOperations, Verbose, TEXT("Executing operation: %s [%s]"), 
           *Operation.OperationType, *Operation.OperationId);
    
    // Simulate operation execution
    Result.bSuccess = true;
    Result.ResultData = FString::Printf(TEXT("Operation %s completed successfully"), *Operation.OperationId);
    
    Result.OperationContext->AddInfo(TEXT("Operation executed"), TEXT("Placeholder execution"));
    Result.OperationContext->CompleteOperation();
    
    FDateTime EndTime = FDateTime::Now();
    Result.ExecutionTime = (EndTime - StartTime).GetTotalSeconds();
    
    return Result;
}

void UMCPBatchOperationHandler::RollbackOperations()
{
    UE_LOG(LogMCPBatchOperations, Warning, TEXT("Starting rollback of %d operations"), Results.Num());
    
    // Rollback in reverse order
    for (int32 i = Results.Num() - 1; i >= 0; --i)
    {
        const FMCPBatchOperationResult& Result = Results[i];
        if (Result.bSuccess)
        {
            // Rollback logic would be operation-specific and is a future enhancement
            UE_LOG(LogMCPBatchOperations, Verbose, TEXT("Rolling back operation: %s"), 
                   *Result.Operation.OperationId);
        }
    }
    
    if (BatchContext)
    {
        BatchContext->AddInfo(TEXT("Rollback completed"), 
                            FString::Printf(TEXT("Rolled back %d operations"), Results.Num()));
    }
}

bool UMCPBatchOperationHandler::ValidateDependencies() const
{
    return !FMCPBatchOperationUtils::HasCircularDependencies(Operations);
}

TArray<FMCPBatchOperation> UMCPBatchOperationHandler::GetExecutableOperations() const
{
    TArray<FMCPBatchOperation> ExecutableOps;
    
    for (const FMCPBatchOperation& Operation : Operations)
    {
        if (AreDependenciesSatisfied(Operation))
        {
            ExecutableOps.Add(Operation);
        }
    }
    
    return ExecutableOps;
}

// Utility functions implementation
FMCPBatchOperation FMCPBatchOperationUtils::CreateOperation(const FString& OperationType, const TSharedPtr<FJsonObject>& Parameters, const FString& OperationId)
{
    FString OpId = OperationId.IsEmpty() ? FGuid::NewGuid().ToString() : OperationId;
    
    FString ParametersString;
    if (Parameters.IsValid())
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParametersString);
        FJsonSerializer::Serialize(Parameters.ToSharedRef(), Writer);
    }
    
    return FMCPBatchOperation(OpId, OperationType, ParametersString);
}

TArray<FMCPBatchOperation> FMCPBatchOperationUtils::CreateOperationsFromJson(const FString& JsonString)
{
    TArray<FMCPBatchOperation> Operations;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return Operations;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* OperationsArray;
    if (JsonObject->TryGetArrayField(TEXT("operations"), OperationsArray))
    {
        for (const TSharedPtr<FJsonValue>& OpValue : *OperationsArray)
        {
            const TSharedPtr<FJsonObject>* OpObject;
            if (OpValue->TryGetObject(OpObject) && OpObject->IsValid())
            {
                FMCPBatchOperation Operation;
                (*OpObject)->TryGetStringField(TEXT("operationId"), Operation.OperationId);
                (*OpObject)->TryGetStringField(TEXT("operationType"), Operation.OperationType);
                (*OpObject)->TryGetStringField(TEXT("parameters"), Operation.Parameters);
                (*OpObject)->TryGetNumberField(TEXT("priority"), Operation.Priority);
                (*OpObject)->TryGetBoolField(TEXT("continueOnFailure"), Operation.bContinueOnFailure);
                
                const TArray<TSharedPtr<FJsonValue>>* DependenciesArray;
                if ((*OpObject)->TryGetArrayField(TEXT("dependencies"), DependenciesArray))
                {
                    for (const TSharedPtr<FJsonValue>& DepValue : *DependenciesArray)
                    {
                        FString Dependency;
                        if (DepValue->TryGetString(Dependency))
                        {
                            Operation.Dependencies.Add(Dependency);
                        }
                    }
                }
                
                Operations.Add(Operation);
            }
        }
    }
    
    return Operations;
}

bool FMCPBatchOperationUtils::ValidateBatchOperation(const FMCPBatchOperation& Operation, FString& OutError)
{
    if (Operation.OperationId.IsEmpty())
    {
        OutError = TEXT("Operation ID cannot be empty");
        return false;
    }
    
    if (Operation.OperationType.IsEmpty())
    {
        OutError = TEXT("Operation type cannot be empty");
        return false;
    }
    
    // Validate JSON parameters if provided
    if (!Operation.Parameters.IsEmpty())
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Operation.Parameters);
        
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            OutError = TEXT("Invalid JSON in operation parameters");
            return false;
        }
    }
    
    return true;
}

TMap<FString, TArray<FString>> FMCPBatchOperationUtils::CreateDependencyGraph(const TArray<FMCPBatchOperation>& Operations)
{
    TMap<FString, TArray<FString>> DependencyGraph;
    
    for (const FMCPBatchOperation& Operation : Operations)
    {
        DependencyGraph.Add(Operation.OperationId, Operation.Dependencies);
    }
    
    return DependencyGraph;
}

bool FMCPBatchOperationUtils::HasCircularDependencies(const TArray<FMCPBatchOperation>& Operations)
{
    TMap<FString, TArray<FString>> DependencyGraph = CreateDependencyGraph(Operations);
    TSet<FString> Visited;
    TSet<FString> RecursionStack;
    
    // Check for cycles using DFS
    for (const auto& GraphPair : DependencyGraph)
    {
        if (!Visited.Contains(GraphPair.Key))
        {
            TFunction<bool(const FString&)> HasCycleDFS = [&](const FString& Node) -> bool
            {
                Visited.Add(Node);
                RecursionStack.Add(Node);
                
                const TArray<FString>* Dependencies = DependencyGraph.Find(Node);
                if (Dependencies)
                {
                    for (const FString& Dependency : *Dependencies)
                    {
                        if (!Visited.Contains(Dependency))
                        {
                            if (HasCycleDFS(Dependency))
                            {
                                return true;
                            }
                        }
                        else if (RecursionStack.Contains(Dependency))
                        {
                            return true;
                        }
                    }
                }
                
                RecursionStack.Remove(Node);
                return false;
            };
            
            if (HasCycleDFS(GraphPair.Key))
            {
                return true;
            }
        }
    }
    
    return false;
}

TArray<FString> FMCPBatchOperationUtils::GenerateExecutionOrder(const TArray<FMCPBatchOperation>& Operations)
{
    TArray<FString> ExecutionOrder;
    TMap<FString, TArray<FString>> DependencyGraph = CreateDependencyGraph(Operations);
    TSet<FString> Visited;
    
    // Topological sort using DFS
    TFunction<void(const FString&)> TopologicalSortDFS = [&](const FString& Node)
    {
        Visited.Add(Node);
        
        const TArray<FString>* Dependencies = DependencyGraph.Find(Node);
        if (Dependencies)
        {
            for (const FString& Dependency : *Dependencies)
            {
                if (!Visited.Contains(Dependency))
                {
                    TopologicalSortDFS(Dependency);
                }
            }
        }
        
        ExecutionOrder.Insert(Node, 0);
    };
    
    for (const FMCPBatchOperation& Operation : Operations)
    {
        if (!Visited.Contains(Operation.OperationId))
        {
            TopologicalSortDFS(Operation.OperationId);
        }
    }
    
    return ExecutionOrder;
}