#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to set physics properties on a component in a Blueprint
 */
class UNREALMCP_API FSetPhysicsPropertiesCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service
     */
    explicit FSetPhysicsPropertiesCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;

    /**
     * Parse JSON parameters into structured data
     * @param JsonString - JSON string to parse
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutComponentName - Parsed component name
     * @param OutPhysicsParams - Parsed physics parameters
     * @param OutError - Error message if parsing fails
     * @return True if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName,
                        TMap<FString, float>& OutPhysicsParams, FString& OutError) const;

    /**
     * Create success response JSON
     * @param BlueprintName - Name of the blueprint
     * @param ComponentName - Name of the component
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName) const;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
