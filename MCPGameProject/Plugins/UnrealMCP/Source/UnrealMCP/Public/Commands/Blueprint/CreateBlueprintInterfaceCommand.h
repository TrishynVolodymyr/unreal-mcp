#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command for creating new Blueprint Interface assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FCreateBlueprintInterfaceCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FCreateBlueprintInterfaceCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters into interface creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutName - Parsed interface name
     * @param OutFolderPath - Parsed folder path
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutName, FString& OutFolderPath, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param InterfaceName - Created interface name
     * @param InterfacePath - Created interface path
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& InterfaceName, const FString& InterfacePath) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};