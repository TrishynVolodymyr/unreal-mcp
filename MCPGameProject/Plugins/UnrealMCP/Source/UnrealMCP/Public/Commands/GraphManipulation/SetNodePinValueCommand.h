#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/BlueprintNodeService.h"

/**
 * Command to set the default value or selection for a pin on a Blueprint node.
 * Works for dropdown selectors, literal values, class references, etc.
 */
class FSetNodePinValueCommand : public IUnrealMCPCommand
{
public:
    explicit FSetNodePinValueCommand(IBlueprintNodeService& InService)
        : Service(InService)
    {
    }

    virtual ~FSetNodePinValueCommand() = default;

    virtual FString GetCommandName() const override
    {
        return TEXT("set_node_pin_value");
    }

    virtual FString Execute(const FString& Parameters) override;
    virtual bool ValidateParams(const FString& Parameters) const override { return true; }

private:
    IBlueprintNodeService& Service;
    
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
