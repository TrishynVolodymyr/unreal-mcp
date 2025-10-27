#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintActionService.h"

/**
 * Command to search for Blueprint actions using keywords
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FSearchBlueprintActionsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintActionService - Service for blueprint action operations
     */
    explicit FSearchBlueprintActionsCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService);
    
    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Service for blueprint action operations */
    TSharedPtr<IBlueprintActionService> BlueprintActionService;
    
    /**
     * Parse parameters from JSON string
     * @param Parameters - JSON string containing parameters
     * @param OutSearchQuery - Parsed search query
     * @param OutCategory - Parsed category filter
     * @param OutMaxResults - Parsed max results
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(
        const FString& Parameters,
        FString& OutSearchQuery,
        FString& OutCategory,
        int32& OutMaxResults,
        FString& OutBlueprintName,
        FString& OutError
    ) const;
    
    /**
     * Create an error response JSON string
     * @param ErrorMessage - Error message
     * @return JSON string representing error response
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};