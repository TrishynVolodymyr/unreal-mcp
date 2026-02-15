#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Parameter structure for widget placement operations
 * Follows structured parameter extraction pattern from new architecture
 */
struct FWidgetPlacementParams
{
    FString WidgetName;
    FString ComponentName;
    FVector2D* Position = nullptr;
    FVector2D* Size = nullptr;
    FVector2D* Alignment = nullptr;
    FVector2D* AnchorMin = nullptr;
    FVector2D* AnchorMax = nullptr;
    bool bAutoSize = false;
    bool bHasAutoSize = false;
};

/**
 * Command for setting the placement (position/size/alignment) of a widget component in a UMG Widget Blueprint
 * Implements the new command pattern architecture with service layer delegation
 * while maintaining compatibility with the existing string-based interface
 */
class UNREALMCP_API FSetWidgetPlacementCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FSetWidgetPlacementCommand(TSharedPtr<IUMGService> InUMGService);

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
     * Extract placement parameters from JSON object
     * @param Params - JSON parameters object
     * @param OutParams - Output parameter structure
     * @return true if extraction was successful
     */
    bool ExtractPlacementParameters(const TSharedPtr<FJsonObject>& Params, FWidgetPlacementParams& OutParams) const;
    
    /**
     * Extract Vector2D parameter from JSON object
     * @param Params - JSON parameters object
     * @param ParameterName - Name of the parameter to extract
     * @return Pointer to extracted vector or nullptr if not found/invalid
     */
    FVector2D* ExtractVector2DParameter(const TSharedPtr<FJsonObject>& Params, const FString& ParameterName) const;
    
    // Response Creation (following structured error handling pattern)
    
    /**
     * Create success response JSON object from parameters
     * @param Params - Widget placement parameters that were applied
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FWidgetPlacementParams& Params) const;
    
    /**
     * Create error response JSON object from MCP error
     * @param Error - MCP error to convert to response
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
    
    /**
     * Parse vector2D from JSON array
     * @param JsonArray - JSON array with 2 elements
     * @param OutVector - Output vector
     * @return true if parsing was successful
     */
    bool ParseVector2DFromJson(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector2D& OutVector) const;

    /** Storage for extracted vectors (avoids static variable reuse bug) */
    mutable TArray<FVector2D> ExtractedVectorStorage;
};