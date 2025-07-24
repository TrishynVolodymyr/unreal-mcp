#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command for adding input action nodes to Blueprints
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FAddBlueprintInputActionNodeCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintNodeService - Reference to the Blueprint node service for operations
     */
    explicit FAddBlueprintInputActionNodeCommand(IBlueprintNodeService& InBlueprintNodeService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the Blueprint node service */
    IBlueprintNodeService& BlueprintNodeService;
    
    /**
     * Parse JSON parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed Blueprint name
     * @param OutActionName - Parsed action name
     * @param OutPosition - Parsed node position
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutActionName, FVector2D& OutPosition, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param NodeId - Created node ID
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& NodeId) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};