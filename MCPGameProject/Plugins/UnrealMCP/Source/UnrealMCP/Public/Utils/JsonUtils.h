#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

/**
 * JSON parsing and conversion utilities
 * Extracted from UnrealMCPCommonUtils for better code organization
 */
class UNREALMCP_API FJsonUtils
{
public:
    /**
     * Create a JSON error response object
     * @param Message - Error message
     * @return JSON object with success=false and error message
     */
    static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& Message);

    /**
     * Create a JSON success response object
     * @param Message - Optional success message
     * @return JSON object with success=true and optional message
     */
    static TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& Message = TEXT(""));

    /**
     * Extract integer array from JSON object
     * @param JsonObject - JSON object to read from
     * @param FieldName - Name of the array field
     * @param OutArray - Output array of integers
     */
    static void GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray);

    /**
     * Extract float array from JSON object
     * @param JsonObject - JSON object to read from
     * @param FieldName - Name of the array field
     * @param OutArray - Output array of floats
     */
    static void GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray);

    /**
     * Extract FVector2D from JSON object
     * @param JsonObject - JSON object to read from
     * @param FieldName - Name of the vector field (expects [x, y] array)
     * @return FVector2D parsed from JSON, or (0,0) if not found
     */
    static FVector2D GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);

    /**
     * Extract FVector from JSON object
     * @param JsonObject - JSON object to read from
     * @param FieldName - Name of the vector field (expects [x, y, z] array)
     * @return FVector parsed from JSON, or (0,0,0) if not found
     */
    static FVector GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);

    /**
     * Extract FRotator from JSON object
     * @param JsonObject - JSON object to read from
     * @param FieldName - Name of the rotator field (expects [pitch, yaw, roll] array)
     * @return FRotator parsed from JSON, or (0,0,0) if not found
     */
    static FRotator GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
};
