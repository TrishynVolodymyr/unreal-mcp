#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for setting the design size mode of a Widget Blueprint
 * Supports modes: DesiredOnScreen, Custom, FillScreen, CustomOnScreen
 */
class UNREALMCP_API FSetWidgetDesignSizeCommand : public IUnrealMCPCommand
{
public:
    /**
     * Constructor
     * @param InUMGService - Shared pointer to the UMG service for operations
     */
    explicit FSetWidgetDesignSizeCommand(TSharedPtr<IUMGService> InUMGService);

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
    TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& WidgetName, const FString& DesignSizeMode,
                                                   int32 Width, int32 Height) const;

    /**
     * Create error response JSON object from MCP error
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
