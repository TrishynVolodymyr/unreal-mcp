#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintActionService.h"

/**
 * Command for getting all available Blueprint actions for a specific class
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FGetActionsForClassCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintActionService - The blueprint action service to use
     */
    explicit FGetActionsForClassCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** The blueprint action service instance */
    TSharedPtr<IBlueprintActionService> BlueprintActionService;

    /**
     * Parse parameters from JSON string
     * @param Parameters - JSON string containing parameters
     * @param OutClassName - Parsed class name
     * @param OutSearchFilter - Parsed search filter
     * @param OutMaxResults - Parsed max results
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(
        const FString& Parameters,
        FString& OutClassName,
        FString& OutSearchFilter,
        int32& OutMaxResults,
        FString& OutError
    ) const;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON error response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};