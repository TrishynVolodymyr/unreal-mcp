#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;
class IUMGService;

/**
 * Service for building Widget Blueprint metadata JSON objects.
 * Handles the construction of various metadata sections like components, layout,
 * hierarchy, bindings, events, variables, functions, etc.
 *
 * Extracted from GetWidgetBlueprintMetadataCommand for better separation of concerns.
 */
class UNREALMCP_API FWidgetMetadataBuilderService
{
public:
    FWidgetMetadataBuilderService(TSharedPtr<IUMGService> InUMGService);

    // Metadata section builders
    TSharedPtr<FJsonObject> BuildComponentsInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildLayoutInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildDimensionsInfo(UWidgetBlueprint* WidgetBlueprint, const FString& ContainerName) const;
    TSharedPtr<FJsonObject> BuildHierarchyInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildBindingsInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildEventsInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildVariablesInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildFunctionsInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildOrphanedNodesInfo(UWidgetBlueprint* WidgetBlueprint) const;
    TSharedPtr<FJsonObject> BuildGraphWarningsInfo(UWidgetBlueprint* WidgetBlueprint) const;

    // Helper methods
    void CollectAllWidgets(UWidget* Widget, TArray<UWidget*>& OutWidgets) const;
    TSharedPtr<FJsonObject> BuildWidgetInfo(UWidget* Widget) const;
    TArray<FString> GetAvailableDelegateEvents(UWidget* Widget) const;

private:
    TSharedPtr<IUMGService> UMGService;
};
