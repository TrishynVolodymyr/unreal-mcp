#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for changing the parent class of a Widget Blueprint
 * Allows reparenting widgets to custom C++ base classes
 */
class UNREALMCP_API FSetWidgetParentClassCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FSetWidgetParentClassCommand(TSharedPtr<IUMGService> InUMGService);

    // IUnrealMCPCommand interface
    virtual FString Execute(const FString& Parameters) override;
    virtual FString GetCommandName() const override;
    virtual bool ValidateParams(const FString& Parameters) const override;

private:
    /** Shared pointer to the UMG service */
    TSharedPtr<IUMGService> UMGService;

    /**
     * Internal execution with JSON objects
     * @param Params - JSON parameters
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params);

    /**
     * Internal validation with JSON objects
     * @param Params - JSON parameters
     * @param OutError - Error message if validation fails
     * @return true if validation passes
     */
    bool ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const;

    /**
     * Create success response JSON object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& WidgetName, const FString& NewParentClass,
                                                   const FString& OldParentClass) const;

    /**
     * Create error response JSON object from MCP error
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
