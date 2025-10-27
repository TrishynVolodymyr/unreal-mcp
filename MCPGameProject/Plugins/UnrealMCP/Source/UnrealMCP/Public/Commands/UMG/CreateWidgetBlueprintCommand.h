#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
class UWidgetBlueprint;
struct FMCPError;

/**
 * Command for creating new UMG Widget Blueprint assets
 * Implements the new command pattern architecture with service layer delegation
 * while maintaining compatibility with the existing string-based interface
 */
class UNREALMCP_API FCreateWidgetBlueprintCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FCreateWidgetBlueprintCommand(TSharedPtr<IUMGService> InUMGService);

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
     * @param WidgetBlueprint - Created widget blueprint
     * @param Path - Path where the blueprint was created
     * @param bAlreadyExists - Whether the blueprint already existed
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(UWidgetBlueprint* WidgetBlueprint, const FString& Path, bool bAlreadyExists = false) const;
    
    /**
     * Create error response JSON object from MCP error
     * @param Error - MCP error to convert to response
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
