#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IStateTreeService;

/**
 * MCP Command: remove_state_tree_binding
 *
 * Removes a property binding from a target node in a StateTree.
 *
 * Parameters:
 *   state_tree_path (string, required): Path to the StateTree asset
 *   target_node_name (string, required): Target node identifier (state name or evaluator name)
 *   target_property_name (string, required): Property name on the target node with the binding to remove
 *   task_index (int, optional): Index of task within state (-1 to ignore, default)
 *   transition_index (int, optional): Index of transition (-1 to ignore, default)
 *   condition_index (int, optional): Index of condition within transition (-1 to ignore, default)
 */
class UNREALMCP_API FRemoveBindingCommand : public IUnrealMCPCommand
{
public:
    explicit FRemoveBindingCommand(IStateTreeService& InService);

    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    FString CreateErrorResponse(const FString& ErrorMessage) const;

    IStateTreeService& Service;
};
