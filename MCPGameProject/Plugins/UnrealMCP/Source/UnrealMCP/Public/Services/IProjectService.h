#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Interface for Project-related operations
 * Handles input mappings, folder creation, struct management, and enhanced input
 */
class UNREALMCP_API IProjectService
{
public:
    virtual ~IProjectService() = default;

    // Input mapping operations
    virtual bool CreateInputMapping(const FString& ActionName, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) = 0;
    
    // Folder operations
    virtual bool CreateFolder(const FString& FolderPath, bool& bOutAlreadyExists, FString& OutError) = 0;
    virtual TArray<FString> ListFolderContents(const FString& FolderPath, bool& bOutSuccess, FString& OutError) = 0;
    
    // Struct operations
    virtual bool CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError) = 0;
    virtual bool UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError) = 0;

    // Enum operations
    // ValueDescriptions is a map from value name to its description (optional)
    virtual bool CreateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutFullPath, FString& OutError) = 0;
    virtual bool UpdateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutError) = 0;
    
    // Enhanced Input operations
    virtual bool CreateEnhancedInputAction(const FString& ActionName, const FString& Path, const FString& Description, const FString& ValueType, FString& OutAssetPath, FString& OutError) = 0;
    virtual bool CreateInputMappingContext(const FString& ContextName, const FString& Path, const FString& Description, FString& OutAssetPath, FString& OutError) = 0;
    virtual bool AddMappingToContext(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputActions(const FString& Path, bool& bOutSuccess, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputMappingContexts(const FString& Path, bool& bOutSuccess, FString& OutError) = 0;
    
    // Utility operations
    virtual FString GetProjectDirectory() const = 0;

    // Asset operations
    virtual bool DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError) = 0;
    virtual bool RenameAsset(const FString& AssetPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError) = 0;
    virtual bool MoveAsset(const FString& AssetPath, const FString& DestinationFolder, FString& OutNewAssetPath, FString& OutError) = 0;
    virtual TArray<TSharedPtr<FJsonObject>> SearchAssets(const FString& Pattern, const FString& AssetClass, const FString& Folder, bool& bOutSuccess, FString& OutError) = 0;

    // DataAsset operations
    virtual bool CreateDataAsset(const FString& Name, const FString& AssetClass, const FString& FolderPath, const TSharedPtr<FJsonObject>& Properties, FString& OutAssetPath, FString& OutError) = 0;
    virtual bool SetDataAssetProperty(const FString& AssetPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) = 0;
    virtual TSharedPtr<FJsonObject> GetDataAssetMetadata(const FString& AssetPath, FString& OutError) = 0;

    // Font Face operations (for TTF-based fonts)
    virtual bool CreateFontFace(const FString& FontName, const FString& Path, const FString& SourceTexturePath, bool bUseSDF, int32 DistanceFieldSpread, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError) = 0;
    virtual bool SetFontFaceProperties(const FString& FontPath, const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, TArray<FString>& OutFailedProperties, FString& OutError) = 0;
    virtual TSharedPtr<FJsonObject> GetFontFaceMetadata(const FString& FontPath, FString& OutError) = 0;

    // TTF Import - imports an external TTF file as a FontFace asset
    // TTFFilePath: Absolute file path to the TTF file on disk
    virtual bool ImportTTFFont(const FString& FontName, const FString& Path, const FString& TTFFilePath, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError) = 0;

    // Offline Font operations (for SDF atlas-based fonts)
    // MetricsFilePath: Absolute file path to the metrics JSON file on disk (not an Unreal asset path)
    virtual bool CreateOfflineFont(const FString& FontName, const FString& Path, const FString& TexturePath, const FString& MetricsFilePath, FString& OutAssetPath, FString& OutError) = 0;
    virtual TSharedPtr<FJsonObject> GetFontMetadata(const FString& FontPath, FString& OutError) = 0;
};
