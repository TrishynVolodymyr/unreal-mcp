#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintNodeService.h"

/**
 * Command to get all graphs in a Blueprint
 */
class UNREALMCP_API FGetBlueprintGraphsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintNodeService - Service for Blueprint node operations
     */
    explicit FGetBlueprintGraphsCommand(IBlueprintNodeService& InBlueprintNodeService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Service for Blueprint node operations */
    IBlueprintNodeService& BlueprintNodeService;

    /**
     * Parse command parameters
     * @param JsonString - JSON parameter string
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutError) const;

    /**
     * Create success response with graph list
     * @param GraphNames - Array of graph names
     * @return JSON response string
     */
    FString CreateSuccessResponse(const TArray<FString>& GraphNames) const;

    /**
     * Create error response
     * @param ErrorMessage - Error message
     * @return JSON error response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};