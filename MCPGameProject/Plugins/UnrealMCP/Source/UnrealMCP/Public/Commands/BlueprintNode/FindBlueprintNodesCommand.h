#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command for finding nodes in Blueprint graphs
 * Implements the IUnrealMCPCommand interface for standardized command execution
 * Uses Service Layer pattern for business logic delegation
 */
class UNREALMCP_API FFindBlueprintNodesCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor - Uses service layer pattern
     * @param InBlueprintNodeService - Reference to the Blueprint node service for operations
     */
    explicit FFindBlueprintNodesCommand(IBlueprintNodeService& InBlueprintNodeService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the Blueprint node service - Service Layer Pattern */
    IBlueprintNodeService& BlueprintNodeService;
    
    /**
     * Parse JSON parameters with proper validation
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed Blueprint name
     * @param OutNodeType - Parsed node type filter
     * @param OutEventType - Parsed event type filter
     * @param OutTargetGraph - Parsed target graph name
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutNodeType, FString& OutEventType, FString& OutTargetGraph, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param NodeInfos - Found node information (ID and title)
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<FBlueprintNodeInfo>& NodeInfos) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};