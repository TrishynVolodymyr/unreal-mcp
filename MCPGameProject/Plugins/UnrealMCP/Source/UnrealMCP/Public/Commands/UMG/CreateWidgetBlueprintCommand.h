#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Json.h"

// Forward declarations
class IUMGService;
class UWidgetBlueprint;

/**
 * Command for creating new UMG Widget Blueprint assets
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FCreateWidgetBlueprintCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Reference to the UMG service for operations
     */
    explicit FCreateWidgetBlueprintCommand(IUMGService& InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the UMG service */
    IUMGService& UMGService;
    
    /**
     * Parse JSON parameters into widget blueprint creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutName - Parsed widget name
     * @param OutParentClass - Parsed parent class name
     * @param OutPath - Parsed creation path
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutName, FString& OutParentClass, FString& OutPath, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param WidgetBlueprint - Created widget blueprint
     * @param Path - Path where the blueprint was created
     * @param bAlreadyExists - Whether the blueprint already existed
     * @return JSON response string
     */
    FString CreateSuccessResponse(UWidgetBlueprint* WidgetBlueprint, const FString& Path, bool bAlreadyExists = false) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
