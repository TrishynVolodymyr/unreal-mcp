#include "Services/ProjectService.h"
#include "Services/Project/ProjectStructService.h"
#include "Services/Project/ProjectEnumService.h"
#include "Services/Project/ProjectFontService.h"
#include "Services/Project/ProjectAssetOperations.h"
#include "Services/Project/ProjectDataAssetService.h"
#include "GameFramework/InputSettings.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

FProjectService::FProjectService()
{
}

// ============================================
// Input Mapping Operations
// ============================================

bool FProjectService::CreateInputMapping(const FString& ActionName, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError)
{
    // Get the input settings
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        OutError = TEXT("Failed to get input settings");
        return false;
    }

    // Create the input action mapping
    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    // Add modifiers if provided
    if (Modifiers.IsValid())
    {
        if (Modifiers->HasField(TEXT("shift")))
        {
            ActionMapping.bShift = Modifiers->GetBoolField(TEXT("shift"));
        }
        if (Modifiers->HasField(TEXT("ctrl")))
        {
            ActionMapping.bCtrl = Modifiers->GetBoolField(TEXT("ctrl"));
        }
        if (Modifiers->HasField(TEXT("alt")))
        {
            ActionMapping.bAlt = Modifiers->GetBoolField(TEXT("alt"));
        }
        if (Modifiers->HasField(TEXT("cmd")))
        {
            ActionMapping.bCmd = Modifiers->GetBoolField(TEXT("cmd"));
        }
    }

    // Add the mapping
    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    return true;
}

// ============================================
// Folder Operations
// ============================================

bool FProjectService::CreateFolder(const FString& FolderPath, bool& bOutAlreadyExists, FString& OutError)
{
    bOutAlreadyExists = false;

    // Get the base project directory
    FString ProjectPath = FPaths::ProjectDir();

    // Check if this is a content folder request
    bool bIsContentFolder = FolderPath.StartsWith(TEXT("/Content/")) || FolderPath.StartsWith(TEXT("Content/"));
    if (bIsContentFolder)
    {
        // Use UE's asset system for content folders
        FString AssetPath = FolderPath;
        if (!AssetPath.StartsWith(TEXT("/Game/")))
        {
            // Convert Content/ to /Game/ for asset paths
            AssetPath = AssetPath.Replace(TEXT("/Content/"), TEXT("/Game/"));
            AssetPath = AssetPath.Replace(TEXT("Content/"), TEXT("/Game/"));
        }

        // Check if the directory already exists
        if (UEditorAssetLibrary::DoesDirectoryExist(AssetPath))
        {
            bOutAlreadyExists = true;
            return true;
        }

        if (!UEditorAssetLibrary::MakeDirectory(AssetPath))
        {
            OutError = FString::Printf(TEXT("Failed to create content folder: %s"), *AssetPath);
            return false;
        }
    }
    else
    {
        // For non-content folders, use platform file system
        // Clean up the folder path to avoid double slashes
        FString CleanFolderPath = FolderPath;
        if (CleanFolderPath.StartsWith(TEXT("/")))
        {
            CleanFolderPath = CleanFolderPath.RightChop(1);
        }

        FString FullPath = FPaths::Combine(ProjectPath, CleanFolderPath);

        // Check if directory already exists
        if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FullPath))
        {
            bOutAlreadyExists = true;
            return true;
        }

        if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FullPath))
        {
            OutError = FString::Printf(TEXT("Failed to create folder: %s"), *FullPath);
            return false;
        }
    }

    return true;
}

TArray<FString> FProjectService::ListFolderContents(const FString& FolderPath, bool& bOutSuccess, FString& OutError)
{
    TArray<FString> Contents;
    bOutSuccess = false;

    // Check if this is a content folder request
    bool bIsContentFolder = FolderPath.StartsWith(TEXT("/Game")) || FolderPath.StartsWith(TEXT("/Content/")) || FolderPath.StartsWith(TEXT("Content/"));

    if (bIsContentFolder)
    {
        // Use UE's asset system for content folders
        FString AssetPath = FolderPath;
        if (AssetPath.StartsWith(TEXT("/Content/")))
        {
            AssetPath = AssetPath.Replace(TEXT("/Content/"), TEXT("/Game/"));
        }
        else if (AssetPath.StartsWith(TEXT("Content/")))
        {
            AssetPath = AssetPath.Replace(TEXT("Content/"), TEXT("/Game/"));
        }

        // Get subdirectories and assets using AssetRegistry
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FString> SubPaths;
        AssetRegistry.GetSubPaths(AssetPath, SubPaths, false);

        for (const FString& SubPath : SubPaths)
        {
            Contents.Add(FString::Printf(TEXT("FOLDER: %s"), *SubPath));
        }

        // Get assets (UE 5.7 compatible)
        TArray<FString> Assets = UEditorAssetLibrary::ListAssets(AssetPath, false, false);
        for (const FString& Asset : Assets)
        {
            Contents.Add(FString::Printf(TEXT("ASSET: %s"), *Asset));
        }

        // If no content was found at all, the path likely doesn't exist
        if (Contents.Num() == 0)
        {
            // Double-check with recursive search - maybe there are only nested assets
            TArray<FString> RecursiveAssets = UEditorAssetLibrary::ListAssets(AssetPath, true, false);
            if (RecursiveAssets.Num() == 0)
            {
                OutError = FString::Printf(TEXT("Content directory does not exist or is empty: %s"), *AssetPath);
                return Contents;
            }
            // If we found recursive assets, the folder exists but only has nested content
            SubPaths.Empty();
            AssetRegistry.GetSubPaths(AssetPath, SubPaths, false);
            for (const FString& SubPath : SubPaths)
            {
                Contents.Add(FString::Printf(TEXT("FOLDER: %s"), *SubPath));
            }
        }
    }
    else
    {
        // For non-content folders, use platform file system
        FString ProjectPath = FPaths::ProjectDir();

        // Clean up the folder path to avoid double slashes
        FString CleanFolderPath = FolderPath;
        if (CleanFolderPath.StartsWith(TEXT("/")))
        {
            CleanFolderPath = CleanFolderPath.RightChop(1);
        }

        FString FullPath = FPaths::Combine(ProjectPath, CleanFolderPath);

        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FullPath))
        {
            OutError = FString::Printf(TEXT("Directory does not exist: %s"), *FullPath);
            return Contents;
        }

        // List directory contents
        TArray<FString> FoundFiles;

        FPlatformFileManager::Get().GetPlatformFile().FindFiles(FoundFiles, *FullPath, TEXT("*"));

        // Use IterateDirectory to find subdirectories (UE 5.7 compatible)
        FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*FullPath, [&Contents](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
        {
            if (bIsDirectory)
            {
                Contents.Add(FString::Printf(TEXT("DIR: %s"), *FPaths::GetCleanFilename(FilenameOrDirectory)));
            }
            return true; // Continue iteration
        });

        for (const FString& File : FoundFiles)
        {
            Contents.Add(FString::Printf(TEXT("FILE: %s"), *FPaths::GetCleanFilename(File)));
        }
    }

    bOutSuccess = true;
    return Contents;
}

FString FProjectService::GetProjectDirectory() const
{
    return FPaths::ProjectDir();
}

// ============================================
// Struct Operations - Delegate to ProjectStructService
// ============================================

bool FProjectService::CreateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutFullPath, FString& OutError)
{
    return FProjectStructService::Get().CreateStruct(StructName, Path, Description, Properties, OutFullPath, OutError);
}

bool FProjectService::UpdateStruct(const FString& StructName, const FString& Path, const FString& Description, const TArray<TSharedPtr<FJsonObject>>& Properties, FString& OutError)
{
    return FProjectStructService::Get().UpdateStruct(StructName, Path, Description, Properties, OutError);
}

TArray<TSharedPtr<FJsonObject>> FProjectService::ShowStructVariables(const FString& StructName, const FString& Path, bool& bOutSuccess, FString& OutError)
{
    return FProjectStructService::Get().ShowStructVariables(StructName, Path, bOutSuccess, OutError);
}

bool FProjectService::CreateStructProperty(UUserDefinedStruct* Struct, const TSharedPtr<FJsonObject>& PropertyObj) const
{
    // This private helper is no longer needed since ProjectStructService handles it internally
    // Keep for backwards compatibility but delegate to the service
    FProjectStructService& Service = FProjectStructService::Get();
    // Note: Since CreateStructProperty is private in ProjectStructService, we can't delegate directly
    // This method shouldn't be called externally - the service handles it internally
    return false;
}

// ============================================
// Enum Operations - Delegate to ProjectEnumService
// ============================================

bool FProjectService::CreateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutFullPath, FString& OutError)
{
    return FProjectEnumService::Get().CreateEnum(EnumName, Path, Description, Values, ValueDescriptions, OutFullPath, OutError);
}

bool FProjectService::UpdateEnum(const FString& EnumName, const FString& Path, const FString& Description, const TArray<FString>& Values, const TMap<FString, FString>& ValueDescriptions, FString& OutError)
{
    return FProjectEnumService::Get().UpdateEnum(EnumName, Path, Description, Values, ValueDescriptions, OutError);
}

// ============================================
// Enhanced Input Operations - Placeholders
// ============================================

bool FProjectService::CreateEnhancedInputAction(const FString& ActionName, const FString& Path, const FString& Description, const FString& ValueType, FString& OutAssetPath, FString& OutError)
{
    // Enhanced Input Action creation is handled by the legacy command system
    // This service method is a placeholder for future refactoring
    OutError = TEXT("Enhanced Input Action creation is handled by legacy commands - use create_enhanced_input_action command");
    return false;
}

bool FProjectService::CreateInputMappingContext(const FString& ContextName, const FString& Path, const FString& Description, FString& OutAssetPath, FString& OutError)
{
    // Input Mapping Context creation is handled by the legacy command system
    // This service method is a placeholder for future refactoring
    OutError = TEXT("Input Mapping Context creation is handled by legacy commands - use create_input_mapping_context command");
    return false;
}

bool FProjectService::AddMappingToContext(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TSharedPtr<FJsonObject>& Modifiers, FString& OutError)
{
    // Mapping addition to context is handled by the legacy command system
    // This service method is a placeholder for future refactoring
    OutError = TEXT("Add mapping to context is handled by legacy commands - use add_mapping_to_context command");
    return false;
}

TArray<TSharedPtr<FJsonObject>> FProjectService::ListInputActions(const FString& Path, bool& bOutSuccess, FString& OutError)
{
    TArray<TSharedPtr<FJsonObject>> Actions;
    bOutSuccess = false;
    OutError = TEXT("List input actions not yet implemented in service layer");
    return Actions;
}

TArray<TSharedPtr<FJsonObject>> FProjectService::ListInputMappingContexts(const FString& Path, bool& bOutSuccess, FString& OutError)
{
    TArray<TSharedPtr<FJsonObject>> Contexts;
    bOutSuccess = false;
    OutError = TEXT("List input mapping contexts not yet implemented in service layer");
    return Contexts;
}

// ============================================
// Asset Operations - Delegate to ProjectAssetOperations
// ============================================

bool FProjectService::DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError)
{
    return FProjectAssetOperations::Get().DuplicateAsset(SourcePath, DestinationPath, NewName, OutNewAssetPath, OutError);
}

bool FProjectService::DeleteAsset(const FString& AssetPath, FString& OutError)
{
    return FProjectAssetOperations::Get().DeleteAsset(AssetPath, OutError);
}

bool FProjectService::RenameAsset(const FString& AssetPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError)
{
    return FProjectAssetOperations::Get().RenameAsset(AssetPath, NewName, OutNewAssetPath, OutError);
}

bool FProjectService::MoveAsset(const FString& AssetPath, const FString& DestinationFolder, FString& OutNewAssetPath, FString& OutError)
{
    return FProjectAssetOperations::Get().MoveAsset(AssetPath, DestinationFolder, OutNewAssetPath, OutError);
}

TArray<TSharedPtr<FJsonObject>> FProjectService::SearchAssets(const FString& Pattern, const FString& AssetClass, const FString& Folder, bool& bOutSuccess, FString& OutError)
{
    return FProjectAssetOperations::Get().SearchAssets(Pattern, AssetClass, Folder, bOutSuccess, OutError);
}

// ============================================
// Font Operations - Delegate to ProjectFontService
// ============================================

bool FProjectService::CreateFontFace(const FString& FontName, const FString& Path, const FString& SourceTexturePath, bool bUseSDF, int32 DistanceFieldSpread, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError)
{
    return FProjectFontService::Get().CreateFontFace(FontName, Path, SourceTexturePath, bUseSDF, DistanceFieldSpread, FontMetrics, OutAssetPath, OutError);
}

bool FProjectService::ImportTTFFont(const FString& FontName, const FString& Path, const FString& TTFFilePath, const TSharedPtr<FJsonObject>& FontMetrics, FString& OutAssetPath, FString& OutError)
{
    return FProjectFontService::Get().ImportTTFFont(FontName, Path, TTFFilePath, FontMetrics, OutAssetPath, OutError);
}

bool FProjectService::SetFontFaceProperties(const FString& FontPath, const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, TArray<FString>& OutFailedProperties, FString& OutError)
{
    return FProjectFontService::Get().SetFontFaceProperties(FontPath, Properties, OutSuccessProperties, OutFailedProperties, OutError);
}

TSharedPtr<FJsonObject> FProjectService::GetFontFaceMetadata(const FString& FontPath, FString& OutError)
{
    return FProjectFontService::Get().GetFontFaceMetadata(FontPath, OutError);
}

bool FProjectService::CreateOfflineFont(const FString& FontName, const FString& Path, const FString& TexturePath, const FString& MetricsFilePath, FString& OutAssetPath, FString& OutError)
{
    return FProjectFontService::Get().CreateOfflineFont(FontName, Path, TexturePath, MetricsFilePath, OutAssetPath, OutError);
}

TSharedPtr<FJsonObject> FProjectService::GetFontMetadata(const FString& FontPath, FString& OutError)
{
    return FProjectFontService::Get().GetFontMetadata(FontPath, OutError);
}

// ============================================
// DataAsset Operations - Delegate to ProjectDataAssetService
// ============================================

bool FProjectService::CreateDataAsset(const FString& Name, const FString& AssetClass, const FString& FolderPath, const TSharedPtr<FJsonObject>& Properties, FString& OutAssetPath, FString& OutError)
{
    return FProjectDataAssetService::Get().CreateDataAsset(Name, AssetClass, FolderPath, Properties, OutAssetPath, OutError);
}

bool FProjectService::SetDataAssetProperty(const FString& AssetPath, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError)
{
    return FProjectDataAssetService::Get().SetDataAssetProperty(AssetPath, PropertyName, PropertyValue, OutError);
}

TSharedPtr<FJsonObject> FProjectService::GetDataAssetMetadata(const FString& AssetPath, FString& OutError)
{
    return FProjectDataAssetService::Get().GetDataAssetMetadata(AssetPath, OutError);
}
