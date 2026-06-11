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

    /**
     * Create a new asset of ANY UObject class (generic — not restricted to
     * UDataAsset). Use for Voxel assets, etc. that have no factory MCP tool.
     * @param Name - Name for the new asset
     * @param AssetClass - Class path or name (e.g. "/Script/Voxel.VoxelSurfaceTypeAsset")
     * @param FolderPath - Content browser folder (default /Game)
     * @param OutAssetPath - Output: full object path of created asset
     * @param OutError - Output: detailed error message if failed
     * @return true on success
     */
    bool CreateAsset(
        const FString& Name,
        const FString& AssetClass,
        const FString& FolderPath,
        FString& OutAssetPath,
        FString& OutError);

    /**
     * Set ANY property on ANY loaded UObject asset via UE property text import
     * (ImportText). Handles numerics, bools, strings, names, enums, object
     * references (by path), structs, and arrays — anything ExportText round-trips.
     * @param AssetPath - Path of the asset
     * @param PropertyName - Reflected property name
     * @param ValueString - UE property-text value (e.g. "/Game/X.X" for object ref,
     *        "(\"/Game/A.A\",\"/Game/B.B\")" for an array of object refs, "5.0", "true")
     * @param OutError - Output: detailed error (incl. UE import parser message)
     * @param OutAppliedValue - Optional output: the property value RE-EXPORTED from the
     *        asset AFTER import + PostEditChangeProperty (what will actually persist).
     *        Lets the caller verify the write took without trusting the success flag —
     *        a PostEditChange that sanitizes/reverts the value shows up here.
     * @return true on success
     */
    bool SetObjectProperty(
        const FString& AssetPath,
        const FString& PropertyName,
        const FString& ValueString,
        FString& OutError,
        FString* OutAppliedValue = nullptr);

private:
    FProjectDataAssetService() = default;
};
