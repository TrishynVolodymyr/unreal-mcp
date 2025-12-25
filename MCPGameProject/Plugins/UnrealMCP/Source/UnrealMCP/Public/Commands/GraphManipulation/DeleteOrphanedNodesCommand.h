#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IBlueprintService;

/**
 * Command to delete orphaned nodes from a Blueprint graph.
 * Orphaned nodes are those not reachable from any entry point via execution flow
 * or data dependencies.
 */
class UNREALMCP_API FDeleteOrphanedNodesCommand : public IUnrealMCPCommand
{
public:
    FDeleteOrphanedNodesCommand(IBlueprintService& InBlueprintService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    /**
     * Parse command parameters
     * @param JsonString - Input JSON
     * @param OutBlueprintName - Target blueprint name
     * @param OutGraphName - Optional graph name filter (empty = all graphs except EventGraph)
     * @param OutIncludeEventGraph - Whether to include EventGraph (default: false)
     * @param OutExcludeReturnNodes - Whether to exclude auto-generated Return Nodes at (0,0) (default: true)
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName,
                        FString& OutGraphName, bool& OutIncludeEventGraph,
                        bool& OutExcludeReturnNodes, FString& OutError) const;

    FString CreateSuccessResponse(const FString& BlueprintName, int32 DeletedCount,
                                 const TArray<FString>& DeletedNodeTitles,
                                 const TArray<FString>& SkippedGraphs) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
