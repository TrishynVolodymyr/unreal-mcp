// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprintNodeSpawner;
class UEdGraph;
class UEdGraphNode;
class UBlueprint;

/**
 * Data about a matched spawner for tracking duplicates and filtering.
 */
struct FMatchedSpawnerInfo
{
    const UBlueprintNodeSpawner* Spawner;
    FString DetectedClassName;
    FString FunctionName;
    FString NodeName;
    bool bExactMatch;

    FMatchedSpawnerInfo()
        : Spawner(nullptr)
        , bExactMatch(false)
    {}
};

/**
 * Service for matching and filtering Blueprint Action Database spawners.
 * Handles the complex logic of finding the correct spawner for a function name,
 * including duplicate detection, class filtering, and AnimBlueprint compatibility.
 */
class FActionSpawnerMatcher
{
public:
    /**
     * Build the list of search names for a function, including aliases and variations.
     *
     * @param FunctionName - Original function name
     * @return Array of search name variations to try
     */
    static TArray<FString> BuildSearchNames(const FString& FunctionName);

    /**
     * Search for matching spawners in the Blueprint Action Database.
     *
     * @param SearchNames - List of function name variations to search for
     * @param ClassName - Optional class name filter
     * @param OutMatchedSpawners - Output: list of matched spawner info
     * @param OutMatchingFunctionsByClass - Output: map of function names to classes for duplicate detection
     */
    static void FindMatchingSpawners(
        const TArray<FString>& SearchNames,
        const FString& ClassName,
        TArray<FMatchedSpawnerInfo>& OutMatchedSpawners,
        TMap<FString, TArray<FString>>& OutMatchingFunctionsByClass
    );

    /**
     * Check if there are duplicate functions (same name, different classes) without class disambiguation.
     *
     * @param MatchingFunctionsByClass - Map of function names to classes
     * @param ClassName - The class name specified (empty if none)
     * @param FunctionName - Original function name for error message
     * @param OutErrorMessage - Output: error message if duplicates found
     * @return True if duplicates found and no class specified
     */
    static bool HasUnresolvedDuplicates(
        const TMap<FString, TArray<FString>>& MatchingFunctionsByClass,
        const FString& ClassName,
        const FString& FunctionName,
        FString& OutErrorMessage
    );

    /**
     * Select a compatible spawner from matched spawners, filtering for Blueprint type.
     *
     * @param MatchedSpawners - List of candidate spawners
     * @param TargetBlueprint - The Blueprint to check compatibility with
     * @return The selected compatible spawner, or nullptr if none found
     */
    static const UBlueprintNodeSpawner* SelectCompatibleSpawner(
        const TArray<FMatchedSpawnerInfo>& MatchedSpawners,
        UBlueprint* TargetBlueprint
    );

    /**
     * Check if a spawner requires AnimBlueprint context.
     *
     * @param Spawner - The spawner to check
     * @return True if the spawner only works in AnimBlueprint
     */
    static bool RequiresAnimBlueprint(const UBlueprintNodeSpawner* Spawner);

    /**
     * Build an error message with suggestions when function is not found.
     *
     * @param FunctionName - The function name that wasn't found
     * @param SearchNames - The search variations that were tried
     * @return Error message with suggestions
     */
    static FString BuildNotFoundErrorMessage(
        const FString& FunctionName,
        const TArray<FString>& SearchNames
    );

private:
    /**
     * Get operation aliases map (Add -> Add_FloatFloat, Add_IntInt, etc.)
     */
    static const TMap<FString, TArray<FString>>& GetOperationAliases();

    /**
     * Normalize function name by removing common UE prefixes (K2_, BP_, etc.)
     */
    static FString NormalizeFunctionName(const FString& Name);

    /**
     * Check if a class name matches the expected class (handling prefixes and common names).
     */
    static bool ClassNameMatches(const FString& ActualClassName, const FString& ExpectedClassName);
};
