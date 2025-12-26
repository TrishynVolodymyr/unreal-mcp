#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/UMG/WidgetMetadataBuilderService.h"

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
 *
 * Uses WidgetMetadataBuilderService for building the actual metadata JSON objects.
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

	/** Metadata builder service */
	FWidgetMetadataBuilderService MetadataBuilder;

	/** Internal execution with parsed JSON */
	TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);

	/** Parameter validation with error output */
	bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;

	/** Check if a field is requested */
	bool ShouldIncludeField(const TArray<FString>& RequestedFields, const FString& FieldName) const;

	/**
	 * Find and load widget blueprint with retry mechanism.
	 * Handles transient asset loading issues by retrying after a brief delay.
	 */
	class UWidgetBlueprint* FindWidgetBlueprintWithRetry(const FString& WidgetName, TArray<FString>& OutAttemptedPaths) const;

	/** Response builders */
	TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& MetadataObj) const;
	TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage) const;
};
