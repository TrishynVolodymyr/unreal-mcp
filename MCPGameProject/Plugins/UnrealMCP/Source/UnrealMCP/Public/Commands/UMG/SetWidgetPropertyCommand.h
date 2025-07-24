#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Json.h"

// Forward declarations
class IUMGService;

/**
 * Command for setting properties on UMG widget components
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FSetWidgetPropertyCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Reference to the UMG service for operations
     */
    explicit FSetWidgetPropertyCommand(IUMGService& InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the UMG service */
    IUMGService& UMGService;
    
    /**
     * Parse JSON parameters into widget property setting parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutComponentName - Parsed component name
     * @param OutProperties - Parsed properties to set
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName, 
                        TSharedPtr<FJsonObject>& OutProperties, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param ComponentName - Name of the modified component
     * @param SuccessProperties - List of successfully set properties
     * @param FailedProperties - List of properties that failed to set
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& ComponentName, const TArray<FString>& SuccessProperties, 
                                 const TArray<FString>& FailedProperties) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
