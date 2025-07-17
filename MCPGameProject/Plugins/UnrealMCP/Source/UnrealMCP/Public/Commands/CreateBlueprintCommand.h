#pragma once

#include "CoreMinimal.h"
#include "IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command for creating new Blueprint assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FCreateBlueprintCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FCreateBlueprintCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters into blueprint creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutParams - Parsed parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FBlueprintCreationParams& OutParams, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Blueprint - Created blueprint
     * @return JSON response string
     */
    FString CreateSuccessResponse(UBlueprint* Blueprint) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};