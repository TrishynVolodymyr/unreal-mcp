#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"

/**
 * Geometry parsing utilities for vectors, rotators, and colors
 * Extracted from UnrealMCPCommonUtils for better code organization
 */
class UNREALMCP_API FGeometryUtils
{
public:
    /**
     * Parse FVector from JSON array [x, y, z]
     * @param JsonArray - JSON array with 3 numeric values
     * @param OutVector - Output vector
     * @return true if parsing succeeded
     */
    static bool ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector);

    /**
     * Parse FLinearColor from JSON array [r, g, b] or [r, g, b, a]
     * @param JsonArray - JSON array with 3-4 numeric values (0-1 range)
     * @param OutColor - Output color
     * @return true if parsing succeeded
     */
    static bool ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor);

    /**
     * Parse FRotator from JSON array [pitch, yaw, roll]
     * @param JsonArray - JSON array with 3 numeric values
     * @param OutRotator - Output rotator
     * @return true if parsing succeeded
     */
    static bool ParseRotator(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FRotator& OutRotator);
};
