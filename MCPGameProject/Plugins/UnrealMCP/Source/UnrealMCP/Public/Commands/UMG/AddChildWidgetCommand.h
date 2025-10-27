#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for adding widget components as children to parent components in UMG Widget Blueprints
 * Implements the new command pattern architecture with service layer delegation
 * while maintaining compatibility with the existing string-based interface
 */
class UNREALMCP_API FAddChildWidgetCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FAddChildWidgetCommand(TSharedPtr<IUMGService> InUMGService);

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
     * @param BlueprintName - Name of the blueprint
     * @param ParentComponentName - Name of the parent component
     * @param ChildComponentName - Name of the child component
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& BlueprintName, const FString& ParentComponentName, const FString& ChildComponentName) const;
    
    /**
     * Create error response JSON object from MCP error
     * @param Error - MCP error to convert to response
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};