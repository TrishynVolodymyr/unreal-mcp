#pragma once

#include "CoreMinimal.h"
#include "MCPError.h"

// Forward declarations
class UMCPOperationContext;
struct FMCPEnhancedError;
enum class EMCPErrorSeverity : uint8;

/**
 * Centralized error handling utility for MCP operations
 * Provides standardized error creation, logging, and handling
 * Enhanced to work with operation contexts and structured error responses
 */
class UNREALMCP_API FMCPErrorHandler
{
public:
    /**
     * Create a standardized error for invalid parameters
     * @param Details Additional details about the parameter validation failure
     * @return FMCPError representing the invalid parameter error
     */
    static FMCPError CreateInvalidParametersError(const FString& Details = TEXT(""));

    /**
     * Create a standardized error for command not found
     * @param CommandName Name of the command that was not found
     * @return FMCPError representing the command not found error
     */
    static FMCPError CreateCommandNotFoundError(const FString& CommandName);

    /**
     * Create a standardized error for execution failure
     * @param Details Additional details about the execution failure
     * @return FMCPError representing the execution failure error
     */
    static FMCPError CreateExecutionFailedError(const FString& Details = TEXT(""));

    /**
     * Create a standardized error for validation failure
     * @param Details Additional details about the validation failure
     * @return FMCPError representing the validation failure error
     */
    static FMCPError CreateValidationFailedError(const FString& Details = TEXT(""));

    /**
     * Create a standardized error for internal errors
     * @param Details Additional details about the internal error
     * @return FMCPError representing the internal error
     */
    static FMCPError CreateInternalError(const FString& Details = TEXT(""));

    /**
     * Log an error to the Unreal Engine logging system
     * @param Error The error to log
     * @param LogCategory Optional log category (defaults to LogUnrealMCP)
     */
    static void LogError(const FMCPError& Error, const FLogCategoryBase* LogCategory = nullptr);

    /**
     * Handle an error by logging it and optionally performing additional actions
     * @param Error The error to handle
     * @param bShouldCrash Whether to crash the application on critical errors
     */
    static void HandleError(const FMCPError& Error, bool bShouldCrash = false);

    // Enhanced error handling methods for operation contexts

    /**
     * Create an enhanced error with context information
     * @param BaseError The base error information
     * @param Severity Severity level of the error
     * @param SourceLocation Source location where the error occurred
     * @param Context Additional context information
     * @return FMCPEnhancedError with full context
     */
    static FMCPEnhancedError CreateEnhancedError(const FMCPError& BaseError, EMCPErrorSeverity Severity, 
                                               const FString& SourceLocation = TEXT(""), 
                                               const TMap<FString, FString>& Context = TMap<FString, FString>());

    /**
     * Add an error to an operation context with automatic severity detection
     * @param Context The operation context to add the error to
     * @param Error The error to add
     * @param SourceLocation Source location where the error occurred
     */
    static void AddErrorToContext(UMCPOperationContext* Context, const FMCPError& Error, 
                                const FString& SourceLocation = TEXT(""));

    /**
     * Add an enhanced error to an operation context
     * @param Context The operation context to add the error to
     * @param Error The enhanced error to add
     */
    static void AddEnhancedErrorToContext(UMCPOperationContext* Context, const FMCPEnhancedError& Error);

    /**
     * Create a comprehensive error response from an operation context
     * @param Context The operation context containing errors
     * @param SuccessData Data to include if operation was successful
     * @param Metadata Additional metadata for the response
     * @return JSON string containing the complete response
     */
    static FString CreateContextualResponse(UMCPOperationContext* Context, 
                                          const FString& SuccessData = TEXT(""), 
                                          const FString& Metadata = TEXT(""));

    /**
     * Determine error severity based on error type
     * @param ErrorType The type of error
     * @return Appropriate severity level for the error type
     */
    static EMCPErrorSeverity DetermineErrorSeverity(EMCPErrorType ErrorType);

    /**
     * Create a structured error response for API consumption
     * @param Error The error to format
     * @param RequestId Optional request ID for tracking
     * @param AdditionalContext Additional context information
     * @return JSON formatted error response
     */
    static FString CreateStructuredErrorResponse(const FMCPError& Error, 
                                               const FString& RequestId = TEXT(""), 
                                               const TMap<FString, FString>& AdditionalContext = TMap<FString, FString>());

    /**
     * Aggregate multiple errors into a single response
     * @param Errors Array of errors to aggregate
     * @param OperationName Name of the operation that generated the errors
     * @return JSON formatted aggregated error response
     */
    static FString CreateAggregatedErrorResponse(const TArray<FMCPError>& Errors, 
                                               const FString& OperationName = TEXT(""));

    /**
     * Log an enhanced error with full context information
     * @param Error The enhanced error to log
     * @param LogCategory Optional log category
     */
    static void LogEnhancedError(const FMCPEnhancedError& Error, const FLogCategoryBase* LogCategory = nullptr);

    /**
     * Handle multiple errors from an operation context
     * @param Context The operation context containing errors
     * @param bShouldCrash Whether to crash on critical errors
     */
    static void HandleContextErrors(UMCPOperationContext* Context, bool bShouldCrash = false);

private:
    /** Get the next available error code for a given error type */
    static int32 GetNextErrorCode(EMCPErrorType ErrorType);

    /** Convert error type to default severity level */
    static EMCPErrorSeverity GetDefaultSeverityForErrorType(EMCPErrorType ErrorType);

    /** Format error context for logging */
    static FString FormatErrorContext(const TMap<FString, FString>& Context);
};
