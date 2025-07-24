#pragma once

#include "CoreMinimal.h"
#include "MCPError.h"
#include "MCPResponse.generated.h"

/**
 * Standardized response structure for all MCP command operations
 * Provides consistent format for success/failure responses with data and error information
 */
USTRUCT(BlueprintType)
struct UNREALMCP_API FMCPResponse
{
    GENERATED_BODY()

    /** Whether the operation was successful */
    UPROPERTY(BlueprintReadOnly)
    bool bSuccess;

    /** Response data as JSON string */
    UPROPERTY(BlueprintReadOnly)
    FString Data;

    /** Error information if operation failed */
    UPROPERTY(BlueprintReadOnly)
    FMCPError Error;

    /** Additional metadata about the response */
    UPROPERTY(BlueprintReadOnly)
    FString Metadata;

    FMCPResponse()
        : bSuccess(false)
        , Data(TEXT(""))
        , Error()
        , Metadata(TEXT(""))
    {
    }

    /** Create a successful response with data */
    static FMCPResponse CreateSuccess(const FString& ResponseData, const FString& ResponseMetadata = TEXT(""))
    {
        FMCPResponse Response;
        Response.bSuccess = true;
        Response.Data = ResponseData;
        Response.Metadata = ResponseMetadata;
        return Response;
    }

    /** Create a failure response with error */
    static FMCPResponse CreateFailure(const FMCPError& ResponseError, const FString& ResponseMetadata = TEXT(""))
    {
        FMCPResponse Response;
        Response.bSuccess = false;
        Response.Error = ResponseError;
        Response.Metadata = ResponseMetadata;
        return Response;
    }

    /** Convert response to JSON string for transmission */
    FString ToJsonString() const;

    /** Create response from JSON string */
    static FMCPResponse FromJsonString(const FString& JsonString);
};
