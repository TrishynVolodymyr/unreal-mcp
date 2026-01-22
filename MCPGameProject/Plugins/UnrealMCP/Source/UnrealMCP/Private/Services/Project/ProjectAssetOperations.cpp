#include "Services/Project/ProjectAssetOperations.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

FProjectAssetOperations& FProjectAssetOperations::Get()
{
    static FProjectAssetOperations Instance;
    return Instance;
}

bool FProjectAssetOperations::DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError)
{
    // Validate source asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
    {
        OutError = FString::Printf(TEXT("Source asset does not exist: %s"), *SourcePath);
        return false;
    }

    // Ensure destination path exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationPath))
    {
        if (!UEditorAssetLibrary::MakeDirectory(DestinationPath))
        {
            OutError = FString::Printf(TEXT("Failed to create destination directory: %s"), *DestinationPath);
            return false;
        }
    }

    // Build the full destination path
    FString CleanDestPath = DestinationPath;
    if (!CleanDestPath.EndsWith(TEXT("/")))
    {
        CleanDestPath += TEXT("/");
    }
    FString FullDestinationPath = CleanDestPath + NewName;

    // Check if destination already exists
    if (UEditorAssetLibrary::DoesAssetExist(FullDestinationPath))
    {
        OutError = FString::Printf(TEXT("Destination asset already exists: %s"), *FullDestinationPath);
        return false;
    }

    // Use UEditorAssetLibrary::DuplicateAsset - this is the simplest and most reliable method
    // It handles all asset types automatically (Blueprints, Widgets, DataTables, Materials, etc.)
    if (!UEditorAssetLibrary::DuplicateAsset(SourcePath, FullDestinationPath))
    {
        OutError = FString::Printf(TEXT("Failed to duplicate asset from '%s' to '%s'"), *SourcePath, *FullDestinationPath);
        return false;
    }

    OutNewAssetPath = FullDestinationPath;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully duplicated asset from '%s' to '%s'"), *SourcePath, *FullDestinationPath);

    return true;
}

bool FProjectAssetOperations::DeleteAsset(const FString& AssetPath, FString& OutError)
{
    // Validate asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        OutError = FString::Printf(TEXT("Asset does not exist: %s"), *AssetPath);
        return false;
    }

    // Use UEditorAssetLibrary::DeleteAsset to remove the asset
    if (!UEditorAssetLibrary::DeleteAsset(AssetPath))
    {
        OutError = FString::Printf(TEXT("Failed to delete asset: %s"), *AssetPath);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully deleted asset: %s"), *AssetPath);
    return true;
}

bool FProjectAssetOperations::RenameAsset(const FString& AssetPath, const FString& NewName, FString& OutNewAssetPath, FString& OutError)
{
    // Validate inputs
    if (AssetPath.IsEmpty())
    {
        OutError = TEXT("Asset path cannot be empty");
        return false;
    }
    if (NewName.IsEmpty())
    {
        OutError = TEXT("New name cannot be empty");
        return false;
    }

    // Normalize the path
    FString NormalizedPath = AssetPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(AssetPath);
        NormalizedPath = AssetPath + TEXT(".") + AssetName;
    }

    // Check if asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(NormalizedPath))
    {
        OutError = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
        return false;
    }

    // Get the directory path
    FString Directory = FPaths::GetPath(AssetPath);
    FString NewPath = Directory / NewName;

    // Use UEditorAssetLibrary to rename (it handles all the bookkeeping)
    if (!UEditorAssetLibrary::RenameAsset(NormalizedPath, NewPath))
    {
        OutError = FString::Printf(TEXT("Failed to rename asset from %s to %s"), *AssetPath, *NewPath);
        return false;
    }

    OutNewAssetPath = NewPath;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully renamed asset from '%s' to '%s'"), *AssetPath, *NewPath);
    return true;
}

bool FProjectAssetOperations::MoveAsset(const FString& AssetPath, const FString& DestinationFolder, FString& OutNewAssetPath, FString& OutError)
{
    // Validate inputs
    if (AssetPath.IsEmpty())
    {
        OutError = TEXT("Asset path cannot be empty");
        return false;
    }
    if (DestinationFolder.IsEmpty())
    {
        OutError = TEXT("Destination folder cannot be empty");
        return false;
    }

    // Normalize the source path
    FString NormalizedPath = AssetPath;
    if (!NormalizedPath.Contains(TEXT(".")))
    {
        FString AssetName = FPaths::GetBaseFilename(AssetPath);
        NormalizedPath = AssetPath + TEXT(".") + AssetName;
    }

    // Check if asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(NormalizedPath))
    {
        OutError = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
        return false;
    }

    // Get the asset name
    FString AssetName = FPaths::GetBaseFilename(AssetPath);
    FString NewPath = DestinationFolder / AssetName;

    // Ensure destination folder exists
    if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationFolder))
    {
        if (!UEditorAssetLibrary::MakeDirectory(DestinationFolder))
        {
            OutError = FString::Printf(TEXT("Failed to create destination folder: %s"), *DestinationFolder);
            return false;
        }
    }

    // Use UEditorAssetLibrary to rename (move is just rename to different directory)
    if (!UEditorAssetLibrary::RenameAsset(NormalizedPath, NewPath))
    {
        OutError = FString::Printf(TEXT("Failed to move asset from %s to %s"), *AssetPath, *NewPath);
        return false;
    }

    OutNewAssetPath = NewPath;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Successfully moved asset from '%s' to '%s'"), *AssetPath, *NewPath);
    return true;
}

TArray<TSharedPtr<FJsonObject>> FProjectAssetOperations::SearchAssets(const FString& Pattern, const FString& AssetClass, const FString& Folder, bool& bOutSuccess, FString& OutError)
{
    TArray<TSharedPtr<FJsonObject>> Results;
    bOutSuccess = false;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Build filter
    FARFilter Filter;

    // Set search path
    FString SearchPath = Folder.IsEmpty() ? TEXT("/Game") : Folder;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;

    // Filter by class if specified
    if (!AssetClass.IsEmpty())
    {
        // Try to find the class using UE5's FindFirstObject (replaces deprecated ANY_PACKAGE)
        UClass* Class = FindFirstObject<UClass>(*AssetClass, EFindFirstObjectOptions::ExactClass);
        if (!Class)
        {
            // Try with common prefixes
            Class = FindFirstObject<UClass>(*(TEXT("U") + AssetClass), EFindFirstObjectOptions::ExactClass);
        }
        if (!Class)
        {
            Class = FindFirstObject<UClass>(*(TEXT("A") + AssetClass), EFindFirstObjectOptions::ExactClass);
        }
        if (Class)
        {
            Filter.ClassPaths.Add(Class->GetClassPathName());
            Filter.bRecursiveClasses = true;
        }
    }

    // Get assets
    TArray<FAssetData> AssetList;
    AssetRegistry.GetAssets(Filter, AssetList);

    // Filter by pattern if specified
    FString SearchPattern = Pattern;
    bool bHasPattern = !SearchPattern.IsEmpty();

    // Convert wildcard pattern to regex-like matching
    if (bHasPattern)
    {
        SearchPattern = SearchPattern.Replace(TEXT("*"), TEXT(""));
    }

    for (const FAssetData& AssetData : AssetList)
    {
        FString AssetName = AssetData.AssetName.ToString();

        // Apply pattern filter
        if (bHasPattern && !AssetName.Contains(SearchPattern, ESearchCase::IgnoreCase))
        {
            continue;
        }

        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), AssetName);
        AssetObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        AssetObj->SetStringField(TEXT("package_path"), AssetData.PackagePath.ToString());
        AssetObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());

        Results.Add(AssetObj);
    }

    bOutSuccess = true;
    UE_LOG(LogTemp, Display, TEXT("MCP Project: Found %d assets matching pattern '%s'"), Results.Num(), *Pattern);
    return Results;
}
