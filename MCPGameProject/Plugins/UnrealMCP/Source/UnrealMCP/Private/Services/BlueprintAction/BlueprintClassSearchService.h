// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Services/BlueprintAction/BlueprintActionDiscoveryService.h"

/**
 * Service for discovering Blueprint actions based on class types
 * Handles searches for specific classes and their inheritance hierarchies
 */
class FBlueprintClassSearchService : public FBlueprintActionDiscoveryService
{
public:
    FBlueprintClassSearchService();
    ~FBlueprintClassSearchService();

    /**
     * Get all available Blueprint actions for a specific class
     * 
     * @param ClassName Name or full path of the class (e.g., "PlayerController" or "/Script/Engine.PlayerController")
     * @param SearchFilter Optional filter to narrow results
     * @param MaxResults Maximum number of results to return
     * @return JSON string with array of matching actions
     * 
     * Example usage:
     * - ClassName="PlayerController" -> finds all PlayerController functions
     * - ClassName="/Script/Engine.Pawn" -> finds all Pawn functions
     */
    FString GetActionsForClass(
        const FString& ClassName,
        const FString& SearchFilter,
        int32 MaxResults
    );

    /**
     * Get all available Blueprint actions for a class and its entire inheritance hierarchy
     * 
     * @param ClassName Name or full path of the class
     * @param SearchFilter Optional filter to narrow results
     * @param MaxResults Maximum number of results to return
     * @return JSON string with array of matching actions including inherited functions
     * 
     * Example usage:
     * - ClassName="Character" -> finds Character functions + Pawn functions + Actor functions
     */
    FString GetActionsForClassHierarchy(
        const FString& ClassName,
        const FString& SearchFilter,
        int32 MaxResults
    );

private:
    /**
     * Resolve class name to UClass pointer
     * Tries multiple search strategies (direct path, short name, common locations)
     * @param ClassName Name or path of class to find
     * @return UClass pointer or nullptr if not found
     */
    class UClass* ResolveClass(const FString& ClassName);

    /**
     * Build class hierarchy for a given class
     * @param TargetClass Class to get hierarchy for
     * @return Array of class names in hierarchy (child to parent order)
     */
    TArray<FString> BuildClassHierarchy(class UClass* TargetClass);

    /**
     * Count actions by category
     * @param Actions Array of action JSON objects
     * @return Map of category names to action counts
     */
    TMap<FString, int32> CountActionsByCategory(const TArray<TSharedPtr<FJsonValue>>& Actions);
};
