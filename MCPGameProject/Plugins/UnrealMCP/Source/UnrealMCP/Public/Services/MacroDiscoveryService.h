#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"

/**
 * Service for discovering macro blueprints using dynamic asset registry search
 * instead of hardcoded paths. Follows the established architectural pattern
 * from FUnrealMCPCommonUtils for consistent asset discovery.
 */
class UNREALMCP_API FMacroDiscoveryService
{
public:
    /**
     * Find a macro blueprint by name using dynamic discovery
     * @param MacroName - Name of the macro to find (e.g., "ForEachLoop", "DoOnce")
     * @return UBlueprint* - Found macro blueprint or nullptr
     */
    static UBlueprint* FindMacroBlueprint(const FString& MacroName);
    
    /**
     * Find a specific macro graph within a macro blueprint
     * @param MacroBlueprint - The blueprint containing macros
     * @param MacroGraphName - Name of the specific macro graph
     * @return UEdGraph* - Found macro graph or nullptr
     */
    static UEdGraph* FindMacroGraph(UBlueprint* MacroBlueprint, const FString& MacroGraphName);
    
    /**
     * Get all available macro blueprints in the project
     * @param SearchPath - Path to search for macros (defaults to engine + game)
     * @return TArray<FString> - Array of macro blueprint paths
     */
    static TArray<FString> FindAllMacroBlueprints(const FString& SearchPath = TEXT(""));
    
    /**
     * Check if a given function name corresponds to a known macro
     * @param FunctionName - Function name to check
     * @return bool - True if it's a known macro type
     */
    static bool IsMacroFunction(const FString& FunctionName);
    
    /**
     * Get macro search paths (engine + user-defined locations)
     * @return TArray<FString> - Array of paths to search for macros
     */
    static TArray<FString> GetMacroSearchPaths();
    
    /**
     * Map friendly function names to their actual macro graph names
     * @param FunctionName - Input function name (e.g., "For Each Loop")
     * @return FString - Mapped macro graph name (e.g., "ForEachLoop")
     */
    static FString MapFunctionNameToMacroGraphName(const FString& FunctionName);

private:
    /**
     * Try to load a macro blueprint from various common locations
     * @param MacroName - Name of the macro blueprint
     * @return UBlueprint* - Found blueprint or nullptr
     */
    static UBlueprint* TryLoadMacroFromCommonPaths(const FString& MacroName);
    
    /**
     * Use asset registry to find macro blueprints
     * @param MacroName - Name to search for
     * @return UBlueprint* - Found blueprint or nullptr
     */
    static UBlueprint* FindMacroUsingAssetRegistry(const FString& MacroName);
}; 
