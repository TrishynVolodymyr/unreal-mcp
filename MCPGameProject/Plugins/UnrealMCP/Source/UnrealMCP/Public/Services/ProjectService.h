#pragma once

#include "CoreMinimal.h"
#include "Services/IProjectService.h"

/**
 * Concrete implementation of Project service
 * Handles all project-related operations including input mappings, folders, structs, and enhanced input
 */
class UNREALMCP_API FProjectService : public IProjectService
{
public:
    FProjectService();
    virtual ~FProjectService() = default;

    // IProjectService interface
    virtual bool CreateInputMapping(const FString& ActionName, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) override;
    virtual bool CreateFolder(const FString& FolderPath, bool& bOutAlreadyExists, FString& OutError) override;
    virtual TArray<FString> ListFolderContents(const FString& FolderPath, bool& bOutSuccess, FString& OutError) override;
    virtual bool CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError) override;
    virtual bool UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError) override;
    virtual bool CreateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutFullPath, FString& OutError) override;
    virtual bool UpdateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutError) override;
    virtual bool CreateEnhancedInputAction(const FString& ActionName, const FString& Path, const FString& Description, const FString& ValueType, FString& OutAssetPath, FString& OutError) override;
    virtual bool CreateInputMappingContext(const FString& ContextName, const FString& Path, const FString& Description, FString& OutAssetPath, FString& OutError) override;
    virtual bool AddMappingToContext(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputActions(const FString& Path, bool& bOutSuccess, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> ListInputMappingContexts(const FString& Path, bool& bOutSuccess, FString& OutError) override;
    virtual FString GetProjectDirectory() const override;
    virtual bool DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError) override;
    virtual bool DeleteAsset(const FString& AssetPath, FString& OutError) override;
    virtual bool RenameAsset(const FString& AssetPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError) override;
    virtual bool MoveAsset(const FString& AssetPath, const FString& DestinationFolder, FString& OutNewAssetPath, FString& OutError) override;
    virtual TArray<TSharedPtr<FJsonObject>> SearchAssets(const FString& Pattern, const FString& AssetClass, const FString& Folder, bool& bOutSuccess, FString& OutError) override;

    // DataAsset operations
    virtual bool CreateDataAsset(const FString& Name, const FString& AssetClass, const FString& FolderPath, const TSharedPtr<FJsonObject>& Properties, FString& OutAssetPath, FString& OutError) override;
    virtual bool SetDataAssetProperty(const FString& AssetPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) override;
    virtual TSharedPtr<FJsonObject> GetDataAssetMetadata(const FString& AssetPath, FString& OutError) override;

    // Font Face operations (for TTF-based fonts)
    virtual bool CreateFontFace(const FString& FontName, const FString& Path, const FString& SourceTexturePath, bool bUseSDF, int32 DistanceFieldSpread, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError) override;
    virtual bool SetFontFaceProperties(const FString& FontPath, const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, TArray<FString>& OutFailedProperties, FString& OutError) override;
    virtual TSharedPtr<FJsonObject> GetFontFaceMetadata(const FString& FontPath, FString& OutError) override;

    // TTF Import - imports an external TTF file as a FontFace asset
    virtual bool ImportTTFFont(const FString& FontName, const FString& Path, const FString& TTFFilePath, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError) override;

    // Offline Font operations (for SDF atlas-based fonts)
    virtual bool CreateOfflineFont(const FString& FontName, const FString& Path, const FString& TexturePath, const FString& MetricsFilePath, FString& OutAssetPath, FString& OutError) override;
    virtual TSharedPtr<FJsonObject> GetFontMetadata(const FString& FontPath, FString& OutError) override;

private:
    // Helper methods for struct operations
    bool CreateStructProperty(class UUserDefinedStruct* Struct, const TSharedPtr<FJsonObject>& PropertyObj) const;
};
