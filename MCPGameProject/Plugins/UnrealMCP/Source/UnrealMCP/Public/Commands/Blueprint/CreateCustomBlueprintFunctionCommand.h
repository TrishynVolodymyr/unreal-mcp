#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Command for creating custom user-defined functions in Blueprint classes
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FCreateCustomBlueprintFunctionCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FCreateCustomBlueprintFunctionCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
public:
    /** Structure to hold function parameter information */
    struct FFunctionParameter
    {
        FString Name;
        FString Type;
    };
    
private:
    
    /**
     * Parse JSON parameters into function creation parameters
     * @param JsonString - JSON string containing parameters
     * @param OutBlueprintName - Parsed blueprint name
     * @param OutFunctionName - Parsed function name
     * @param OutInputs - Parsed input parameters
     * @param OutOutputs - Parsed output parameters
     * @param OutIsPure - Whether function is pure
     * @param OutIsConst - Whether function is const
     * @param OutAccessSpecifier - Function access level
     * @param OutCategory - Function category
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, 
                        FString& OutBlueprintName,
                        FString& OutFunctionName,
                        TArray<FFunctionParameter>& OutInputs,
                        TArray<FFunctionParameter>& OutOutputs,
                        bool& OutIsPure,
                        bool& OutIsConst,
                        FString& OutAccessSpecifier,
                        FString& OutCategory,
                        FString& OutError) const;
    
    /**
     * Parse parameter array from JSON
     * @param JsonArray - JSON array containing parameter objects
     * @param OutParameters - Parsed parameters
     * @return true if parsing succeeded
     */
    bool ParseParameterArray(const TArray<TSharedPtr<FJsonValue>>& JsonArray, TArray<FFunctionParameter>& OutParameters) const;
    
    /**
     * Create success response JSON
     * @param BlueprintName - Blueprint name
     * @param FunctionName - Created function name
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& BlueprintName, const FString& FunctionName) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};