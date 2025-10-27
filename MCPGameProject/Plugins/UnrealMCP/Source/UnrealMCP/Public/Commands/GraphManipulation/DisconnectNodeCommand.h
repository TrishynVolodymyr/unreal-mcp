#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/BlueprintNodeService.h"

/**
 * Command to disconnect all connections from a specific node in a Blueprint graph.
 * This is useful for preparing a node for replacement or removal.
 */
class FDisconnectNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FDisconnectNodeCommand(IBlueprintNodeService& InService)
        : Service(InService)
    {
    }

    virtual ~FDisconnectNodeCommand() = default;

    virtual FString GetCommandName() const override
    {
        return TEXT("disconnect_node");
    }

    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override { return true; }

private:
    IBlueprintNodeService& Service;
    
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
