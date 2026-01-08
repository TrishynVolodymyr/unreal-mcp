#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for deleting multiple actors by name in a single operation
 * Implements the IUnrealMCPCommand interface for standardized command execution
 *
 * Parameters:
 *   names: Array of actor names to delete
 *
 * Returns:
 *   JSON object with results per actor:
 *   {
 *     "results": [
 *       {"name": "Actor1", "success": true, "deleted": true},
 *       {"name": "Actor2", "success": false, "error": "Actor not found"}
 *     ],
 *     "total": 2,
 *     "succeeded": 1,
 *     "failed": 1,
 *     "success": true
 *   }
 */
class UNREALMCP_API FBatchDeleteActorsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InEditorService - Reference to the editor service for operations
     */
    explicit FBatchDeleteActorsCommand(IEditorService& InEditorService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the editor service */
    IEditorService& EditorService;

    /**
     * Parse JSON parameters
     * @param JsonString - JSON string containing parameters
     * @param OutActorNames - Parsed array of actor names
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, TArray<FString>& OutActorNames, FString& OutError) const;

    /**
     * Create success response JSON with detailed per-actor results
     * @param Results - Array of result objects for each actor
     * @param TotalCount - Total number of actors processed
     * @param SuccessCount - Number of successfully deleted actors
     * @param FailedCount - Number of failed deletions
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
