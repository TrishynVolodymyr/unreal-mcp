#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"
#include "Services/Blueprint/BlueprintMetadataBuilderService.h"

class UBlueprint;
class IBlueprintService;

/**
 * Command to retrieve comprehensive metadata about a Blueprint
 * Supports selective field querying for performance optimization
 *
 * Uses BlueprintMetadataBuilderService for building the actual metadata JSON objects.
 */
class UNREALMCP_API FGetBlueprintMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetBlueprintMetadataCommand(IBlueprintService& InBlueprintService);

private:
    IBlueprintService& BlueprintService;
    FBlueprintMetadataBuilderService MetadataBuilder;

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /**
     * Parse JSON parameters
     * @param JsonString - JSON parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutFields - Requested metadata fields (* for all)
     * @param OutFilter - Optional filters for graph_nodes field
     * @param OutError - Error message if parsing fails
     * @return True if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FString>& OutFields, FGraphNodesFilter& OutFilter, FString& OutError) const;

    /**
     * Find Blueprint by name or path
     * @param BlueprintName - Blueprint name or asset path
     * @return Found Blueprint or nullptr
     */
    UBlueprint* FindBlueprint(const FString& BlueprintName) const;

    /**
     * Build metadata fields based on requested fields
     * @param Blueprint - Target Blueprint
     * @param Fields - Requested field names (* for all)
     * @param Filter - Optional filters for graph_nodes field
     * @return JSON object with requested fields
     */
    TSharedPtr<FJsonObject> BuildMetadata(UBlueprint* Blueprint, const TArray<FString>& Fields, const FGraphNodesFilter& Filter) const;

    /**
     * Create success response
     * @param Metadata - Metadata JSON object
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const;

    /**
     * Create error response
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;

    /**
     * Check if field should be included
     * @param FieldName - Field to check
     * @param RequestedFields - List of requested fields
     * @return True if field should be included
     */
    bool ShouldIncludeField(const FString& FieldName, const TArray<FString>& RequestedFields) const;
};
