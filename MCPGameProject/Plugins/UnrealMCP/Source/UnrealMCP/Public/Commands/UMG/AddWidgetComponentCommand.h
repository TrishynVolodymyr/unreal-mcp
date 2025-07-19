#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Json.h"

// Forward declarations
class IUMGService;
class UWidget;

/**
 * Command for adding widget components to UMG Widget Blueprints
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FAddWidgetComponentCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Reference to the UMG service for operations
     */
    explicit FAddWidgetComponentCommand(IUMGService& InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the UMG service */
    IUMGService& UMGService;
    
    /**
     * Parse JSON parameters into widget component creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutComponentName - Parsed component name
     * @param OutComponentType - Parsed component type
     * @param OutPosition - Parsed position
     * @param OutSize - Parsed size
     * @param OutKwargs - Parsed additional parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName, 
                        FString& OutComponentType, FVector2D& OutPosition, FVector2D& OutSize, 
                        TSharedPtr<FJsonObject>& OutKwargs, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Widget - Created widget component
     * @param ComponentName - Name of the created component
     * @param ComponentType - Type of the created component
     * @return JSON response string
     */
    FString CreateSuccessResponse(UWidget* Widget, const FString& ComponentName, const FString& ComponentType) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
