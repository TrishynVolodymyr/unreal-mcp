#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

class IBlueprintActionService;

/**
 * Command for getting specific information about a Blueprint node's pin
 */
class UNREALMCP_API FGetNodePinInfoCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintActionService - The blueprint action service to use
     */
    explicit FGetNodePinInfoCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** The blueprint action service instance */
    TSharedPtr<IBlueprintActionService> BlueprintActionService;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON error response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};