// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Services/BlueprintAction/BlueprintActionDiscoveryService.h"

/**
 * Service for discovering Blueprint actions based on pin types
 * Handles searches for specific pin types (object, float, int, etc.) and their subcategories
 */
class FBlueprintPinSearchService : public FBlueprintActionDiscoveryService
{
public:
    FBlueprintPinSearchService();
    ~FBlueprintPinSearchService();

    /**
     * Get all available Blueprint actions for a specific pin type
     * 
     * @param PinType Type of the pin (object, float, int, bool, string, struct, etc.)
     * @param PinSubCategory Subcategory for object/struct pins (e.g., "PlayerController", "Vector")
     * @param SearchFilter Optional filter to narrow results (searches in name, keywords, category)
     * @param MaxResults Maximum number of results to return (default: 50)
     * @return JSON string with array of matching actions
     * 
     * Example usage:
     * - PinType="object", PinSubCategory="PlayerController" -> finds all functions for PlayerController
     * - PinType="float", PinSubCategory="" -> finds math operations for float
     * - PinType="struct", PinSubCategory="Vector" -> finds vector operations
     */
    FString GetActionsForPin(
        const FString& PinType,
        const FString& PinSubCategory,
        const FString& SearchFilter,
        int32 MaxResults
    );

private:
    /**
     * Resolve short class names to full paths
     * @param ShortName Short name like "PlayerController"
     * @return Full path like "/Script/Engine.PlayerController" or empty if not found
     */
    FString ResolveShortClassName(const FString& ShortName);

    /**
     * Check if node is relevant for math/numeric operations
     * @param TemplateNode Node to check
     * @param PinType Pin type being searched
     * @return true if node is relevant for math operations
     */
    bool IsRelevantForMathOperations(class UEdGraphNode* TemplateNode, const FString& PinType);

    /**
     * Check if node is relevant for object pin type
     * @param TemplateNode Node to check
     * @param TargetClass Class to check compatibility with
     * @return true if node is relevant for the object type
     */
    bool IsRelevantForObjectType(class UEdGraphNode* TemplateNode, class UClass* TargetClass);

    /**
     * Add native property getter/setter nodes for a class
     * @param TargetClass Class to get properties from
     * @param OutActions Output array to add property nodes to
     * @param MaxResults Maximum number to add
     */
    void AddNativePropertyNodes(
        class UClass* TargetClass,
        TArray<TSharedPtr<FJsonValue>>& OutActions,
        int32 MaxResults
    );
};
