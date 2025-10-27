#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintService.h"

/**
 * Parameters for calling a Blueprint function
 */
struct UNREALMCP_API FBlueprintFunctionCallParams
{
    /** Name of the target object to call the function on */
    FString TargetName;
    
    /** Name of the BlueprintCallable function to execute */
    FString FunctionName;
    
    /** List of string parameters to pass to the function */
    TArray<FString> StringParams;
    
    /**
     * Validate the parameters
     * @param OutError - Error message if validation fails
     * @return true if parameters are valid
     */
    bool IsValid(FString& OutError) const
    {
        if (TargetName.IsEmpty())
        {
            OutError = TEXT("target_name is required");
            return false;
        }
        
        if (FunctionName.IsEmpty())
        {
            OutError = TEXT("function_name is required");
            return false;
        }
        
        return true;
    }
};

/**
 * Command for calling BlueprintCallable functions by name
 * Implements the IUnrealMCPCommand interface for standardized command execution
 */
class UNREALMCP_API FCallBlueprintFunctionCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InBlueprintService - Reference to the blueprint service for operations
     */
    explicit FCallBlueprintFunctionCommand(IBlueprintService& InBlueprintService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Reference to the blueprint service */
    IBlueprintService& BlueprintService;
    
    /**
     * Parse JSON parameters into function call parameters
     * @param JsonString - JSON string containing parameters
     * @param OutParams - Parsed parameters
     * @param OutError - Error message if parsing fails
     * @return true if parsing succeeded
     */
    bool ParseParameters(const FString& JsonString, FBlueprintFunctionCallParams& OutParams, FString& OutError) const;
    
    /**
     * Create success response JSON
     * @param Result - Function call result
     * @return JSON response string
     */
    FString CreateSuccessResponse(const FString& Result) const;
    
    /**
     * Create error response JSON
     * @param ErrorMessage - Error message
     * @return JSON response string
     */
    FString CreateErrorResponse(const FString& ErrorMessage) const;
};