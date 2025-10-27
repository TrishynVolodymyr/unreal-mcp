#pragma once

#include "CoreMinimal.h"
#include "MCPError.h"
#include "MCPOperationContext.generated.h"

/**
 * Enumeration for error severity levels
 */
UENUM(BlueprintType)
enum class EMCPErrorSeverity : uint8
{
    Info = 0,
    Warning,
    Error,
    Critical,
    Fatal
};

/**
 * Enhanced error structure with severity and context information
 */
USTRUCT(BlueprintType)
struct UNREALMCP_API FMCPEnhancedError
{
    GENERATED_BODY()

    /** Base error information */
    UPROPERTY(BlueprintReadOnly)
    FMCPError BaseError;

    /** Severity level of the error */
    UPROPERTY(BlueprintReadOnly)
    EMCPErrorSeverity Severity;

    /** Timestamp when the error occurred */
    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;

    /** Source location where the error occurred (file:line) */
    UPROPERTY(BlueprintReadOnly)
    FString SourceLocation;

    /** Additional context information */
    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Context;

    /** Stack trace information (if available) */
    UPROPERTY(BlueprintReadOnly)
    FString StackTrace;

    FMCPEnhancedError()
        : BaseError()
        , Severity(EMCPErrorSeverity::Error)
        , Timestamp(FDateTime::Now())
        , SourceLocation(TEXT(""))
        , Context()
        , StackTrace(TEXT(""))
    {
    }

    FMCPEnhancedError(const FMCPError& InBaseError, EMCPErrorSeverity InSeverity = EMCPErrorSeverity::Error)
        : BaseError(InBaseError)
        , Severity(InSeverity)
        , Timestamp(FDateTime::Now())
        , SourceLocation(TEXT(""))
        , Context()
        , StackTrace(TEXT(""))
    {
    }

    /** Add context information to the error */
    void AddContext(const FString& Key, const FString& Value)
    {
        Context.Add(Key, Value);
    }

    /** Set source location information */
    void SetSourceLocation(const FString& InSourceLocation)
    {
        SourceLocation = InSourceLocation;
    }

    /** Convert to JSON for transmission */
    FString ToJsonString() const;

    /** Create from JSON string */
    static FMCPEnhancedError FromJsonString(const FString& JsonString);
};

/**
 * Operation context for tracking MCP operations and aggregating errors
 * Provides comprehensive error handling, logging, and debugging capabilities
 */
UCLASS(BlueprintType)
class UNREALMCP_API UMCPOperationContext : public UObject
{
    GENERATED_BODY()

public:
    UMCPOperationContext();

    /** Initialize the operation context with operation details */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void Initialize(const FString& InOperationName, const FString& InOperationId = TEXT(""));

    /** Add an error to the operation context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void AddError(const FMCPError& Error, EMCPErrorSeverity Severity = EMCPErrorSeverity::Error, const FString& SourceLocation = TEXT(""));

    /** Add an enhanced error to the operation context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void AddEnhancedError(const FMCPEnhancedError& Error);

    /** Add a warning to the operation context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void AddWarning(const FString& Warning, const FString& Details = TEXT(""), const FString& SourceLocation = TEXT(""));

    /** Add informational message to the operation context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void AddInfo(const FString& Info, const FString& Details = TEXT(""));

    /** Add context information that applies to the entire operation */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void AddOperationContext(const FString& Key, const FString& Value);

    /** Check if the operation has any errors */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    bool HasErrors() const;

    /** Check if the operation has any warnings */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    bool HasWarnings() const;

    /** Get the count of errors by severity */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    int32 GetErrorCount(EMCPErrorSeverity Severity = EMCPErrorSeverity::Error) const;

    /** Get all errors of a specific severity */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    TArray<FMCPEnhancedError> GetErrorsBySeverity(EMCPErrorSeverity Severity) const;

    /** Get the most severe error in the context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    FMCPEnhancedError GetMostSevereError() const;

    /** Create a comprehensive response object from the operation context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    FString CreateResponse(const FString& SuccessData = TEXT(""), const FString& Metadata = TEXT("")) const;

    /** Create a summary of all errors and warnings */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    FString CreateErrorSummary() const;

    /** Clear all errors and warnings from the context */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void Clear();

    /** Get operation timing information */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    float GetOperationDuration() const;

    /** Mark the operation as completed */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    void CompleteOperation();

    /** Get operation statistics */
    UFUNCTION(BlueprintCallable, Category = "MCP Operation Context")
    FString GetOperationStats() const;

protected:
    /** Name of the operation being tracked */
    UPROPERTY(BlueprintReadOnly, Category = "Operation Info")
    FString OperationName;

    /** Unique identifier for this operation */
    UPROPERTY(BlueprintReadOnly, Category = "Operation Info")
    FString OperationId;

    /** Timestamp when the operation started */
    UPROPERTY(BlueprintReadOnly, Category = "Operation Info")
    FDateTime StartTime;

    /** Timestamp when the operation completed */
    UPROPERTY(BlueprintReadOnly, Category = "Operation Info")
    FDateTime EndTime;

    /** Whether the operation has been completed */
    UPROPERTY(BlueprintReadOnly, Category = "Operation Info")
    bool bOperationCompleted;

    /** Collection of all errors encountered during the operation */
    UPROPERTY(BlueprintReadOnly, Category = "Errors")
    TArray<FMCPEnhancedError> Errors;

    /** Context information that applies to the entire operation */
    UPROPERTY(BlueprintReadOnly, Category = "Context")
    TMap<FString, FString> OperationContext;

private:
    /** Generate a unique operation ID */
    FString GenerateOperationId() const;

    /** Convert severity enum to string for logging */
    FString SeverityToString(EMCPErrorSeverity Severity) const;
};

/**
 * Utility macros for adding errors with source location information
 */
#define MCP_ADD_ERROR(Context, Error, Severity) \
    if (Context) { \
        Context->AddError(Error, Severity, FString::Printf(TEXT("%s:%d"), TEXT(__FILE__), __LINE__)); \
    }

#define MCP_ADD_WARNING(Context, Warning, Details) \
    if (Context) { \
        Context->AddWarning(Warning, Details, FString::Printf(TEXT("%s:%d"), TEXT(__FILE__), __LINE__)); \
    }

#define MCP_ADD_INFO(Context, Info, Details) \
    if (Context) { \
        Context->AddInfo(Info, Details); \
    }
