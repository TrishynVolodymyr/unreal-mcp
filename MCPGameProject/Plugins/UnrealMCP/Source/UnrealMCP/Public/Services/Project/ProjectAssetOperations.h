#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Service for general asset management operations.
 * Handles rename, move, search, duplicate, and delete operations.
 */
class UNREALMCP_API FProjectAssetOperations
{
public:
    /**
     * Get the singleton instance
     */
    static FProjectAssetOperations& Get();

    /**
     * Duplicate an existing asset to a new location.
     * @param SourcePath - Path of the asset to duplicate
     * @param DestinationPath - Folder path for the new asset
     * @param NewName - Name for the duplicated asset
     * @param OutNewAssetPath - Output: full path of the new asset
     * @param OutError - Output: error message if failed
     * @return true if asset was duplicated successfully
     */
    bool DuplicateAsset(
        const FString& SourcePath,
        const FString& DestinationPath,
        const FString& NewName,
        FString& OutNewAssetPath,
        FString& OutError);

    /**
     * Delete an asset.
     * @param AssetPath - Path of the asset to delete
     * @param OutError - Output: error message if failed
     * @return true if asset was deleted successfully
     */
    bool DeleteAsset(
        const FString& AssetPath,
        FString& OutError);

    /**
     * Rename an asset.
     * @param AssetPath - Current path of the asset
     * @param NewName - New name for the asset
     * @param OutNewAssetPath - Output: new full path of the asset
     * @param OutError - Output: error message if failed
     * @return true if asset was renamed successfully
     */
    bool RenameAsset(
        const FString& AssetPath,
        const FString& NewName,
        FString& OutNewAssetPath,
        FString& OutError);

    /**
     * Move an asset to a different folder.
     * @param AssetPath - Current path of the asset
     * @param DestinationFolder - Target folder path
     * @param OutNewAssetPath - Output: new full path of the asset
     * @param OutError - Output: error message if failed
     * @return true if asset was moved successfully
     */
    bool MoveAsset(
        const FString& AssetPath,
        const FString& DestinationFolder,
        FString& OutNewAssetPath,
        FString& OutError);

    /**
     * Search for assets matching criteria.
     * @param Pattern - Name pattern to search for (supports wildcards)
     * @param AssetClass - Optional: filter by asset class name
     * @param Folder - Optional: limit search to specific folder
     * @param bOutSuccess - Output: whether search succeeded
     * @param OutError - Output: error message if failed
     * @return Array of JSON objects describing matching assets
     */
    TArray<TSharedPtr<FJsonObject>> SearchAssets(
        const FString& Pattern,
        const FString& AssetClass,
        const FString& Folder,
        bool& bOutSuccess,
        FString& OutError);

private:
    FProjectAssetOperations() = default;
};
