#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/BlueprintNodeService.h"

/**
 * Command to delete a node from a Blueprint graph.
 */
class FDeleteNodeCommand : public IUnrealMCPCommand
{
public:
    explicit FDeleteNodeCommand(IBlueprintNodeService& InService)
        : Service(InService)
    {
    }

    virtual ~FDeleteNodeCommand() = default;

    virtual FString GetCommandName() const override
    {
        return TEXT("delete_node");
    }

    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override { return true; }

private:
    IBlueprintNodeService& Service;
    
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
