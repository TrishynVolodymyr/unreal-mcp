#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to add variable definitions to a Blueprint
 */
class UNREALMCP_API FAddBlueprintVariableCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service
     */
    explicit FAddBlueprintVariableCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;

    /**
     * Create success response JSON
     * @param BlueprintName - Name of the blueprint
     * @param VariableName - Name of the variable
     * @param VariableType - Type of the variable
     * @param bIsExposed - Whether the variable is exposed
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& VariableName, 
                                 const FString& VariableType, bool bIsExposed) const;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};