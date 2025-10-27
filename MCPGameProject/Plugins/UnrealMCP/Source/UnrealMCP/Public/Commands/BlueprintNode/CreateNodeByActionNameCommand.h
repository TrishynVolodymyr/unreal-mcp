#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintActionService.h"

/**
 * Command for creating Blueprint nodes by action/function name using the Blueprint Action Database
 * Follows the new architecture with service layer delegation and structured error handling
 */
class UNREALMCP_API FCreateNodeByActionNameCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintActionService - The blueprint action service to use
     */
    explicit FCreateNodeByActionNameCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** The blueprint action service instance */
    TSharedPtr<IBlueprintActionService> BlueprintActionService;
};