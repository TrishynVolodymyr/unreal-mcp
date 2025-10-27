#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for setting up property bindings for Text Block widgets
 * Implements the new command pattern architecture with service layer delegation
 * while maintaining compatibility with the existing string-based interface
 */
class UNREALMCP_API FSetTextBlockBindingCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FSetTextBlockBindingCommand(TSharedPtr<IUMGService> InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Shared pointer to the UMG service */
    TSharedPtr<IUMGService> UMGService;
    
    /**
     * Internal execution with JSON objects (new architecture)
     * @param Params - JSON parameters
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);
    
    /**
     * Internal validation with JSON objects (new architecture)
     * @param Params - JSON parameters
     * @param OutError - Error message if validation fails
     * @return true if validation passes
     */
    bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;
    
    /**
     * Create success response JSON object
     * @param TextBlockName - Name of the text block with binding
     * @param BindingProperty - Name of the binding property
     * @param VariableType - Type of the binding variable
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& TextBlockName, const FString& BindingProperty, const FString& VariableType) const;
    
    /**
     * Create error response JSON object from MCP error
     * @param Error - MCP error to convert to response
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};