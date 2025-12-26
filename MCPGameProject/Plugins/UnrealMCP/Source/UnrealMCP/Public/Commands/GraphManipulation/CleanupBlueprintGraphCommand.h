#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IBlueprintService;

/**
 * Unified command for cleaning up Blueprint graphs.
 *
 * Supports multiple cleanup modes:
 * - "orphans": Delete orphaned nodes not reachable from entry points
 * - "print_strings": Delete Print String nodes and rewire execution flow
 *
 * For print_strings mode:
 * - If Print String is middleware (has both execute input and then output connected):
 *   Rewires the source node's exec to the target node's exec, then deletes
 * - If Print String is a dead end (only execute input connected):
 *   Simply deletes the node
 */
class UNREALMCP_API FCleanupBlueprintGraphCommand : public IUnrealMCPCommand
{
public:
    FCleanupBlueprintGraphCommand(IBlueprintService& InBlueprintService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    IBlueprintService& BlueprintService;

    /** Cleanup mode enum */
    enum class ECleanupMode
    {
        Orphans,
        PrintStrings
    };

    /**
     * Parse command parameters
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName,
                        ECleanupMode& OutCleanupMode, FString& OutGraphName,
                        bool& OutIncludeEventGraph, FString& OutError) const;

    /**
     * Execute orphans cleanup (delegates to existing logic)
     */
    FString ExecuteOrphansCleanup(UBlueprint* Blueprint, const FString& GraphName,
                                  bool bIncludeEventGraph);

    /**
     * Execute print strings cleanup
     */
    FString ExecutePrintStringsCleanup(UBlueprint* Blueprint, const FString& GraphName,
                                       bool bIncludeEventGraph);

    /**
     * Check if a node is a Print String node
     */
    bool IsPrintStringNode(UEdGraphNode* Node) const;

    /**
     * Get the exec input pin for a node
     */
    UEdGraphPin* GetExecInputPin(UEdGraphNode* Node) const;

    /**
     * Get the exec output pin (then) for a node
     */
    UEdGraphPin* GetExecOutputPin(UEdGraphNode* Node) const;

    FString CreateSuccessResponse(const FString& BlueprintName, const FString& CleanupMode,
                                 int32 DeletedCount, int32 RewiredCount,
                                 const TArray<FString>& DeletedNodeTitles,
                                 const TArray<FString>& SkippedGraphs) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
