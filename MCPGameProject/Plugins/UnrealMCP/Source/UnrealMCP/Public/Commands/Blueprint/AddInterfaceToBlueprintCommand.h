#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command for adding interfaces to Blueprint assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FAddInterfaceToBlueprintCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FAddInterfaceToBlueprintCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters into interface addition parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutInterfaceName - Parsed interface name
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutInterfaceName, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param BlueprintName - Blueprint name
     * @param InterfaceName - Interface name
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& InterfaceName) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};