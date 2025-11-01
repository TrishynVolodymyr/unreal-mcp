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
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
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
    UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Searching for widget blueprint: %s"), *WidgetPath);
    
    // Try direct loading first
    UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
    if (WidgetBP)
    {
        UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct loading: %s"), *WidgetBP->GetName());
        return WidgetBP;
    }
    
    // Try with common paths
    TArray<FString> SearchPaths = GetCommonAssetSearchPaths(WidgetPath);
    
    for (const FString& SearchPath : SearchPaths)
    {
        WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *SearchPath);
        if (WidgetBP)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via search path: %s"), *SearchPath);
            return WidgetBP;
        }
    }
    
    // Use asset registry as fallback
    TArray<FString> FoundWidgets = FindWidgetBlueprints(FPaths::GetBaseFilename(WidgetPath));
    if (FoundWidgets.Num() > 0)
    {
        WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *FoundWidgets[0]);
        if (WidgetBP)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via asset registry: %s"), *FoundWidgets[0]);
            return WidgetBP;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find widget blueprint for: %s"), *WidgetPath);
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
    
    // Try direct loading first
    UScriptStruct* FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructPath);
    if (FoundStruct)
    {
        UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via direct loading: %s"), *FoundStruct->GetName());
        return FoundStruct;
    }
    
    // Try with common engine paths
    TArray<FString> CommonPaths = {
        BuildEnginePath(StructPath),
        BuildCorePath(StructPath),
        BuildGamePath(StructPath)
    };
    
    for (const FString& Path : CommonPaths)
    {
        FoundStruct = LoadObject<UScriptStruct>(nullptr, *Path);
        if (FoundStruct)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found via common path: %s"), *Path);
            return FoundStruct;
        }
    }
    
    // Try finding user-defined structs using asset registry (without UUserDefinedStruct class)
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    
    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;
    
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString().Contains(StructPath))
        {
            UObject* Asset = AssetData.GetAsset();
            UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Asset);
            if (ScriptStruct)
            {
                UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found user-defined struct: %s"), *ScriptStruct->GetName());
                return ScriptStruct;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not find struct: %s"), *StructPath);
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
    
    for (const FString& SearchPath : SearchPaths)
    {
        UClass* FoundClass = LoadObject<UClass>(nullptr, *SearchPath);
        if (FoundClass)
        {
            UE_LOG(LogTemp, Display, TEXT("AssetDiscoveryService: Found class via search path: %s -> %s"), *SearchPath, *FoundClass->GetName());
            return FoundClass;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AssetDiscoveryService: Could not resolve object class: %s"), *ClassName);
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
