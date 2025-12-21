#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"


/**
 * Service for discovering and loading assets in the project
 * Handles the missing functionality from UnrealMCPCommonUtils
 */
class UNREALMCP_API FAssetDiscoveryService
{
public:
    static FAssetDiscoveryService& Get();

    // Asset discovery methods
    TArray<FString> FindAssetsByType(const FString& AssetType, const FString& SearchPath = TEXT("/Game"));
    TArray<FString> FindAssetsByName(const FString& AssetName, const FString& SearchPath = TEXT("/Game"));
    TArray<FString> FindWidgetBlueprints(const FString& WidgetName = TEXT(""), const FString& SearchPath = TEXT("/Game"));
    TArray<FString> FindBlueprints(const FString& BlueprintName = TEXT(""), const FString& SearchPath = TEXT("/Game"));
    TArray<FString> FindDataTables(const FString& TableName = TEXT(""), const FString& SearchPath = TEXT("/Game"));

    // Enhanced asset loading
    UClass* FindWidgetClass(const FString& WidgetPath);
    UWidgetBlueprint* FindWidgetBlueprint(const FString& WidgetPath);
    UObject* FindAssetByPath(const FString& AssetPath);
    UObject* FindAssetByName(const FString& AssetName, const FString& AssetType = TEXT(""));
    UScriptStruct* FindStructType(const FString& StructPath);
    UEnum* FindEnumType(const FString& EnumPath);

    // Asset path utilities
    TArray<FString> GetCommonAssetSearchPaths(const FString& AssetName);
    FString NormalizeAssetPath(const FString& AssetPath);
    bool IsValidAssetPath(const FString& AssetPath);

    // Class resolution utilities
    UClass* ResolveObjectClass(const FString& ClassName);
    UClass* ResolveUMGClass(const FString& ClassName);
    UClass* ResolveEngineClass(const FString& ClassName);

private:
    FAssetDiscoveryService() = default;
    ~FAssetDiscoveryService() = default;

    // Helper methods
    FString BuildGamePath(const FString& Path);
    FString BuildEnginePath(const FString& Path);
    FString BuildCorePath(const FString& Path);
    FString BuildUMGPath(const FString& Path);
};