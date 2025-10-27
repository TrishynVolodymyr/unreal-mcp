#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Parameter structure for widget component layout operations
 * Follows structured parameter extraction pattern from new architecture
 */
struct FWidgetComponentLayoutParams
{
    FString WidgetName;
};

/**
 * Command for getting hierarchical layout information for all components within a UMG Widget Blueprint
 * Implements the new command pattern architecture with service layer delegation
 * while maintaining compatibility with the existing string-based interface
 */
class UNREALMCP_API FGetWidgetComponentLayoutCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FGetWidgetComponentLayoutCommand(TSharedPtr<IUMGService> InUMGService);

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
    
    // JSON Utility Methods (following centralized JSON utilities pattern)
    
    /**
     * Parse JSON parameters from string
     * @param Parameters - JSON string to parse
     * @return Parsed JSON object or nullptr if parsing failed
     */
    TSharedPtr<FJsonObject> ParseJsonParameters(const FString& Parameters) const;
    
    /**
     * Serialize JSON response to string
     * @param Response - JSON response object
     * @return Serialized JSON string
     */
    FString SerializeJsonResponse(const TSharedPtr<FJsonObject>& Response) const;
    
    /**
     * Serialize error response to string
     * @param Error - MCP error to serialize
     * @return Serialized error response string
     */
    FString SerializeErrorResponse(const FMCPError& Error) const;
    
    // Parameter Extraction (following structured parameter extraction pattern)
    
    /**
     * Extract widget component layout parameters from JSON object
     * @param Params - JSON parameters object
     * @param OutParams - Output parameter structure
     * @return true if extraction was successful
     */
    bool ExtractWidgetComponentLayoutParameters(const TSharedPtr<FJsonObject>& Params, FWidgetComponentLayoutParams& OutParams) const;
    
    // Response Creation (following structured error handling pattern)
    
    /**
     * Create success response JSON object from layout information
     * @param Params - Widget component layout parameters that were queried
     * @param LayoutInfo - The retrieved layout information
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FWidgetComponentLayoutParams& Params, const TSharedPtr<FJsonObject>& LayoutInfo) const;
    
    /**
     * Create error response JSON object from MCP error
     * @param Error - MCP error to convert to response
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};