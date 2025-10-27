#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Helper functions for Blueprint node creation operations
 * Extracted from UnrealMCPBlueprintActionCommands to improve maintainability
 */
class UNREALMCP_API UnrealMCPNodeCreationHelpers
{
public:
    /**
     * Parse JSON parameters with error handling
     * @param JsonParams - JSON string to parse
     * @param OutParamsObject - Parsed JSON object output
     * @param OutResultObj - Result object for error reporting
     * @return true if parsing succeeded or JsonParams was empty
     */
    static bool ParseJsonParameters(const FString& JsonParams, TSharedPtr<FJsonObject>& OutParamsObject, TSharedPtr<FJsonObject>& OutResultObj);
}; 
