#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Json.h"

// Forward declarations
class IUMGService;

/**
 * Command for binding events to UMG widget components
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FBindWidgetEventCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Reference to the UMG service for operations
     */
    explicit FBindWidgetEventCommand(IUMGService& InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the UMG service */
    IUMGService& UMGService;
    
    /**
     * Parse JSON parameters into widget event binding parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutComponentName - Parsed component name
     * @param OutEventName - Parsed event name
     * @param OutFunctionName - Parsed function name (optional)
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName, 
                        FString& OutEventName, FString& OutFunctionName, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param ComponentName - Name of the component with bound event
     * @param EventName - Name of the bound event
     * @param FunctionName - Name of the created/bound function
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& ComponentName, const FString& EventName, const FString& FunctionName) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
