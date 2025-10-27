#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintActionService.h"

/**
 * Command to get all available Blueprint actions for a specific pin type
 */
class UNREALMCP_API FGetActionsForPinCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintActionService - Service for blueprint action operations
     */
    explicit FGetActionsForPinCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService);
    
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
     * @param OutPinType - Parsed pin type
     * @param OutPinSubcategory - Parsed pin subcategory
     * @param OutSearchFilter - Parsed search filter
     * @param OutMaxResults - Parsed max results
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(
        const FString& Parameters,
        FString& OutPinType,
        FString& OutPinSubcategory,
        FString& OutSearchFilter,
        int32& OutMaxResults,
        FString& OutError
    ) const;
    
    /**
     * Create an error response JSON string
     * @param ErrorMessage - Error message
     * @return JSON string representing error response
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};