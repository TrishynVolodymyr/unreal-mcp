// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Services/BlueprintAction/BlueprintActionDiscoveryService.h"

/**
 * Service for general Blueprint action search
 * Handles keyword-based searches across all available actions with category filtering
 */
class FBlueprintActionSearchService : public FBlueprintActionDiscoveryService
{
public:
    FBlueprintActionSearchService();
    ~FBlueprintActionSearchService();

    /**
     * Search for Blueprint actions using keywords
     * 
     * @param SearchQuery Search string (searches in name, keywords, category, tooltip)
     * @param Category Optional category filter (Flow Control, Math, Utilities, etc.)
     * @param MaxResults Maximum number of results to return
     * @param BlueprintName Optional Blueprint name for local variable discovery
     * @return JSON string with array of matching actions
     * 
     * Example usage:
     * - SearchQuery="add" -> finds addition operations
     * - SearchQuery="branch", Category="Flow Control" -> finds conditional nodes
     * - SearchQuery="print" -> finds Print String nodes
     */
    FString SearchBlueprintActions(
        const FString& SearchQuery,
        const FString& Category,
        int32 MaxResults,
        const FString& BlueprintName
    );

private:
    /**
     * Check if action matches search query
     * @param ActionName Action name to check
     * @param Category Action category
     * @param Tooltip Action tooltip
     * @param Keywords Action keywords
     * @param SearchQuery Query to match against
     * @return true if action matches query
     */
    bool MatchesSearchQuery(
        const FString& ActionName,
        const FString& Category,
        const FString& Tooltip,
        const FString& Keywords,
        const FString& SearchQuery
    );

    /**
     * Check if action matches category filter
     * @param ActionCategory Category of the action
     * @param CategoryFilter Filter to match against
     * @return true if matches or no filter specified
     */
    bool MatchesCategoryFilter(
        const FString& ActionCategory,
        const FString& CategoryFilter
    );

    /**
     * Discover local variables in a Blueprint and add them as actions
     * @param BlueprintName Name of Blueprint to get variables from
     * @param OutActions Output array to add variable actions to
     * @param SearchQuery Optional filter for variable names
     */
    void DiscoverLocalVariables(
        const FString& BlueprintName,
        TArray<TSharedPtr<FJsonValue>>& OutActions,
        const FString& SearchQuery
    );
};
