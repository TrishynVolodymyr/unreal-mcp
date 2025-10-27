#pragma once

#include "CoreMinimal.h"
#include "MCPError.generated.h"

/**
 * Enumeration of MCP error types for categorizing different kinds of errors
 */
UENUM(BlueprintType)
enum class EMCPErrorType : uint8
{
    None = 0,
    InvalidParameters,
    CommandNotFound,
    ExecutionFailed,
    ValidationFailed,
    InternalError,
    NetworkError,
    TimeoutError
};

/**
 * Structure representing an MCP error with type, code, and message
 */
USTRUCT(BlueprintType)
struct UNREALMCP_API FMCPError
{
    GENERATED_BODY()

    /** Type of error that occurred */
    UPROPERTY(BlueprintReadOnly)
    EMCPErrorType ErrorType;

    /** Numeric error code for programmatic handling */
    UPROPERTY(BlueprintReadOnly)
    int32 ErrorCode;

    /** Human-readable error message */
    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;

    /** Additional context or details about the error */
    UPROPERTY(BlueprintReadOnly)
    FString ErrorDetails;

    FMCPError()
        : ErrorType(EMCPErrorType::None)
        , ErrorCode(0)
        , ErrorMessage(TEXT(""))
        , ErrorDetails(TEXT(""))
    {
    }

    FMCPError(EMCPErrorType InErrorType, int32 InErrorCode, const FString& InErrorMessage, const FString& InErrorDetails = TEXT(""))
        : ErrorType(InErrorType)
        , ErrorCode(InErrorCode)
        , ErrorMessage(InErrorMessage)
        , ErrorDetails(InErrorDetails)
    {
    }

    /** Check if this represents an error condition */
    bool HasError() const
    {
        return ErrorType != EMCPErrorType::None;
    }

    /** Convert error to JSON string for transmission */
    FString ToJsonString() const;

    /** Create error from JSON string */
    static FMCPError FromJsonString(const FString& JsonString);
};
