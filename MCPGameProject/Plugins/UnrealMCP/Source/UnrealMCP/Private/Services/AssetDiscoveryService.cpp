#include "Services/AssetDiscoveryService.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"


FAssetDiscoveryService& FAssetDiscoveryService::Get()
{
    static FAssetDiscoveryService Instance;
    return Instance;
}

TArray<FString> FAssetDiscoveryService::FindAssetsByType(const FString& AssetType, const FString& SearchPath)
{
    TArray<FString> FoundAssets;
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    
    // Add class names based on asset type
    if (AssetType.Equals(TEXT("Blueprint"), ESearchCase::IgnoreCase))
    {
        Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    }
    else if (AssetType.Equals(TEXT("WidgetBlueprint"), ESearchCase::IgnoreCase))
    {
        Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    }
    
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        FoundAssets.Add(AssetData.GetObjectPathString());
    }
    
    return FoundAssets;
}

TArray<FString> FAssetDiscoveryService::FindAssetsByName(const FString& AssetName, const FString& SearchPath)
{
    TArray<FString> FoundAssets;
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString().Contains(AssetName))
        {
            FoundAssets.Add(AssetData.GetObjectPathString());
        }
    }
    
    return FoundAssets;
}

TArray<FString> FAssetDiscoveryService::FindWidgetBlueprints(const FString& WidgetName, const FString& SearchPath)
{
    TArray<FString> FoundWidgets;
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    
    FARFilter Filter;
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (WidgetName.IsEmpty() || AssetData.AssetName.ToString().Contains(WidgetName))
        {
            FoundWidgets.Add(AssetData.GetObjectPathString());
        }
    }
    
    return FoundWidgets;
}

TArray<FString> FAssetDiscoveryService::FindBlueprints(const FString& BlueprintName, const FString& SearchPath)
{
    TArray<FString> FoundBlueprints;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    // Include both regular Blueprints and Widget Blueprints
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;

    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (BlueprintName.IsEmpty() || AssetData.AssetName.ToString().Contains(BlueprintName))
        {
            FoundBlueprints.Add(AssetData.GetObjectPathString());
        }
    }

    return FoundBlueprints;
}

TArray<FString> FAssetDiscoveryService::FindDataTables(const FString& TableName, const FString& SearchPath)
{
    TArray<FString> FoundTables;
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    
    FARFilter Filter;
    Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (TableName.IsEmpty() || AssetData.AssetName.ToString().Contains(TableName))
        {
            FoundTables.Add(AssetData.GetObjectPathString());
        }
    }
    
    return FoundTables;
}

UClass* FAssetDiscoveryService::FindWidgetClass(const FString& WidgetPath)
{
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Searching for widget class: %s"), *WidgetPath);
    
    // First try direct loading
    UClass* FoundClass = LoadObject<UClass>(nullptr, *WidgetPath);
    if (FoundClass)
    {
        UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found class via direct loading: %s"), *FoundClass->GetName());
        return FoundClass;
    }
    
    // Try loading as widget blueprint
    UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(WidgetPath);
    if (WidgetBP && WidgetBP->GeneratedClass)
    {
        UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found class via widget blueprint: %s"), *WidgetBP->GeneratedClass->GetName());
        return WidgetBP->GeneratedClass;
    }
    
    // Try with common UMG class names
    UClass* UMGClass = ResolveUMGClass(WidgetPath);
    if (UMGClass)
    {
        return UMGClass;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find widget class for: %s"), *WidgetPath);
    return nullptr;
}

UWidgetBlueprint* FAssetDiscoveryService::FindWidgetBlueprint(const FString& WidgetPath)
{
    UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Searching for widget blueprint: %s"), *WidgetPath);

    // Try direct loading first (works when not in PIE)
    UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
    if (WidgetBP)
    {
        UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found via direct loading: %s"), *WidgetBP->GetName());
        return WidgetBP;
    }

    // Try with common paths
    TArray<FString> SearchPaths = GetCommonAssetSearchPaths(WidgetPath);

    for (const FString& SearchPath : SearchPaths)
    {
        UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Trying search path: %s"), *SearchPath);
        WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *SearchPath);
        if (WidgetBP)
        {
            UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found via search path: %s"), *SearchPath);
            return WidgetBP;
        }
    }

    // Use asset registry as fallback - this works during PIE because we use GetAsset()
    // instead of LoadObject which is blocked during PIE
    FString SearchName = FPaths::GetBaseFilename(WidgetPath);
    UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Searching asset registry for: %s"), *SearchName);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;

    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Asset registry returned %d widget blueprints"), AssetDataList.Num());

    // First pass: exact match
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
        {
            // Use GetAsset() instead of LoadObject - this works during PIE
            WidgetBP = Cast<UWidgetBlueprint>(AssetData.GetAsset());
            if (WidgetBP)
            {
                UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found exact match via asset registry: %s"), *WidgetBP->GetName());
                return WidgetBP;
            }
        }
    }

    // Second pass: contains match
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString().Contains(SearchName, ESearchCase::IgnoreCase))
        {
            WidgetBP = Cast<UWidgetBlueprint>(AssetData.GetAsset());
            if (WidgetBP)
            {
                UE_LOG(LogTemp, Display, TEXT("FindWidgetBlueprint: Found partial match via asset registry: %s"), *WidgetBP->GetName());
                return WidgetBP;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindWidgetBlueprint: Could not find widget blueprint for: %s"), *WidgetPath);
    return nullptr;
}

UObject* FAssetDiscoveryService::FindAssetByPath(const FString& AssetPath)
{
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Searching for asset: %s"), *AssetPath);
    
    // Try direct loading
    UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
    if (Asset)
    {
        UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct loading: %s"), *Asset->GetName());
        return Asset;
    }
    
    // Try with common paths
    TArray<FString> SearchPaths = GetCommonAssetSearchPaths(AssetPath);
    
    for (const FString& SearchPath : SearchPaths)
    {
        Asset = LoadObject<UObject>(nullptr, *SearchPath);
        if (Asset)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via search path: %s"), *SearchPath);
            return Asset;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find asset for: %s"), *AssetPath);
    return nullptr;
}

UObject* FAssetDiscoveryService::FindAssetByName(const FString& AssetName, const FString& AssetType)
{
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Searching for asset: %s (Type: %s)"), *AssetName, *AssetType);
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    
    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;
    
    if (!AssetType.IsEmpty())
    {
        if (AssetType.Equals(TEXT("Blueprint"), ESearchCase::IgnoreCase))
        {
            Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
        }
        else if (AssetType.Equals(TEXT("WidgetBlueprint"), ESearchCase::IgnoreCase))
        {
            Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
        }
    }
    
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString().Equals(AssetName, ESearchCase::IgnoreCase))
        {
            UObject* Asset = AssetData.GetAsset();
            if (Asset)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found asset: %s"), *Asset->GetName());
                return Asset;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find asset: %s"), *AssetName);
    return nullptr;
}

UScriptStruct* FAssetDiscoveryService::FindStructType(const FString& StructPath)
{
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Searching for struct: %s"), *StructPath);

    // Extract the base struct name (handle paths like /Game/Inventory/Data/S_ItemInstance)
    FString StructName = FPaths::GetBaseFilename(StructPath);
    // Also try without any path prefix
    FString CleanStructPath = StructPath;
    if (CleanStructPath.StartsWith(TEXT("Struct:")))
    {
        CleanStructPath = CleanStructPath.Mid(7); // Remove "Struct:" prefix
    }
    StructName = FPaths::GetBaseFilename(CleanStructPath);

    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Extracted struct name: %s from path: %s"), *StructName, *StructPath);

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
    if (const UScriptStruct* const* FoundBuiltIn = BuiltInStructs.Find(StructName))
    {
        UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found built-in struct: %s"), *(*FoundBuiltIn)->GetName());
        return const_cast<UScriptStruct*>(*FoundBuiltIn);
    }

    // Strategy 2: Try direct loading with the full path if it looks like a path
    if (CleanStructPath.StartsWith(TEXT("/")) || CleanStructPath.Contains(TEXT(".")))
    {
        // Try as UUserDefinedStruct (user-created structs in editor)
        UUserDefinedStruct* DirectUserStruct = LoadObject<UUserDefinedStruct>(nullptr, *CleanStructPath);
        if (DirectUserStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct path (UserDefinedStruct): %s"), *DirectUserStruct->GetName());
            return DirectUserStruct;
        }

        // Try as regular UScriptStruct (C++ structs)
        UScriptStruct* DirectStruct = LoadObject<UScriptStruct>(nullptr, *CleanStructPath);
        if (DirectStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct path (ScriptStruct): %s"), *DirectStruct->GetName());
            return DirectStruct;
        }

        // Try with .StructName suffix (asset reference format)
        FString AssetPath = FString::Printf(TEXT("%s.%s"), *CleanStructPath, *StructName);
        DirectUserStruct = LoadObject<UUserDefinedStruct>(nullptr, *AssetPath);
        if (DirectUserStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via asset path: %s"), *DirectUserStruct->GetName());
            return DirectUserStruct;
        }
    }

    // Strategy 3: Search using asset registry for UUserDefinedStruct
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());

    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found %d UserDefinedStruct assets in registry"), AssetDataList.Num());

    // First pass: exact match on name
    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Equals(StructName, ESearchCase::IgnoreCase))
        {
            UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(AssetData.GetAsset());
            if (UserStruct)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found exact match UserDefinedStruct: %s at %s"),
                    *UserStruct->GetName(), *AssetData.GetObjectPathString());
                return UserStruct;
            }
        }
    }

    // Second pass: Contains match (for partial names)
    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Contains(StructName, ESearchCase::IgnoreCase) ||
            StructName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(AssetData.GetAsset());
            if (UserStruct)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found partial match UserDefinedStruct: %s at %s"),
                    *UserStruct->GetName(), *AssetData.GetObjectPathString());
                return UserStruct;
            }
        }
    }

    // Strategy 4: Try with common engine paths for C++ structs
    TArray<FString> CommonPaths = {
        BuildEnginePath(StructName),
        BuildCorePath(StructName),
        BuildGamePath(StructName)
    };

    for (const FString& Path : CommonPaths)
    {
        UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *Path);
        if (FoundStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via common path: %s"), *Path);
            return FoundStruct;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find struct: %s (searched as: %s)"), *StructPath, *StructName);
    return nullptr;
}

UEnum* FAssetDiscoveryService::FindEnumType(const FString& EnumPath)
{
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Searching for enum: %s"), *EnumPath);

    // Extract the base enum name (handle paths like /Game/Inventory/Data/E_EquipmentSlot)
    FString EnumName = FPaths::GetBaseFilename(EnumPath);
    // Also try without any path prefix
    FString CleanEnumPath = EnumPath;
    if (CleanEnumPath.StartsWith(TEXT("Enum:")))
    {
        CleanEnumPath = CleanEnumPath.Mid(5); // Remove "Enum:" prefix
    }
    EnumName = FPaths::GetBaseFilename(CleanEnumPath);

    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Extracted enum name: %s from path: %s"), *EnumName, *EnumPath);

    // Strategy 1: Try direct loading with the full path if it looks like a path
    if (CleanEnumPath.StartsWith(TEXT("/")) || CleanEnumPath.Contains(TEXT(".")))
    {
        // Try as UUserDefinedEnum (user-created enums in editor)
        UUserDefinedEnum* DirectUserEnum = LoadObject<UUserDefinedEnum>(nullptr, *CleanEnumPath);
        if (DirectUserEnum)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct path (UserDefinedEnum): %s"), *DirectUserEnum->GetName());
            return DirectUserEnum;
        }

        // Try as regular UEnum (C++ enums)
        UEnum* DirectEnum = LoadObject<UEnum>(nullptr, *CleanEnumPath);
        if (DirectEnum)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct path (UEnum): %s"), *DirectEnum->GetName());
            return DirectEnum;
        }

        // Try with .EnumName suffix (asset reference format)
        FString AssetPath = FString::Printf(TEXT("%s.%s"), *CleanEnumPath, *EnumName);
        DirectUserEnum = LoadObject<UUserDefinedEnum>(nullptr, *AssetPath);
        if (DirectUserEnum)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via asset path: %s"), *DirectUserEnum->GetName());
            return DirectUserEnum;
        }
    }

    // Strategy 2: Search using asset registry for UUserDefinedEnum
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UUserDefinedEnum::StaticClass()->GetClassPathName());

    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found %d UserDefinedEnum assets in registry"), AssetDataList.Num());

    // First pass: exact match on name
    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Equals(EnumName, ESearchCase::IgnoreCase))
        {
            UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(AssetData.GetAsset());
            if (UserEnum)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found exact match UserDefinedEnum: %s at %s"),
                    *UserEnum->GetName(), *AssetData.GetObjectPathString());
                return UserEnum;
            }
        }
    }

    // Second pass: Contains match (for partial names)
    for (const FAssetData& AssetData : AssetDataList)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Contains(EnumName, ESearchCase::IgnoreCase) ||
            EnumName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(AssetData.GetAsset());
            if (UserEnum)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found partial match UserDefinedEnum: %s at %s"),
                    *UserEnum->GetName(), *AssetData.GetObjectPathString());
                return UserEnum;
            }
        }
    }

    // Strategy 3: Try with common engine paths for C++ enums
    TArray<FString> CommonPaths = {
        BuildEnginePath(EnumName),
        BuildCorePath(EnumName),
        BuildGamePath(EnumName)
    };

    for (const FString& Path : CommonPaths)
    {
        UEnum* FoundEnum = LoadObject<UEnum>(nullptr, *Path);
        if (FoundEnum)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via common path: %s"), *Path);
            return FoundEnum;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find enum: %s (searched as: %s)"), *EnumPath, *EnumName);
    return nullptr;
}

TArray<FString> FAssetDiscoveryService::GetCommonAssetSearchPaths(const FString& AssetName)
{
    TArray<FString> SearchPaths;
    
    FString BaseName = FPaths::GetBaseFilename(AssetName);
    
    // Add various common paths
    SearchPaths.Add(AssetName); // Original path
    SearchPaths.Add(BuildGamePath(AssetName));
    SearchPaths.Add(BuildGamePath(FString::Printf(TEXT("Blueprints/%s"), *BaseName)));
    SearchPaths.Add(BuildGamePath(FString::Printf(TEXT("UI/%s"), *BaseName)));
    SearchPaths.Add(BuildGamePath(FString::Printf(TEXT("Widgets/%s"), *BaseName)));
    SearchPaths.Add(BuildGamePath(FString::Printf(TEXT("Data/%s"), *BaseName)));
    
    return SearchPaths;
}

FString FAssetDiscoveryService::NormalizeAssetPath(const FString& AssetPath)
{
    FString NormalizedPath = AssetPath;
    
    // Remove .uasset extension
    if (NormalizedPath.EndsWith(TEXT(".uasset")))
    {
        NormalizedPath = NormalizedPath.LeftChop(7);
    }
    
    // Ensure it starts with /Game/ if it's a relative path
    if (!NormalizedPath.StartsWith(TEXT("/")) && !NormalizedPath.StartsWith(TEXT("Game/")))
    {
        NormalizedPath = BuildGamePath(NormalizedPath);
    }
    
    return NormalizedPath;
}

bool FAssetDiscoveryService::IsValidAssetPath(const FString& AssetPath)
{
    return UEditorAssetLibrary::DoesAssetExist(AssetPath);
}

UClass* FAssetDiscoveryService::ResolveObjectClass(const FString& ClassName)
{
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Resolving object class: %s"), *ClassName);
    
    // Try engine classes first
    UClass* EngineClass = ResolveEngineClass(ClassName);
    if (EngineClass)
    {
        return EngineClass;
    }
    
    // Try UMG classes
    UClass* UMGClass = ResolveUMGClass(ClassName);
    if (UMGClass)
    {
        return UMGClass;
    }
    
    // Try direct loading with various paths
    TArray<FString> SearchPaths = {
        ClassName,
        BuildEnginePath(ClassName),
        BuildCorePath(ClassName),
        BuildUMGPath(ClassName),
        BuildGamePath(ClassName),
        BuildGamePath(FString::Printf(TEXT("Blueprints/%s"), *ClassName))
    };

    // CRITICAL FIX: For Blueprint classes, also try appending the _C suffix
    // Blueprint generated classes use format: /Game/Path/To/BP_Name.BP_Name_C
    TArray<FString> BlueprintPaths;
    for (const FString& SearchPath : SearchPaths)
    {
        // Only try Blueprint class format for /Game/ paths
        if (SearchPath.StartsWith(TEXT("/Game/")))
        {
            FString AssetName = FPaths::GetBaseFilename(SearchPath);

            // If the path doesn't already have the _C suffix, try adding it
            if (!SearchPath.EndsWith(TEXT("_C")))
            {
                // Format: /Game/Path/To/BP_Name.BP_Name_C
                BlueprintPaths.Add(FString::Printf(TEXT("%s.%s_C"), *SearchPath, *AssetName));
            }
        }
    }

    // Combine both search path lists
    SearchPaths.Append(BlueprintPaths);

    for (const FString& SearchPath : SearchPaths)
    {
        UClass* FoundClass = LoadObject<UClass>(nullptr, *SearchPath);
        if (FoundClass)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found class via search path: %s -> %s"), *SearchPath, *FoundClass->GetName());
            return FoundClass;
        }
    }

    // Strategy: Use asset registry to find Blueprint by name
    // This handles cases like "BP_DialogueComponent" without requiring full path
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());

    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    FString SearchName = FPaths::GetBaseFilename(ClassName);

    // First pass: exact match
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
        {
            UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
            if (Blueprint && Blueprint->GeneratedClass)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found Blueprint class via asset registry: %s -> %s"),
                    *SearchName, *Blueprint->GeneratedClass->GetName());
                return Blueprint->GeneratedClass;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not resolve object class: %s"), *ClassName);
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Tried the following paths:"));
    for (const FString& SearchPath : SearchPaths)
    {
        UE_LOG(LogTemp, Warning, TEXT("  - %s"), *SearchPath);
    }
    return nullptr;
}

UClass* FAssetDiscoveryService::ResolveUMGClass(const FString& ClassName)
{
    if (ClassName.Equals(TEXT("UserWidget"), ESearchCase::IgnoreCase))
    {
        return UUserWidget::StaticClass();
    }
    else if (ClassName.Equals(TEXT("Widget"), ESearchCase::IgnoreCase))
    {
        return UWidget::StaticClass();
    }
    else if (ClassName.Equals(TEXT("PanelWidget"), ESearchCase::IgnoreCase))
    {
        return UPanelWidget::StaticClass();
    }
    
    return nullptr;
}

UClass* FAssetDiscoveryService::ResolveEngineClass(const FString& ClassName)
{
    if (ClassName.Equals(TEXT("Actor"), ESearchCase::IgnoreCase))
    {
        return AActor::StaticClass();
    }
    else if (ClassName.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase))
    {
        return APawn::StaticClass();
    }
    else if (ClassName.Equals(TEXT("Character"), ESearchCase::IgnoreCase))
    {
        return ACharacter::StaticClass();
    }
    else if (ClassName.Equals(TEXT("PlayerController"), ESearchCase::IgnoreCase))
    {
        return APlayerController::StaticClass();
    }
    else if (ClassName.Equals(TEXT("GameMode"), ESearchCase::IgnoreCase))
    {
        return AGameModeBase::StaticClass();
    }
    else if (ClassName.Equals(TEXT("Object"), ESearchCase::IgnoreCase))
    {
        return UObject::StaticClass();
    }
    
    return nullptr;
}

FString FAssetDiscoveryService::BuildGamePath(const FString& Path)
{
    FString CleanPath = Path;
    
    // Remove leading slash
    if (CleanPath.StartsWith(TEXT("/")))
    {
        CleanPath = CleanPath.RightChop(1);
    }
    
    // Fix: Check if already starts with Game/
    if (CleanPath.StartsWith(TEXT("Game/"), ESearchCase::IgnoreCase))
    {
        // Already has Game/ prefix, just add leading slash
        return FString::Printf(TEXT("/%s"), *CleanPath);
    }
    
    return FString::Printf(TEXT("/Game/%s"), *CleanPath);
}

FString FAssetDiscoveryService::BuildEnginePath(const FString& Path)
{
    return FString::Printf(TEXT("/Script/Engine.%s"), *Path);
}

FString FAssetDiscoveryService::BuildCorePath(const FString& Path)
{
    return FString::Printf(TEXT("/Script/CoreUObject.%s"), *Path);
}

FString FAssetDiscoveryService::BuildUMGPath(const FString& Path)
{
    return FString::Printf(TEXT("/Script/UMG.%s"), *Path);
}
