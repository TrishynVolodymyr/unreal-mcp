#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraphPin.h"

class UBlueprint;
class IBlueprintService;

/**
 * Command to retrieve comprehensive metadata about a Blueprint
 * Supports selective field querying for performance optimization
 */
class UNREALMCP_API FGetBlueprintMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetBlueprintMetadataCommand(IBlueprintService& InBlueprintService);

private:
    IBlueprintService& BlueprintService;

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
     * @param OutError - Error message if parsing fails
     * @return True if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FString>& OutFields, FString& OutError) const;

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
     * @return JSON object with requested fields
     */
    TSharedPtr<FJsonObject> BuildMetadata(UBlueprint* Blueprint, const TArray<FString>& Fields) const;

    // Field builders (each builds a specific metadata category)
    TSharedPtr<FJsonObject> BuildParentClassInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildInterfacesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildVariablesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildFunctionsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildComponentsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildGraphsInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildStatusInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildMetadataInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildTimelinesInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildAssetInfo(UBlueprint* Blueprint) const;
    TSharedPtr<FJsonObject> BuildOrphanedNodesInfo(UBlueprint* Blueprint) const;

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

    /**
     * Convert FEdGraphPinType to a human-readable string
     * @param PinType - The pin type to convert
     * @return String representation of the type (e.g., "BP_DialogueNPC", "float", "FVector")
     */
    FString GetPinTypeAsString(const FEdGraphPinType& PinType) const;
};
