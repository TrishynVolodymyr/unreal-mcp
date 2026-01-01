#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Service for creating and managing font assets.
 * Handles FontFace creation, TTF import, offline/bitmap fonts, and metadata.
 */
class UNREALMCP_API FProjectFontService
{
public:
    /**
     * Get the singleton instance
     */
    static FProjectFontService& Get();

    /**
     * Create a FontFace asset from an SDF texture.
     * @param FontName - Name of the FontFace asset
     * @param Path - Content browser path
     * @param SourceTexturePath - Path to SDF texture
     * @param bUseSDF - Whether to use SDF rendering
     * @param DistanceFieldSpread - SDF spread value
     * @param FontMetrics - Optional font metrics
     * @param OutAssetPath - Output: created asset path
     * @param OutError - Output: error message if failed
     * @return true if created successfully
     */
    bool CreateFontFace(
        const FString& FontName,
        const FString& Path,
        const FString& SourceTexturePath,
        bool bUseSDF,
        int32 DistanceFieldSpread,
        const TSharedPtr<FJsonObject>& FontMetrics,
        FString& OutAssetPath,
        FString& OutError);

    /**
     * Import a TTF font file and create UFont asset.
     * @param FontName - Name of the font asset
     * @param Path - Content browser path
     * @param TTFFilePath - Absolute path to TTF file on disk
     * @param FontMetrics - Optional font metrics
     * @param OutAssetPath - Output: created asset path
     * @param OutError - Output: error message if failed
     * @return true if imported successfully
     */
    bool ImportTTFFont(
        const FString& FontName,
        const FString& Path,
        const FString& TTFFilePath,
        const TSharedPtr<FJsonObject>& FontMetrics,
        FString& OutAssetPath,
        FString& OutError);

    /**
     * Set properties on an existing FontFace asset.
     * @param FontPath - Path to the FontFace asset
     * @param Properties - Properties to set
     * @param OutSuccessProperties - Properties that were set successfully
     * @param OutFailedProperties - Properties that failed to set
     * @param OutError - Output: error message if failed
     * @return true if at least some properties were set
     */
    bool SetFontFaceProperties(
        const FString& FontPath,
        const TSharedPtr<FJsonObject>& Properties,
        TArray<FString>& OutSuccessProperties,
        TArray<FString>& OutFailedProperties,
        FString& OutError);

    /**
     * Get metadata about a FontFace asset.
     * @param FontPath - Path to the FontFace asset
     * @param OutError - Output: error message if failed
     * @return JSON object with font metadata, or nullptr on failure
     */
    TSharedPtr<FJsonObject> GetFontFaceMetadata(const FString& FontPath, FString& OutError);

    /**
     * Create an offline/bitmap font from texture atlas and metrics.
     * @param FontName - Name of the font asset
     * @param Path - Content browser path
     * @param TexturePath - Path to texture atlas in UE
     * @param MetricsFilePath - Absolute path to metrics JSON on disk
     * @param OutAssetPath - Output: created asset path
     * @param OutError - Output: error message if failed
     * @return true if created successfully
     */
    bool CreateOfflineFont(
        const FString& FontName,
        const FString& Path,
        const FString& TexturePath,
        const FString& MetricsFilePath,
        FString& OutAssetPath,
        FString& OutError);

    /**
     * Get metadata about a UFont asset.
     * @param FontPath - Path to the font asset
     * @param OutError - Output: error message if failed
     * @return JSON object with font metadata, or nullptr on failure
     */
    TSharedPtr<FJsonObject> GetFontMetadata(const FString& FontPath, FString& OutError);

private:
    FProjectFontService() = default;
};
