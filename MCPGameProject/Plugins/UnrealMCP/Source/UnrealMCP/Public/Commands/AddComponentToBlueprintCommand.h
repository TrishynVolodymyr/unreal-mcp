#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"
#include "Commands/ComponentCreationParams.h"

/**
 * Command for adding components to existing Blueprint assets
 * Implements the IUnrealMCPCommand interface with parameter validation
 */
class UNREALMCP_API FAddComponentToBlueprintCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FAddComponentToBlueprintCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters into component creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Name of the target blueprint
     * @param OutParams - Parsed component parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                        FComponentCreationParams& OutParams, FString& OutError) const;
    
    /**
     * Validate component type is supported
     * @param ComponentType - Type of component to validate
     * @return true if component type is supported
     */
    bool IsValidComponentType(const FString& ComponentType) const;
    
    /**
     * Create success response JSON
     * @param BlueprintName - Name of the blueprint
     * @param ComponentName - Name of the added component
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
