#pragma once

#include "CoreMinimal.h"
#include "MCPOperationContext.h"
#include "MCPBatchOperationHandler.generated.h"

/**
 * Structure representing a single operation in a batch
 */
USTRUCT(BlueprintType)
struct UNREALMCP_API FMCPBatchOperation
{
    GENERATED_BODY()

    /** Unique identifier for this operation within the batch */
    UPROPERTY(BlueprintReadOnly)
    FString OperationId;

    /** Name/type of the operation */
    UPROPERTY(BlueprintReadOnly)
    FString OperationType;

    /** Parameters for the operation as JSON string */
    UPROPERTY(BlueprintReadOnly)
    FString Parameters;

    /** Whether this operation depends on previous operations */
    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Dependencies;

    /** Priority of the operation (higher numbers execute first) */
    UPROPERTY(BlueprintReadOnly)
    int32 Priority;

    /** Whether to continue batch execution if this operation fails */
    UPROPERTY(BlueprintReadOnly)
    bool bContinueOnFailure;

    FMCPBatchOperation()
        : OperationId(TEXT(""))
        , OperationType(TEXT(""))
        , Parameters(TEXT(""))
        , Dependencies()
        , Priority(0)
        , bContinueOnFailure(true)
    {
    }

    FMCPBatchOperation(const FString& InOperationId, const FString& InOperationType, const FString& InParameters)
        : OperationId(InOperationId)
        , OperationType(InOperationType)
        , Parameters(InParameters)
        , Dependencies()
        , Priority(0)
        , bContinueOnFailure(true)
    {
    }
};

/**
 * Result of a batch operation execution
 */
USTRUCT(BlueprintType)
struct UNREALMCP_API FMCPBatchOperationResult
{
    GENERATED_BODY()

    /** The operation that was executed */
    UPROPERTY(BlueprintReadOnly)
    FMCPBatchOperation Operation;

    /** Whether the operation succeeded */
    UPROPERTY(BlueprintReadOnly)
    bool bSuccess;

    /** Result data from the operation */
    UPROPERTY(BlueprintReadOnly)
    FString ResultData;

    /** Operation context with errors and warnings */
    UPROPERTY(BlueprintReadOnly)
    UMCPOperationContext* OperationContext;

    /** Execution time for this operation */
    UPROPERTY(BlueprintReadOnly)
    float ExecutionTime;

    FMCPBatchOperationResult()
        : Operation()
        , bSuccess(false)
        , ResultData(TEXT(""))
        , OperationContext(nullptr)
        , ExecutionTime(0.0f)
    {
    }
};

/**
 * Handler for executing multiple MCP operations in batch with error aggregation
 * Provides dependency management, error handling, and rollback capabilities
 */
UCLASS(BlueprintType)
class UNREALMCP_API UMCPBatchOperationHandler : public UObject
{
    GENERATED_BODY()

public:
    UMCPBatchOperationHandler();

    /** Initialize the batch handler with overall batch context */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    void Initialize(const FString& BatchName, const FString& BatchId = TEXT(""));

    /** Add an operation to the batch */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    void AddOperation(const FMCPBatchOperation& Operation);

    /** Add multiple operations to the batch */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    void AddOperations(const TArray<FMCPBatchOperation>& Operations);

    /** Execute all operations in the batch */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    bool ExecuteBatch();

    /** Execute operations with rollback capability */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    bool ExecuteBatchWithRollback();

    /** Get the results of all executed operations */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    TArray<FMCPBatchOperationResult> GetResults() const;

    /** Get the overall batch operation context */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    UMCPOperationContext* GetBatchContext() const;

    /** Get aggregated error summary for the entire batch */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    FString GetBatchErrorSummary() const;

    /** Create comprehensive batch response */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    FString CreateBatchResponse() const;

    /** Check if the batch execution was successful */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    bool IsBatchSuccessful() const;

    /** Get statistics for the batch execution */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    FString GetBatchStatistics() const;

    /** Clear all operations and results */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    void Clear();

    /** Set whether to stop execution on first failure */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    void SetStopOnFirstFailure(bool bStopOnFailure);

    /** Set maximum number of parallel operations */
    UFUNCTION(BlueprintCallable, Category = "MCP Batch Operations")
    void SetMaxParallelOperations(int32 MaxParallel);

protected:
    /** Overall batch operation context */
    UPROPERTY(BlueprintReadOnly, Category = "Batch Context")
    UMCPOperationContext* BatchContext;

    /** List of operations to execute */
    UPROPERTY(BlueprintReadOnly, Category = "Operations")
    TArray<FMCPBatchOperation> Operations;

    /** Results of executed operations */
    UPROPERTY(BlueprintReadOnly, Category = "Results")
    TArray<FMCPBatchOperationResult> Results;

    /** Whether to stop execution on first failure */
    UPROPERTY(BlueprintReadOnly, Category = "Configuration")
    bool bStopOnFirstFailure;

    /** Maximum number of operations to execute in parallel */
    UPROPERTY(BlueprintReadOnly, Category = "Configuration")
    int32 MaxParallelOperations;

    /** Whether the batch has been executed */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bBatchExecuted;

private:
    /** Sort operations by priority and dependencies */
    void SortOperationsByPriorityAndDependencies();

    /** Check if an operation's dependencies are satisfied */
    bool AreDependenciesSatisfied(const FMCPBatchOperation& Operation) const;

    /** Execute a single operation */
    FMCPBatchOperationResult ExecuteSingleOperation(const FMCPBatchOperation& Operation);

    /** Rollback executed operations in reverse order */
    void RollbackOperations();

    /** Validate operation dependencies */
    bool ValidateDependencies() const;

    /** Get operations that can be executed in parallel */
    TArray<FMCPBatchOperation> GetExecutableOperations() const;
};

/**
 * Utility functions for batch operation management
 */
class UNREALMCP_API FMCPBatchOperationUtils
{
public:
    /** Create a batch operation from JSON parameters */
    static FMCPBatchOperation CreateOperation(const FString& OperationType, const TSharedPtr<FJsonObject>& Parameters, const FString& OperationId = TEXT(""));

    /** Create multiple operations from JSON array */
    static TArray<FMCPBatchOperation> CreateOperationsFromJson(const FString& JsonString);

    /** Validate batch operation parameters */
    static bool ValidateBatchOperation(const FMCPBatchOperation& Operation, FString& OutError);

    /** Create dependency graph for operations */
    static TMap<FString, TArray<FString>> CreateDependencyGraph(const TArray<FMCPBatchOperation>& Operations);

    /** Check for circular dependencies */
    static bool HasCircularDependencies(const TArray<FMCPBatchOperation>& Operations);

    /** Generate execution order based on dependencies */
    static TArray<FString> GenerateExecutionOrder(const TArray<FMCPBatchOperation>& Operations);
};
