#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to set properties on a Blueprint class default object
 */
class UNREALMCP_API FSetBlueprintPropertyCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service
     */
    explicit FSetBlueprintPropertyCommand(IBlueprintService& InBlueprintService);

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
     * @param OutPropertyName - Parsed property name
     * @param OutPropertyValue - Parsed property value
     * @param OutError - Error message if parsing fails
     * @return True if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutPropertyName,
                        TSharedPtr<FJsonValue>& OutPropertyValue, FString& OutError) const;

    /**
     * Create success response JSON
     * @param BlueprintName - Name of the blueprint
     * @param PropertyName - Name of the property
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& PropertyName) const;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
