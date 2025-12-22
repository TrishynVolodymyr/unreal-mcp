#include "Commands/UMG/RemoveWidgetFunctionGraphCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FRemoveWidgetFunctionGraphCommand::FRemoveWidgetFunctionGraphCommand(TSharedPtr<IUMGService> InUMGService)
	: UMGService(InUMGService)
{
}

FString FRemoveWidgetFunctionGraphCommand::Execute(const FString& Parameters)
{
	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Use internal execution with JSON objects
	TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);

	// Convert response back to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

FString FRemoveWidgetFunctionGraphCommand::GetCommandName() const
{
	return TEXT("remove_widget_function_graph");
}

bool FRemoveWidgetFunctionGraphCommand::ValidateParams(const FString& Parameters) const
{
	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString ValidationError;
	return ValidateParamsInternal(JsonObject, ValidationError);
}

TSharedPtr<FJsonObject> FRemoveWidgetFunctionGraphCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
	if (!UMGService.IsValid())
	{
		FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
		return CreateErrorResponse(Error);
	}

	// Validate parameters
	FString ValidationError;
	if (!ValidateParamsInternal(Params, ValidationError))
	{
		FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
		return CreateErrorResponse(Error);
	}

	// Extract parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString FunctionName = Params->GetStringField(TEXT("function_name"));

	// Call the UMG service to remove the function graph
	bool bResult = UMGService->RemoveWidgetFunctionGraph(WidgetName, FunctionName);

	if (!bResult)
	{
		FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
			FString::Printf(TEXT("Failed to remove function graph '%s' from widget '%s'"),
				*FunctionName, *WidgetName));
		return CreateErrorResponse(Error);
	}

	return CreateSuccessResponse(WidgetName, FunctionName);
}

bool FRemoveWidgetFunctionGraphCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
	if (!Params.IsValid())
	{
		OutError = TEXT("Invalid parameters object");
		return false;
	}

	// Check required parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	if (WidgetName.IsEmpty())
	{
		OutError = TEXT("Missing or empty 'widget_name' parameter");
		return false;
	}

	FString FunctionName = Params->GetStringField(TEXT("function_name"));
	if (FunctionName.IsEmpty())
	{
		OutError = TEXT("Missing or empty 'function_name' parameter");
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FRemoveWidgetFunctionGraphCommand::CreateSuccessResponse(
	const FString& WidgetName,
	const FString& FunctionName) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResponseObj->SetStringField(TEXT("function_name"), FunctionName);
	ResponseObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Successfully removed function graph '%s' from widget '%s'"),
			*FunctionName, *WidgetName));

	return ResponseObj;
}

TSharedPtr<FJsonObject> FRemoveWidgetFunctionGraphCommand::CreateErrorResponse(const FMCPError& Error) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
	ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
	ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
	ResponseObj->SetNumberField(TEXT("error_type"), static_cast<int32>(Error.ErrorType));

	return ResponseObj;
}
