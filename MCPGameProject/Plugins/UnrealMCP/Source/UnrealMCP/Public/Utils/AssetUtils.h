#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UBlueprint;
class UUserWidget;
class UScriptStruct;

/**
 * Asset discovery and loading utilities
 * Extracted from UnrealMCPCommonUtils for better code organization
 */
class UNREALMCP_API FAssetUtils
{
public:
    /**
     * Find a widget class by path or name with multiple search strategies
     * @param WidgetPath - Widget path or name to search for
     * @return Found widget class or nullptr
     */
    static UClass* FindWidgetClass(const FString& WidgetPath);

    /**
     * Find a widget blueprint by path or name
     * @param WidgetPath - Widget blueprint path or name
     * @return Found widget blueprint or nullptr
     */
    static UBlueprint* FindWidgetBlueprint(const FString& WidgetPath);

    /**
     * Find an asset by its full path
     * @param AssetPath - Full asset path
     * @return Found asset or nullptr
     */
    static UObject* FindAssetByPath(const FString& AssetPath);

    /**
     * Find an asset by name and optional type
     * @param AssetName - Asset name to search for
     * @param AssetType - Optional asset type filter
     * @return Found asset or nullptr
     */
    static UObject* FindAssetByName(const FString& AssetName, const FString& AssetType = TEXT(""));

    /**
     * Find a struct type by path or name
     * @param StructPath - Struct path or name
     * @return Found struct or nullptr
     */
    static UScriptStruct* FindStructType(const FString& StructPath);

    /**
     * Generate common asset search paths for a given asset name
     * @param AssetName - Asset name to generate paths for
     * @return Array of potential asset paths
     */
    static TArray<FString> GetCommonAssetSearchPaths(const FString& AssetName);

    /**
     * Normalize an asset path by removing prefixes and cleaning
     * @param AssetPath - Asset path to normalize
     * @return Normalized asset path
     */
    static FString NormalizeAssetPath(const FString& AssetPath);

    /**
     * Check if an asset path is valid
     * @param AssetPath - Asset path to validate
     * @return true if asset exists at path
     */
    static bool IsValidAssetPath(const FString& AssetPath);

    /**
     * Find assets by type (e.g., "Blueprint", "WidgetBlueprint")
     * @param AssetType - Type of asset to search for
     * @param SearchPath - Path to search in (default: /Game)
     * @return Array of asset paths
     */
    static TArray<FString> FindAssetsByType(const FString& AssetType, const FString& SearchPath = TEXT("/Game"));

    /**
     * Find assets by name
     * @param AssetName - Asset name to search for
     * @param SearchPath - Path to search in (default: /Game)
     * @return Array of asset paths
     */
    static TArray<FString> FindAssetsByName(const FString& AssetName, const FString& SearchPath = TEXT("/Game"));

    /**
     * Find widget blueprints
     * @param WidgetName - Optional widget name filter
     * @param SearchPath - Path to search in (default: /Game)
     * @return Array of widget blueprint paths
     */
    static TArray<FString> FindWidgetBlueprints(const FString& WidgetName = TEXT(""), const FString& SearchPath = TEXT("/Game"));

private:
    // Helper methods used internally
    static FString BuildEnginePath(const FString& Path);
    static FString BuildCorePath(const FString& Path);
};
