#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command to list all components in a Blueprint class
 */
class UNREALMCP_API FListBlueprintComponentsCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service
     */
    explicit FListBlueprintComponentsCommand(IBlueprintService& InBlueprintService);

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
     * @param OutError - Error message if parsing fails
     * @return True if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutError) const;

    /**
     * Create success response JSON
     * @param BlueprintName - Name of the blueprint
     * @param Components - List of component information
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const TArray<TPair<FString, FString>>& Components) const;

    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};
