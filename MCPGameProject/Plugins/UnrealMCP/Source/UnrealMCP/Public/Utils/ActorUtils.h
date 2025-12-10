#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// Forward declarations
class AActor;

/**
 * Actor manipulation and serialization utilities
 * Extracted from UnrealMCPCommonUtils for better code organization
 */
class UNREALMCP_API FActorUtils
{
public:
    /**
     * Convert an actor to JSON value format
     * @param Actor - Actor to convert
     * @return JSON value representation
     */
    static TSharedPtr<FJsonValue> ActorToJson(AActor* Actor);

    /**
     * Convert an actor to JSON object format
     * @param Actor - Actor to convert
     * @param bDetailed - Whether to include detailed information
     * @return JSON object representation
     */
    static TSharedPtr<FJsonObject> ActorToJsonObject(AActor* Actor, bool bDetailed = false);

    /**
     * Find an actor in the world by name
     * @param ActorName - Name of actor to find
     * @return Found actor or nullptr
     */
    static AActor* FindActorByName(const FString& ActorName);

    /**
     * Call a BlueprintCallable function by name with string parameters
     * @param Target - Object to call function on
     * @param FunctionName - Name of function to call
     * @param StringParams - Array of string parameters
     * @param OutError - Error message if call fails
     * @return true if function was called successfully
     */
    static bool CallFunctionByName(UObject* Target, const FString& FunctionName,
                                   const TArray<FString>& StringParams, FString& OutError);
};
