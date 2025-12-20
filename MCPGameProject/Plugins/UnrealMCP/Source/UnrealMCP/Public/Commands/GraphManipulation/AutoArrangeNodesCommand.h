#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to auto-arrange nodes in a Blueprint graph for better readability.
 * Uses connection-aware horizontal flow layout.
 *
 * Parameters:
 *   - blueprint_name (string, required): Name or path of the Blueprint
 *   - graph_name (string, optional): Name of the graph to arrange (default: "EventGraph")
 *
 * Returns:
 *   - success (bool): Whether the operation succeeded
 *   - arranged_count (int): Number of nodes that were arranged
 *   - graph_name (string): Name of the graph that was arranged
 */
class UNREALMCP_API FAutoArrangeNodesCommand : public IUnrealMCPCommand
{
public:
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override { return TEXT("auto_arrange_nodes"); }
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& GraphName, int32 ArrangedCount) const;
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
