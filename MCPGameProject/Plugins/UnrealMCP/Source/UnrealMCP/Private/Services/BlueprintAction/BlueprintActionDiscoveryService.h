// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

class UBlueprintNodeSpawner;
class UBlueprintFunctionNodeSpawner;
class UBlueprintEventNodeSpawner;

/**
 * Base service for Blueprint action discovery with shared utilities
 * Provides common functionality for processing node spawners and JSON serialization
 */
class FBlueprintActionDiscoveryService
{
public:
    FBlueprintActionDiscoveryService();
    ~FBlueprintActionDiscoveryService();

protected:
    /**
     * Process a UBlueprintFunctionNodeSpawner and extract metadata
     * @param NodeSpawner The function node spawner to process
     * @param OutActionName Extracted action name
     * @param OutCategory Extracted category
     * @param OutTooltip Extracted tooltip
     * @param OutKeywords Extracted keywords
     * @param OutNodeType Extracted node type
     * @return true if successfully processed
     */
    bool ProcessFunctionNodeSpawner(
        UBlueprintFunctionNodeSpawner* NodeSpawner,
        FString& OutActionName,
        FString& OutCategory,
        FString& OutTooltip,
        FString& OutKeywords,
        FString& OutNodeType
    );

    /**
     * Process a UBlueprintEventNodeSpawner and extract metadata
     * @param NodeSpawner The event node spawner to process
     * @param OutActionName Extracted action name
     * @param OutCategory Extracted category
     * @param OutTooltip Extracted tooltip
     * @param OutKeywords Extracted keywords
     * @param OutNodeType Extracted node type
     * @return true if successfully processed
     */
    bool ProcessEventNodeSpawner(
        UBlueprintEventNodeSpawner* NodeSpawner,
        FString& OutActionName,
        FString& OutCategory,
        FString& OutTooltip,
        FString& OutKeywords,
        FString& OutNodeType
    );

    /**
     * Build a JSON result object for action data
     * @param Actions Array of action JSON objects
     * @param bSuccess Success status
     * @param Message Result message
     * @param AdditionalFields Optional additional fields to add
     * @return JSON string
     */
    FString BuildActionResult(
        const TArray<TSharedPtr<FJsonValue>>& Actions,
        bool bSuccess,
        const FString& Message,
        TSharedPtr<FJsonObject> AdditionalFields = nullptr
    );

    /**
     * Detect duplicate function names in action results
     * @param Actions Array of actions to check
     * @return Map of function names to list of class names where they appear
     */
    TMap<FString, TArray<FString>> DetectDuplicateFunctions(
        const TArray<TSharedPtr<FJsonValue>>& Actions
    );

    /**
     * Add duplicate warning to result message
     * @param Duplicates Map of duplicate function names to classes
     * @param BaseMessage Base message to prepend warning to
     * @return Message with duplicate warning prepended
     */
    FString AddDuplicateWarning(
        const TMap<FString, TArray<FString>>& Duplicates,
        const FString& BaseMessage
    );
};
