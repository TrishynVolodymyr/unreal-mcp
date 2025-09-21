#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command for connecting Blueprint nodes
 * Implements the IUnrealMCPCommand interface for standardized command execution
 * Follows the new architecture with proper service layer delegation
 */
class UNREALMCP_API FConnectBlueprintNodesCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintNodeService - Shared pointer to the Blueprint node service for operations
     */
    explicit FConnectBlueprintNodesCommand(TSharedPtr<IBlueprintNodeService> InBlueprintNodeService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Shared pointer to the Blueprint node service */
    TSharedPtr<IBlueprintNodeService> BlueprintNodeService;
    
    /**
     * Parse JSON parameters into connection parameters
     * @param JsonObject - JSON object containing parameters
     * @param OutBlueprintName - Parsed Blueprint name
     * @param OutConnections - Parsed connection parameters
     * @param OutTargetGraph - Parsed target graph name (optional)
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const TSharedPtr<FJsonObject>& JsonObject, FString& OutBlueprintName, TArray<FBlueprintNodeConnectionParams>& OutConnections, FString& OutTargetGraph, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Results - Connection results
     * @param Connections - Original connection parameters for detailed response
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<bool>& Results, const TArray<FBlueprintNodeConnectionParams>& Connections) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};