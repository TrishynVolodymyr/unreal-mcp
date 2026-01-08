#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for spawning multiple actors in a single operation
 * Implements the IUnrealMCPCommand interface for standardized command execution
 *
 * Parameters:
 *   actors: Array of actor spawn configurations, each containing:
 *     - name: Actor name (required)
 *     - type: Actor type - friendly name, Blueprint path, or Class path (required)
 *     - location: [X, Y, Z] spawn location (optional, default [0,0,0])
 *     - rotation: [Pitch, Yaw, Roll] spawn rotation (optional, default [0,0,0])
 *     - scale: [X, Y, Z] scale (optional, default [1,1,1])
 *     - (type-specific params like mesh_path, text_content, box_extent, etc.)
 *
 * Returns:
 *   JSON object with results per actor:
 *   {
 *     "results": [
 *       {"name": "Actor1", "success": true, "actor": {...actor details...}},
 *       {"name": "Actor2", "success": false, "error": "Failed to spawn"}
 *     ],
 *     "total": 2,
 *     "succeeded": 1,
 *     "failed": 1,
 *     "success": true
 *   }
 */
class UNREALMCP_API FBatchSpawnActorsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service for operations
     */
    explicit FBatchSpawnActorsCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;

    /**
     * Parse JSON parameters to extract actor configurations array
     * @param JsonString - JSON string containing parameters
     * @param OutActorConfigs - Parsed array of actor configuration JSON objects
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, TArray<TSharedPtr<FJsonObject>>& OutActorConfigs, FString& OutError) const;

    /**
     * Parse a single actor configuration into spawn params
     * @param ActorConfig - JSON object with actor configuration
     * @param OutParams - Parsed spawn parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseActorConfig(const TSharedPtr<FJsonObject>& ActorConfig, FActorSpawnParams& OutParams, FString& OutError) const;

    /**
     * Create success response JSON with detailed per-actor results
     * @param Results - Array of result objects for each actor
     * @param TotalCount - Total number of actors processed
     * @param SuccessCount - Number of successfully spawned actors
     * @param FailedCount - Number of failed spawns
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Results, int32 TotalCount, int32 SuccessCount, int32 FailedCount) const;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
