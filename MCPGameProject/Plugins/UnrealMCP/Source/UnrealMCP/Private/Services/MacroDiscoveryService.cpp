#include "Services/MacroDiscoveryService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/EditorEngine.h"
#include "Misc/Paths.h"

UBlueprint* FMacroDiscoveryService::FindMacroBlueprint(const FString& MacroName)
{
    UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Searching for macro blueprint: %s"), *MacroName);
    
    // Strategy 1: Try loading from common macro paths
    UBlueprint* MacroBlueprint = TryLoadMacroFromCommonPaths(MacroName);
    if (MacroBlueprint)
    {
        UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found macro via common paths: %s"), *MacroBlueprint->GetName());
        return MacroBlueprint;
    }
    
    // Strategy 2: Use asset registry for dynamic discovery
    MacroBlueprint = FindMacroUsingAssetRegistry(MacroName);
    if (MacroBlueprint)
    {
        UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found macro via asset registry: %s"), *MacroBlueprint->GetName());
        return MacroBlueprint;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("MacroDiscoveryService: Could not find macro blueprint for: %s"), *MacroName);
    return nullptr;
}

UEdGraph* FMacroDiscoveryService::FindMacroGraph(UBlueprint* MacroBlueprint, const FString& MacroGraphName)
{
    if (!MacroBlueprint || MacroBlueprint->MacroGraphs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MacroDiscoveryService: Invalid macro blueprint or no macro graphs"));
        return nullptr;
    }
    
    UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Searching for macro graph '%s' in blueprint with %d graphs"), 
           *MacroGraphName, MacroBlueprint->MacroGraphs.Num());
    
    // Find the specific macro graph by name
    for (UEdGraph* MacroGraph : MacroBlueprint->MacroGraphs)
    {
        if (MacroGraph && MacroGraph->GetFName().ToString().Equals(MacroGraphName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found macro graph: %s"), *MacroGraphName);
            return MacroGraph;
        }
        
        if (MacroGraph)
        {
            UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Available macro graph: %s"), *MacroGraph->GetFName().ToString());
        }
    }
    
    // If specific graph not found, try first graph as fallback for single-macro blueprints
    if (MacroBlueprint->MacroGraphs.Num() == 1)
    {
        UEdGraph* FirstGraph = MacroBlueprint->MacroGraphs[0];
        UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Using single available macro graph as fallback: %s"), 
               FirstGraph ? *FirstGraph->GetFName().ToString() : TEXT("NULL"));
        return FirstGraph;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("MacroDiscoveryService: Could not find macro graph '%s'"), *MacroGraphName);
    return nullptr;
}

TArray<FString> FMacroDiscoveryService::FindAllMacroBlueprints(const FString& SearchPath)
{
    TArray<FString> FoundMacros;
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Blueprint assets
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    
    // Add search paths
    TArray<FString> SearchPaths = GetMacroSearchPaths();
    if (!SearchPath.IsEmpty())
    {
        SearchPaths.Add(SearchPath);
    }
    
    for (const FString& Path : SearchPaths)
    {
        Filter.PackagePaths.Add(FName(*Path));
    }
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    // Filter for blueprints that contain macro graphs
    for (const FAssetData& AssetData : AssetDataList)
    {
        UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
        if (Blueprint && Blueprint->MacroGraphs.Num() > 0)
        {
            FString AssetPath = AssetData.GetSoftObjectPath().ToString();
            FoundMacros.Add(AssetPath);
            UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found macro blueprint: %s (with %d macro graphs)"), 
                   *AssetPath, Blueprint->MacroGraphs.Num());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found %d macro blueprints total"), FoundMacros.Num());
    return FoundMacros;
}

bool FMacroDiscoveryService::IsMacroFunction(const FString& FunctionName)
{
    // Check against known macro function names
    static TArray<FString> KnownMacros = {
        TEXT("Loop"), TEXT("For Loop"), TEXT("ForLoop"),
        TEXT("Loop with Break"), TEXT("LoopWithBreak"),
        TEXT("For Loop with Break"), TEXT("ForLoopWithBreak"),
        TEXT("For Each Loop"), TEXT("ForEachLoop"),
        TEXT("For Each Loop with Break"), TEXT("ForEachLoopWithBreak"),
        TEXT("Reverse for Each Loop"), TEXT("ReverseForEachLoop"),
        TEXT("While Loop"), TEXT("WhileLoop"),
        TEXT("Do Once"), TEXT("DoOnce"),
        TEXT("Do N"), TEXT("DoN"),
        TEXT("MultiGate"), TEXT("Multi Gate"),
        TEXT("Flip Flop"), TEXT("FlipFlop")
    };
    
    for (const FString& KnownMacro : KnownMacros)
    {
        if (FunctionName.Equals(KnownMacro, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }
    
    return false;
}

TArray<FString> FMacroDiscoveryService::GetMacroSearchPaths()
{
    TArray<FString> SearchPaths;
    
    // Engine macro locations (most common for standard macros)
    SearchPaths.Add(TEXT("/Engine/EditorBlueprintResources"));
    SearchPaths.Add(TEXT("/Engine/Content"));
    SearchPaths.Add(TEXT("/Engine"));
    
    // Game content macro locations (for user-defined macros)
    SearchPaths.Add(TEXT("/Game/Blueprints/Macros"));
    SearchPaths.Add(TEXT("/Game/Macros"));
    SearchPaths.Add(TEXT("/Game/Blueprints"));
    SearchPaths.Add(TEXT("/Game"));
    
    return SearchPaths;
}

FString FMacroDiscoveryService::MapFunctionNameToMacroGraphName(const FString& FunctionName)
{
    // Map common function name variations to their actual macro graph names
    static TMap<FString, FString> NameMappings = {
        {TEXT("For Each Loop"), TEXT("ForEachLoop")},
        {TEXT("ForEachLoop"), TEXT("ForEachLoop")},
        {TEXT("For Loop"), TEXT("ForLoop")},
        {TEXT("ForLoop"), TEXT("ForLoop")},
        {TEXT("Loop"), TEXT("ForLoop")},
        {TEXT("For Each Loop with Break"), TEXT("ForEachLoopWithBreak")},
        {TEXT("ForEachLoopWithBreak"), TEXT("ForEachLoopWithBreak")},
        {TEXT("For Loop with Break"), TEXT("ForLoopWithBreak")},
        {TEXT("ForLoopWithBreak"), TEXT("ForLoopWithBreak")},
        {TEXT("Loop with Break"), TEXT("ForLoopWithBreak")},
        {TEXT("LoopWithBreak"), TEXT("ForLoopWithBreak")},
        {TEXT("While Loop"), TEXT("WhileLoop")},
        {TEXT("WhileLoop"), TEXT("WhileLoop")},
        {TEXT("Do Once"), TEXT("DoOnce")},
        {TEXT("DoOnce"), TEXT("DoOnce")},
        {TEXT("Do N"), TEXT("DoN")},
        {TEXT("DoN"), TEXT("DoN")},
        {TEXT("MultiGate"), TEXT("MultiGate")},
        {TEXT("Multi Gate"), TEXT("MultiGate")},
        {TEXT("Flip Flop"), TEXT("FlipFlop")},
        {TEXT("FlipFlop"), TEXT("FlipFlop")}
    };
    
    for (const auto& Mapping : NameMappings)
    {
        if (FunctionName.Equals(Mapping.Key, ESearchCase::IgnoreCase))
        {
            return Mapping.Value;
        }
    }
    
    // If no mapping found, return the original name
    return FunctionName;
}

UBlueprint* FMacroDiscoveryService::TryLoadMacroFromCommonPaths(const FString& MacroName)
{
    // Try loading from known macro blueprint locations
    TArray<FString> CommonMacroPaths = {
        TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"),
        TEXT("/Engine/EditorBlueprintResources/StandardMacros"),
        FString::Printf(TEXT("/Game/Blueprints/Macros/%s.%s"), *MacroName, *MacroName),
        FString::Printf(TEXT("/Game/Macros/%s.%s"), *MacroName, *MacroName),
        FString::Printf(TEXT("/Game/Blueprints/%s.%s"), *MacroName, *MacroName)
    };
    
    for (const FString& Path : CommonMacroPaths)
    {
        UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Trying common path: %s"), *Path);
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
        if (Blueprint && Blueprint->MacroGraphs.Num() > 0)
        {
            UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found macro blueprint at: %s"), *Path);
            return Blueprint;
        }
    }
    
    return nullptr;
}

UBlueprint* FMacroDiscoveryService::FindMacroUsingAssetRegistry(const FString& MacroName)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Blueprint assets
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    
    // Add all search paths
    TArray<FString> SearchPaths = GetMacroSearchPaths();
    for (const FString& Path : SearchPaths)
    {
        Filter.PackagePaths.Add(FName(*Path));
    }
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Searching %d blueprint assets for macro '%s'"), 
           AssetDataList.Num(), *MacroName);
    
    // Search for blueprints that contain the requested macro
    for (const FAssetData& AssetData : AssetDataList)
    {
        UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
        if (!Blueprint || Blueprint->MacroGraphs.Num() == 0)
        {
            continue;
        }
        
        // Check if this blueprint contains a macro with the requested name
        FString MappedMacroName = MapFunctionNameToMacroGraphName(MacroName);
        for (UEdGraph* MacroGraph : Blueprint->MacroGraphs)
        {
            if (MacroGraph && MacroGraph->GetFName().ToString().Equals(MappedMacroName, ESearchCase::IgnoreCase))
            {
                FString AssetPath = AssetData.GetSoftObjectPath().ToString();
                UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found matching macro '%s' in blueprint: %s"), 
                       *MappedMacroName, *AssetPath);
                return Blueprint;
            }
        }
        
        // Also check for asset name matches (like "StandardMacros")
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Contains(TEXT("Macro"), ESearchCase::IgnoreCase) || 
            AssetName.Contains(TEXT("Standard"), ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Display, TEXT("MacroDiscoveryService: Found potential macro blueprint by name: %s"), *AssetName);
            return Blueprint;
        }
    }
    
    return nullptr;
} 
