#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command to get level metadata with selective field querying.
 * Consolidates: get_actors_in_level, find_actors_by_name
 *
 * Supported fields:
 * - "actors": All actors in the level (optionally filtered by actor_filter)
 * - "*": All fields (default if no fields specified)
 *
 * Parameters:
 * - fields: Array of field names to include (default: ["actors"])
 * - actor_filter: Optional pattern for actor name filtering (supports wildcards *)
 */
class FGetLevelMetadataCommand : public IUnrealMCPCommand
{
public:
    FGetLevelMetadataCommand(IEditorService& InEditorService);

    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;
    virtual FString Execute(const FString& Parameters) override;

private:
    IEditorService& EditorService;

    /** Check if a specific field is requested */
    bool IsFieldRequested(const TArray<TSharedPtr<FJsonValue>>* FieldsArray, const FString& FieldName) const;

    /** Build actors info with optional filtering */
    TSharedPtr<FJsonObject> BuildActorsInfo(const FString& ActorFilter) const;
};
