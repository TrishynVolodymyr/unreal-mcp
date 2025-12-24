#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IUMGService;

/**
 * Command to retrieve comprehensive metadata about a Widget Blueprint.
 *
 * Consolidates the following deprecated tools:
 * - check_widget_component_exists → fields=["components"]
 * - get_widget_component_layout → fields=["layout"]
 * - get_widget_container_component_dimensions → fields=["dimensions"]
 *
 * Supported fields:
 * - "components" - List all widget components in the blueprint
 * - "layout" - Full hierarchical layout with positions, sizes, anchors
 * - "dimensions" - Container dimensions
 * - "hierarchy" - Parent/child widget tree structure
 * - "bindings" - Property bindings
 * - "events" - Bound events and delegates
 * - "variables" - Blueprint variables
 * - "functions" - Blueprint functions
 * - "orphaned_nodes" - Nodes with no connections in blueprint graphs
 * - "graph_warnings" - Cast nodes with disconnected exec pins and other issues
 * - "*" - Return all available fields
 */
class UNREALMCP_API FGetWidgetBlueprintMetadataCommand : public IUnrealMCPCommand
{
public:
	FGetWidgetBlueprintMetadataCommand(TSharedPtr<IUMGService> InUMGService);
	virtual ~FGetWidgetBlueprintMetadataCommand() = default;

	// IUnrealMCPCommand interface
	virtual FString GetCommandName() const override;
	virtual FString Execute(const FString& Parameters) override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	/** UMG Service for widget operations */
	TSharedPtr<IUMGService> UMGService;

	/** Internal execution with parsed JSON */
	TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);

	/** Parameter validation with error output */
	bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;

	/** Check if a field is requested */
	bool ShouldIncludeField(const TArray<FString>& RequestedFields, const FString& FieldName) const;

	/** Build metadata sections */
	TSharedPtr<FJsonObject> BuildComponentsInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildLayoutInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildDimensionsInfo(class UWidgetBlueprint* WidgetBlueprint, const FString& ContainerName) const;
	TSharedPtr<FJsonObject> BuildHierarchyInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildBindingsInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildEventsInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildVariablesInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildFunctionsInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildOrphanedNodesInfo(class UWidgetBlueprint* WidgetBlueprint) const;
	TSharedPtr<FJsonObject> BuildGraphWarningsInfo(class UWidgetBlueprint* WidgetBlueprint) const;

	/** Helper to collect all widgets in the widget tree */
	void CollectAllWidgets(class UWidget* Widget, TArray<class UWidget*>& OutWidgets) const;

	/** Helper to build widget info for a single widget */
	TSharedPtr<FJsonObject> BuildWidgetInfo(class UWidget* Widget) const;

	/** Helper to get available delegate events for a widget */
	TArray<FString> GetAvailableDelegateEvents(class UWidget* Widget) const;

	/** Response builders */
	TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& MetadataObj) const;
	TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage) const;
};
