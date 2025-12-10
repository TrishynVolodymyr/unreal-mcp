#include "Utils/AssetUtils.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"

// Helper method implementations
TArray<FString> FAssetUtils::FindAssetsByType(const FString& AssetType, const FString& SearchPath)
{
    TArray<FString> FoundAssets;

    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Create filter for asset type
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(*AssetType));

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        FoundAssets.Add(AssetData.GetSoftObjectPath().ToString());
    }

    UE_LOG(LogTemp, Display, TEXT("Found %d assets of type '%s' in path '%s'"), FoundAssets.Num(), *AssetType, *SearchPath);
    return FoundAssets;
}

TArray<FString> FAssetUtils::FindAssetsByName(const FString& AssetName, const FString& SearchPath)
{
    TArray<FString> FoundAssets;

    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Create filter for search path
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetBaseName = FPaths::GetBaseFilename(AssetData.AssetName.ToString());
        if (AssetBaseName.Contains(AssetName, ESearchCase::IgnoreCase) ||
            AssetData.AssetName.ToString().Contains(AssetName, ESearchCase::IgnoreCase))
        {
            FoundAssets.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Found %d assets matching name '%s' in path '%s'"), FoundAssets.Num(), *AssetName, *SearchPath);
    return FoundAssets;
}

TArray<FString> FAssetUtils::FindWidgetBlueprints(const FString& WidgetName, const FString& SearchPath)
{
    TArray<FString> FoundWidgets;

    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Create filter for Widget Blueprints
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/UMGEditor"), TEXT("WidgetBlueprint")));

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (WidgetName.IsEmpty() ||
            AssetData.AssetName.ToString().Contains(WidgetName, ESearchCase::IgnoreCase))
        {
            FoundWidgets.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Found %d widget blueprints matching '%s' in path '%s'"), FoundWidgets.Num(), *WidgetName, *SearchPath);
    return FoundWidgets;
}

FString FAssetUtils::BuildEnginePath(const FString& Path)
{
    return FString::Printf(TEXT("/Script/Engine.%s"), *Path);
}

FString FAssetUtils::BuildCorePath(const FString& Path)
{
    return FString::Printf(TEXT("/Script/CoreUObject.%s"), *Path);
}

// Public method implementations
UClass* FAssetUtils::FindWidgetClass(const FString& WidgetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Searching for widget class: %s"), *WidgetPath);

    // Strategy 1: Direct class loading if path looks like a class path
    if (WidgetPath.Contains(TEXT("_C")) || WidgetPath.StartsWith(TEXT("/Script/")))
    {
        UClass* DirectClass = LoadObject<UClass>(nullptr, *WidgetPath);
        if (DirectClass && DirectClass->IsChildOf(UUserWidget::StaticClass()))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found class via direct loading: %s"), *DirectClass->GetName());
            return DirectClass;
        }
    }

    // Strategy 2: Asset-based loading
    UBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetPath);
    if (WidgetBlueprint && WidgetBlueprint->GeneratedClass)
    {
        UClass* GeneratedClass = WidgetBlueprint->GeneratedClass;
        if (GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found class via blueprint: %s"), *GeneratedClass->GetName());
            return GeneratedClass;
        }
    }

    // Strategy 3: Search using asset discovery
    TArray<FString> CommonPaths = GetCommonAssetSearchPaths(WidgetPath);
    for (const FString& SearchPath : CommonPaths)
    {
        UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Trying search path: %s"), *SearchPath);

        // Try loading as blueprint asset first
        if (UEditorAssetLibrary::DoesAssetExist(SearchPath))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(SearchPath);
            if (UBlueprint* BP = Cast<UBlueprint>(Asset))
            {
                if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
                {
                    UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found widget class via asset search: %s"), *BP->GeneratedClass->GetName());
                    return BP->GeneratedClass;
                }
            }
        }

        // Try loading as class with _C suffix
        FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *SearchPath, *FPaths::GetBaseFilename(SearchPath));
        UClass* Class = LoadObject<UClass>(nullptr, *ClassPath);
        if (Class && Class->IsChildOf(UUserWidget::StaticClass()))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetClass: Found widget class via class path: %s"), *Class->GetName());
            return Class;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindWidgetClass: Could not find widget class for: %s"), *WidgetPath);
    return nullptr;
}

UBlueprint* FAssetUtils::FindWidgetBlueprint(const FString& WidgetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Searching for widget blueprint: %s"), *WidgetPath);

    // Strategy 1: Direct asset loading
    if (UEditorAssetLibrary::DoesAssetExist(WidgetPath))
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(WidgetPath);
        if (UBlueprint* BP = Cast<UBlueprint>(Asset))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found blueprint via direct loading: %s"), *BP->GetName());
            return BP;
        }
    }

    // Strategy 2: Search using common paths
    TArray<FString> CommonPaths = GetCommonAssetSearchPaths(WidgetPath);
    for (const FString& SearchPath : CommonPaths)
    {
        UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Trying search path: %s"), *SearchPath);

        if (UEditorAssetLibrary::DoesAssetExist(SearchPath))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(SearchPath);
            if (UBlueprint* BP = Cast<UBlueprint>(Asset))
            {
                UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found blueprint via asset search: %s"), *BP->GetName());
                return BP;
            }
        }
    }

    // Strategy 3: Use asset registry search
    TArray<FString> FoundWidgets = FindWidgetBlueprints(FPaths::GetBaseFilename(WidgetPath));
    for (const FString& FoundPath : FoundWidgets)
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(FoundPath);
        if (UBlueprint* BP = Cast<UBlueprint>(Asset))
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found blueprint via registry search: %s"), *BP->GetName());
            return BP;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindWidgetBlueprint: Could not find widget blueprint for: %s"), *WidgetPath);
    return nullptr;
}

UObject* FAssetUtils::FindAssetByPath(const FString& AssetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindAssetByPath: Searching for asset: %s"), *AssetPath);

    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
        if (Asset)
        {
            UE_LOG(LogTemp, Display, TEXT("FindAssetByPath: Found asset: %s"), *Asset->GetName());
            return Asset;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindAssetByPath: Could not find asset: %s"), *AssetPath);
    return nullptr;
}

UObject* FAssetUtils::FindAssetByName(const FString& AssetName, const FString& AssetType)
{
    UE_LOG(LogTemp, Display, TEXT("FindAssetByName: Searching for asset '%s' of type '%s'"), *AssetName, *AssetType);

    TArray<FString> FoundAssets;
    if (!AssetType.IsEmpty())
    {
        FoundAssets = FindAssetsByType(AssetType);
    }
    else
    {
        FoundAssets = FindAssetsByName(AssetName);
    }

    for (const FString& AssetPath : FoundAssets)
    {
        if (FPaths::GetBaseFilename(AssetPath).Contains(AssetName, ESearchCase::IgnoreCase))
        {
            UObject* Asset = FindAssetByPath(AssetPath);
            if (Asset)
            {
                UE_LOG(LogTemp, Display, TEXT("FindAssetByName: Found matching asset: %s"), *Asset->GetName());
                return Asset;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindAssetByName: Could not find asset '%s'"), *AssetName);
    return nullptr;
}

UScriptStruct* FAssetUtils::FindStructType(const FString& StructPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindStructType: Searching for struct: %s"), *StructPath);

    // Strategy 1: Check built-in struct types first
    static const TMap<FString, UScriptStruct*> BuiltInStructs = {
        {TEXT("Vector"), TBaseStructure<FVector>::Get()},
        {TEXT("Rotator"), TBaseStructure<FRotator>::Get()},
        {TEXT("Transform"), TBaseStructure<FTransform>::Get()},
        {TEXT("Color"), TBaseStructure<FLinearColor>::Get()},
        {TEXT("LinearColor"), TBaseStructure<FLinearColor>::Get()},
        {TEXT("Vector2D"), TBaseStructure<FVector2D>::Get()},
        {TEXT("IntPoint"), TBaseStructure<FIntPoint>::Get()},
        {TEXT("IntVector"), TBaseStructure<FIntVector>::Get()},
        {TEXT("Guid"), TBaseStructure<FGuid>::Get()},
        {TEXT("DateTime"), TBaseStructure<FDateTime>::Get()}
    };

    // Check for exact built-in match
    if (const UScriptStruct* const* FoundBuiltIn = BuiltInStructs.Find(StructPath))
    {
        UE_LOG(LogTemp, Display, TEXT("FindStructType: Found built-in struct: %s"), *(*FoundBuiltIn)->GetName());
        return const_cast<UScriptStruct*>(*FoundBuiltIn);
    }

    // Strategy 2: Try direct struct loading with various naming conventions
    TArray<FString> StructNameVariations;
    StructNameVariations.Add(StructPath);
    StructNameVariations.Add(FString::Printf(TEXT("F%s"), *StructPath));

    // If it's already a path, try loading directly
    if (StructPath.StartsWith(TEXT("/")) || StructPath.Contains(TEXT(".")))
    {
        UScriptStruct* DirectStruct = LoadObject<UScriptStruct>(nullptr, *StructPath);
        if (DirectStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via direct path: %s"), *DirectStruct->GetName());
            return DirectStruct;
        }
    }

    // Strategy 3: Search in common struct directories
    TArray<FString> StructDirectories = {
        TEXT("/Game/DataStructures/"),
        TEXT("/Game/Data/"),
        TEXT("/Game/Blueprints/DataStructures/"),
        TEXT("/Game/Blueprints/Structs/"),
        TEXT("/Game/Blueprints/"),
        TEXT("/Game/Structs/"),
        TEXT("/Game/")
    };

    for (const FString& StructDir : StructDirectories)
    {
        for (const FString& StructVariation : StructNameVariations)
        {
            // Try as asset path (e.g., /Game/DataStructures/MyStruct.MyStruct)
            FString AssetPath = FString::Printf(TEXT("%s%s.%s"), *StructDir, *StructVariation, *StructVariation);
            UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *AssetPath);
            if (FoundStruct)
            {
                UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via asset search: %s"), *FoundStruct->GetName());
                return FoundStruct;
            }

            // Try with base filename only
            FString BaseFilename = FPaths::GetBaseFilename(StructPath);
            if (BaseFilename != StructPath)
            {
                AssetPath = FString::Printf(TEXT("%s%s.%s"), *StructDir, *BaseFilename, *BaseFilename);
                FoundStruct = LoadObject<UScriptStruct>(nullptr, *AssetPath);
                if (FoundStruct)
                {
                    UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via base filename search: %s"), *FoundStruct->GetName());
                    return FoundStruct;
                }
            }
        }
    }

    // Strategy 4: Try engine paths for built-in structs
    for (const FString& StructVariation : StructNameVariations)
    {
        FString EnginePath = BuildEnginePath(StructVariation);
        UScriptStruct* EngineStruct = LoadObject<UScriptStruct>(nullptr, *EnginePath);
        if (EngineStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via engine path: %s"), *EngineStruct->GetName());
            return EngineStruct;
        }

        FString CorePath = BuildCorePath(StructVariation);
        UScriptStruct* CoreStruct = LoadObject<UScriptStruct>(nullptr, *CorePath);
        if (CoreStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via core path: %s"), *CoreStruct->GetName());
            return CoreStruct;
        }
    }

    // Strategy 5: Use asset registry to find user-defined structs
    TArray<FString> FoundStructs = FindAssetsByType(TEXT("UserDefinedStruct"));
    for (const FString& FoundPath : FoundStructs)
    {
        if (FPaths::GetBaseFilename(FoundPath).Contains(StructPath, ESearchCase::IgnoreCase) ||
            FPaths::GetBaseFilename(FoundPath).Contains(FString::Printf(TEXT("F%s"), *StructPath), ESearchCase::IgnoreCase))
        {
            UScriptStruct* Struct = Cast<UScriptStruct>(FindAssetByPath(FoundPath));
            if (Struct)
            {
                UE_LOG(LogTemp, Display, TEXT("FindStructType: Found struct via registry search: %s"), *Struct->GetName());
                return Struct;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindStructType: Could not find struct for: %s"), *StructPath);
    return nullptr;
}

TArray<FString> FAssetUtils::GetCommonAssetSearchPaths(const FString& AssetName)
{
    TArray<FString> SearchPaths;

    // If AssetName is already a full path (starts with /Game/ or /Script/), just return it
    if (AssetName.StartsWith(TEXT("/Game/")) || AssetName.StartsWith(TEXT("/Script/")))
    {
        SearchPaths.Add(AssetName);
        return SearchPaths;
    }

    // Clean the asset name
    FString CleanName = NormalizeAssetPath(AssetName);

    // Remove common prefixes and suffixes for search
    if (CleanName.StartsWith(TEXT("WBP_")))
    {
        CleanName = CleanName.RightChop(4);
    }
    if (CleanName.StartsWith(TEXT("BP_")))
    {
        CleanName = CleanName.RightChop(3);
    }

    // Common widget directories
    TArray<FString> CommonDirs = {
        TEXT("/Game/Widgets/"),
        TEXT("/Game/UI/"),
        TEXT("/Game/UMG/"),
        TEXT("/Game/Blueprints/Widgets/"),
        TEXT("/Game/Blueprints/UI/"),
        TEXT("/Game/Blueprints/"),
        TEXT("/Game/")
    };

    // Build search paths
    for (const FString& Dir : CommonDirs)
    {
        // Original name
        SearchPaths.Add(Dir + AssetName);
        SearchPaths.Add(Dir + CleanName);

        // With WBP_ prefix
        if (!AssetName.StartsWith(TEXT("WBP_")))
        {
            SearchPaths.Add(Dir + TEXT("WBP_") + AssetName);
            SearchPaths.Add(Dir + TEXT("WBP_") + CleanName);
        }

        // With BP_ prefix
        if (!AssetName.StartsWith(TEXT("BP_")))
        {
            SearchPaths.Add(Dir + TEXT("BP_") + AssetName);
            SearchPaths.Add(Dir + TEXT("BP_") + CleanName);
        }
    }

    return SearchPaths;
}

FString FAssetUtils::NormalizeAssetPath(const FString& AssetPath)
{
    FString CleanPath = AssetPath;
    CleanPath.TrimStartAndEndInline();

    // Remove leading slashes and Game/ prefix for normalization
    if (CleanPath.StartsWith(TEXT("/")))
    {
        CleanPath.RemoveFromStart(TEXT("/"));
    }
    if (CleanPath.StartsWith(TEXT("Game/")))
    {
        CleanPath.RemoveFromStart(TEXT("Game/"));
    }

    // Get just the filename if it's a full path
    if (CleanPath.Contains(TEXT("/")))
    {
        CleanPath = FPaths::GetBaseFilename(CleanPath);
    }

    return CleanPath;
}

bool FAssetUtils::IsValidAssetPath(const FString& AssetPath)
{
    return UEditorAssetLibrary::DoesAssetExist(AssetPath);
}
