#include "Commands/UMG/CaptureWidgetScreenshotCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogCaptureWidgetScreenshotCommand, Log, All);

FCaptureWidgetScreenshotCommand::FCaptureWidgetScreenshotCommand(TSharedPtr<IUMGService> InUMGService)
	: UMGService(InUMGService)
{
}

FString FCaptureWidgetScreenshotCommand::Execute(const FString& Parameters)
{
	UE_LOG(LogCaptureWidgetScreenshotCommand, Log, TEXT("CaptureWidgetScreenshotCommand::Execute - Command execution started"));
	UE_LOG(LogCaptureWidgetScreenshotCommand, Verbose, TEXT("Parameters: %s"), *Parameters);

	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject = ParseJsonParameters(Parameters);
	if (!JsonObject.IsValid())
	{
		FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
		return SerializeErrorResponse(Error);
	}

	// Validate parameters
	FString ValidationError;
	if (!ValidateParamsInternal(JsonObject, ValidationError))
	{
		FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
		return SerializeErrorResponse(Error);
	}

	// Execute command using service layer delegation
	TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);

	// Serialize response
	return SerializeJsonResponse(Response);
}

TSharedPtr<FJsonObject> FCaptureWidgetScreenshotCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
	// Validate service availability
	if (!UMGService.IsValid())
	{
		UE_LOG(LogCaptureWidgetScreenshotCommand, Error, TEXT("UMG service is not available - dependency injection failed"));
		FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
		return CreateErrorResponse(Error);
	}

	// Extract and validate parameters
	FWidgetScreenshotParams ScreenshotParams;
	if (!ExtractWidgetScreenshotParameters(Params, ScreenshotParams))
	{
		FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Failed to extract widget screenshot parameters"));
		return CreateErrorResponse(Error);
	}

	UE_LOG(LogCaptureWidgetScreenshotCommand, Log, TEXT("Capturing screenshot for widget '%s' at %dx%d"),
		*ScreenshotParams.WidgetName, ScreenshotParams.Width, ScreenshotParams.Height);

	// Delegate to service layer
	TSharedPtr<FJsonObject> ScreenshotData;
	bool bSuccess = UMGService->CaptureWidgetScreenshot(
		ScreenshotParams.WidgetName,
		ScreenshotParams.Width,
		ScreenshotParams.Height,
		ScreenshotParams.Format,
		ScreenshotData);

	if (!bSuccess || !ScreenshotData.IsValid())
	{
		UE_LOG(LogCaptureWidgetScreenshotCommand, Warning, TEXT("Service layer failed to capture widget screenshot"));
		FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
			FString::Printf(TEXT("Failed to capture screenshot for widget '%s'"),
				*ScreenshotParams.WidgetName));
		return CreateErrorResponse(Error);
	}

	UE_LOG(LogCaptureWidgetScreenshotCommand, Log, TEXT("Widget screenshot captured successfully"));
	return CreateSuccessResponse(ScreenshotParams, ScreenshotData);
}

FString FCaptureWidgetScreenshotCommand::GetCommandName() const
{
	return TEXT("capture_widget_screenshot");
}

bool FCaptureWidgetScreenshotCommand::ValidateParams(const FString& Parameters) const
{
	// Parse JSON for validation
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString ValidationError;
	return ValidateParamsInternal(JsonObject, ValidationError);
}

bool FCaptureWidgetScreenshotCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
	if (!Params.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	// Check for widget_name (required)
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing required parameter: widget_name");
		return false;
	}

	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	if (WidgetName.IsEmpty())
	{
		OutError = TEXT("widget_name cannot be empty");
		return false;
	}

	// Validate width if provided
	if (Params->HasField(TEXT("width")))
	{
		int32 Width = (int32)Params->GetNumberField(TEXT("width"));
		if (Width <= 0 || Width > 8192)
		{
			OutError = TEXT("width must be between 1 and 8192");
			return false;
		}
	}

	// Validate height if provided
	if (Params->HasField(TEXT("height")))
	{
		int32 Height = (int32)Params->GetNumberField(TEXT("height"));
		if (Height <= 0 || Height > 8192)
		{
			OutError = TEXT("height must be between 1 and 8192");
			return false;
		}
	}

	// Validate format if provided
	if (Params->HasField(TEXT("format")))
	{
		FString Format = Params->GetStringField(TEXT("format"));
		if (Format != TEXT("png") && Format != TEXT("jpg") && Format != TEXT("jpeg"))
		{
			OutError = TEXT("format must be 'png', 'jpg', or 'jpeg'");
			return false;
		}
	}

	return true;
}

// JSON Utility Methods
TSharedPtr<FJsonObject> FCaptureWidgetScreenshotCommand::ParseJsonParameters(const FString& Parameters) const
{
	if (Parameters.IsEmpty())
	{
		UE_LOG(LogCaptureWidgetScreenshotCommand, Warning, TEXT("Empty parameters provided"));
		return nullptr;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogCaptureWidgetScreenshotCommand, Error, TEXT("Failed to parse JSON parameters: %s"), *Parameters);
		return nullptr;
	}

	return JsonObject;
}

FString FCaptureWidgetScreenshotCommand::SerializeJsonResponse(const TSharedPtr<FJsonObject>& Response) const
{
	if (!Response.IsValid())
	{
		UE_LOG(LogCaptureWidgetScreenshotCommand, Error, TEXT("Invalid response object for serialization"));
		return TEXT("{}");
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

FString FCaptureWidgetScreenshotCommand::SerializeErrorResponse(const FMCPError& Error) const
{
	TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);
	return SerializeJsonResponse(ErrorResponse);
}

// Parameter Extraction
bool FCaptureWidgetScreenshotCommand::ExtractWidgetScreenshotParameters(const TSharedPtr<FJsonObject>& Params, FWidgetScreenshotParams& OutParams) const
{
	// Extract widget name (required)
	if (!Params->HasField(TEXT("widget_name")))
	{
		UE_LOG(LogCaptureWidgetScreenshotCommand, Error, TEXT("Missing widget_name parameter"));
		return false;
	}
	OutParams.WidgetName = Params->GetStringField(TEXT("widget_name"));

	// Extract width (optional, default 800)
	OutParams.Width = Params->HasField(TEXT("width")) ? (int32)Params->GetNumberField(TEXT("width")) : 800;

	// Extract height (optional, default 600)
	OutParams.Height = Params->HasField(TEXT("height")) ? (int32)Params->GetNumberField(TEXT("height")) : 600;

	// Extract format (optional, default "png")
	OutParams.Format = Params->HasField(TEXT("format")) ? Params->GetStringField(TEXT("format")) : TEXT("png");

	return true;
}

// Response Creation
TSharedPtr<FJsonObject> FCaptureWidgetScreenshotCommand::CreateSuccessResponse(const FWidgetScreenshotParams& Params, const TSharedPtr<FJsonObject>& ScreenshotData) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);

	// Copy all fields from the screenshot data to the response
	if (ScreenshotData.IsValid())
	{
		for (auto& Pair : ScreenshotData->Values)
		{
			ResponseObj->SetField(Pair.Key, Pair.Value);
		}
	}

	// Ensure success is set to true
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("widget_name"), Params.WidgetName);

	if (!ResponseObj->HasField(TEXT("message")))
	{
		ResponseObj->SetStringField(TEXT("message"),
			FString::Printf(TEXT("Successfully captured screenshot for widget '%s' at %dx%d"),
				*Params.WidgetName, Params.Width, Params.Height));
	}

	return ResponseObj;
}

TSharedPtr<FJsonObject> FCaptureWidgetScreenshotCommand::CreateErrorResponse(const FMCPError& Error) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
	ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to capture widget screenshot: %s"), *Error.ErrorMessage));

	return ResponseObj;
}
