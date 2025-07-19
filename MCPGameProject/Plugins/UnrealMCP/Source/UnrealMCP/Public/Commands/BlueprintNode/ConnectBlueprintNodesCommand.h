#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command for connecting Blueprint nodes
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FConnectBlueprintNodesCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintNodeService - Reference to the Blueprint node service for operations
     */
    explicit FConnectBlueprintNodesCommand(IBlueprintNodeService& InBlueprintNodeService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the Blueprint node service */
    IBlueprintNodeService& BlueprintNodeService;
    
    /**
     * Parse JSON parameters into connection parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed Blueprint name
     * @param OutConnections - Parsed connection parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FBlueprintNodeConnectionParams>& OutConnections, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Results - Connection results
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<bool>& Results) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};