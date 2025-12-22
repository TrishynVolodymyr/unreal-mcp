#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Command for removing function graphs from Widget Blueprints.
 *
 * This command removes function graphs (override functions, custom events, etc.)
 * from Widget Blueprints. Useful for:
 * - Cleaning up broken/corrupt function graphs that prevent compilation
 * - Removing unwanted override functions
 * - Resetting widget event handlers
 *
 * Example usage:
 * {
 *   "widget_name": "WBP_InventorySlot",
 *   "function_name": "OnMouseButtonDown"
 * }
 */
class UNREALMCP_API FRemoveWidgetFunctionGraphCommand : public IUnrealMCPCommand
{
public:
	/**
	 * Constructor
	 * @param InUMGService - Shared pointer to the UMG service for operations
	 */
	explicit FRemoveWidgetFunctionGraphCommand(TSharedPtr<IUMGService> InUMGService);

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
	TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& WidgetName, const FString& FunctionName) const;

	/**
	 * Create error response JSON object from MCP error
	 */
	TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
