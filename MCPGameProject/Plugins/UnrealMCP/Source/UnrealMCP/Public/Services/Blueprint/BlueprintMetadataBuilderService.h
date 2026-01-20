#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class UBlueprint;
class UEdGraphNode;
class UEdGraph;
class IBlueprintService;
struct FEdGraphPinType;

/**
 * Detail level for graph nodes field.
 */
enum class EGraphNodesDetailLevel
{
    Summary,    // Node IDs and titles only
    Flow,       // Exec pin connections (default)
    Full        // All pin connections and default values
};

/**
 * Filter options for graph nodes field.
 */
struct FGraphNodesFilter
{
    FString GraphName;
    FString NodeType;
    FString EventType;
    FString ComponentName;  // For component_properties field
    EGraphNodesDetailLevel DetailLevel = EGraphNodesDetailLevel::Flow;
};

/**
 * Service for building Blueprint metadata JSON objects.
 * Handles the construction of various metadata sections like parent_class, interfaces,
 * variables, functions, components, graphs, etc.
 *
 * Extracted from GetBlueprintMetadataCommand for better separation of concerns.
 */
class UNREALMCP_API FBlueprintMetadataBuilderService
{
public:
    FBlueprintMetadataBuilderService(IBlueprintService& InBlueprintService);

    // Metadata section builders
    TSharedPtr<FJsonObject> BuildParentClassInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildInterfacesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildVariablesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildFunctionsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildComponentsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildComponentPropertiesInfo(UBlueprint* Blueprint, const FString& ComponentName) const;
    TSharedPtr<FJsonObject> BuildGraphsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildStatusInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildMetadataInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildTimelinesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildAssetInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildOrphanedNodesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildGraphWarningsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildGraphNodesInfo(UBlueprint* Blueprint, const FGraphNodesFilter& Filter) const;

    // Helper methods
    FString GetPinTypeAsString(const FEdGraphPinType& PinType) const;
    bool MatchesNodeTypeFilter(UEdGraphNode* Node, const FString& NodeType) const;
    bool MatchesEventTypeFilter(UEdGraphNode* Node, const FString& EventType) const;

private:
    IBlueprintService& BlueprintService;
};
