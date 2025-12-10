#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Dom/JsonObject.h"

// Forward declarations
class IUMGService;
struct FMCPError;

/**
 * Parameter structure for widget screenshot capture operations
 */
struct FWidgetScreenshotParams
{
	FString WidgetName;
	int32 Width;
	int32 Height;
	FString Format;  // "png" or "jpg"
};

/**
 * Command for capturing a screenshot of a UMG Widget Blueprint preview
 * Renders the widget to a texture and returns as base64-encoded image data
 * This allows AI to visually inspect the widget layout
 */
class UNREALMCP_API FCaptureWidgetScreenshotCommand : public IUnrealMCPCommand
{
public:
	/**
	 * Constructor
	 * @param InUMGService - Shared pointer to the UMG service for operations
	 */
	explicit FCaptureWidgetScreenshotCommand(TSharedPtr<IUMGService> InUMGService);

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

	// JSON Utility Methods

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

	// Parameter Extraction

	/**
	 * Extract widget screenshot parameters from JSON object
	 * @param Params - JSON parameters object
	 * @param OutParams - Output parameter structure
	 * @return true if extraction was successful
	 */
	bool ExtractWidgetScreenshotParameters(const TSharedPtr<FJsonObject>& Params, FWidgetScreenshotParams& OutParams) const;

	// Response Creation

	/**
	 * Create success response JSON object from screenshot data
	 * @param Params - Widget screenshot parameters that were used
	 * @param ScreenshotData - The captured screenshot data (base64-encoded image)
	 * @return JSON response object
	 */
	TSharedPtr<FJsonObject> CreateSuccessResponse(const FWidgetScreenshotParams& Params, const TSharedPtr<FJsonObject>& ScreenshotData) const;

	/**
	 * Create error response JSON object from MCP error
	 * @param Error - MCP error to convert to response
	 * @return JSON response object
	 */
	TSharedPtr<FJsonObject> CreateErrorResponse(const FMCPError& Error) const;
};
