#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/BlueprintNodeService.h"

/**
 * Command to get detailed information about a node's pin connections.
 * Useful for analyzing what needs to be reconnected during node replacement.
 */
class FGetNodeConnectionsCommand : public IUnrealMCPCommand
{
public:
    explicit FGetNodeConnectionsCommand(IBlueprintNodeService& InService)
        : Service(InService)
    {
    }

    virtual ~FGetNodeConnectionsCommand() = default;

    virtual FString GetCommandName() const override
    {
        return TEXT("get_node_connections");
    }

    virtual FString Execute(const FString& Parameters) override;

private:
    IBlueprintNodeService& Service;
};
