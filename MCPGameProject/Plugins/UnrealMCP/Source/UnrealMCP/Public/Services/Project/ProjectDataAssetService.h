#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

/**
 * Service for creating and managing DataAsset instances.
 * Handles creation, property setting, and metadata queries.
 */
class UNREALMCP_API FProjectDataAssetService
{
public:
    /**
     * Get the singleton instance
     */
    static FProjectDataAssetService& Get();

    /**
     * Create a new DataAsset.
     * @param Name - Name for the new DataAsset
     * @param AssetClass - Class type for the DataAsset (e.g., UPrimaryDataAsset)
     * @param FolderPath - Content browser path for the asset
     * @param Properties - Optional initial property values
     * @param OutAssetPath - Output: full path of created asset
     * @param OutError - Output: error message if failed
     * @return true if asset was created successfully
     */
    bool CreateDataAsset(
        const FString& Name,
        const FString& AssetClass,
        const FString& FolderPath,
        const TSharedPtr<FJsonObject>& Properties,
        FString& OutAssetPath,
        FString& OutError);

    /**
     * Set a property value on a DataAsset.
     * @param AssetPath - Path of the DataAsset
     * @param PropertyName - Name of the property to set
     * @param PropertyValue - Value to set
     * @param OutError - Output: error message if failed
     * @return true if property was set successfully
     */
    bool SetDataAssetProperty(
        const FString& AssetPath,
        const FString& PropertyName,
        const TSharedPtr<FJsonValue>& PropertyValue,
        FString& OutError);

    /**
     * Get metadata about a DataAsset including its properties.
     * @param AssetPath - Path of the DataAsset
     * @param OutError - Output: error message if failed
     * @return JSON object with asset metadata, or nullptr on failure
     */
    TSharedPtr<FJsonObject> GetDataAssetMetadata(
        const FString& AssetPath,
        FString& OutError);

private:
    FProjectDataAssetService() = default;
};
