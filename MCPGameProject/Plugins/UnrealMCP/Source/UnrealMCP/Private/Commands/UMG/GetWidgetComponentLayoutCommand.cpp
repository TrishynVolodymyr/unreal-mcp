#include "Commands/UMG/GetWidgetComponentLayoutCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogGetWidgetComponentLayoutCommand, Log, All);

FGetWidgetComponentLayoutCommand::FGetWidgetComponentLayoutCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FGetWidgetComponentLayoutCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogGetWidgetComponentLayoutCommand, Log, TEXT("GetWidgetComponentLayoutCommand::Execute - Command execution started"));
    UE_LOG(LogGetWidgetComponentLayoutCommand, Verbose, TEXT("Parameters: %s"), *Parameters);
    
    // Parse JSON parameters using centralized JSON utilities
    TSharedPtr<FJsonObject> JsonObject = ParseJsonParameters(Parameters);
    if (!JsonObject.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
        return SerializeErrorResponse(Error);
    }

    // Validate parameters using structured validation
    FString ValidationError;
    if (!ValidateParamsInternal(JsonObject, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return SerializeErrorResponse(Error);
    }

    // Execute command using service layer delegation
    TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);
    
    // Serialize response using centralized JSON utilities
    return SerializeJsonResponse(Response);
}

TSharedPtr<FJsonObject> FGetWidgetComponentLayoutCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    // Validate service availability using dependency injection pattern
    if (!UMGService.IsValid())
    {
        UE_LOG(LogGetWidgetComponentLayoutCommand, Error, TEXT("UMG service is not available - dependency injection failed"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }

    // Extract and validate parameters using structured parameter extraction
    FWidgetComponentLayoutParams LayoutParams;
    if (!ExtractWidgetComponentLayoutParameters(Params, LayoutParams))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Failed to extract widget component layout parameters"));
        return CreateErrorResponse(Error);
    }

    UE_LOG(LogGetWidgetComponentLayoutCommand, Log, TEXT("Getting component layout for widget '%s'"), 
           *LayoutParams.WidgetName);

    // Delegate to service layer following single responsibility principle
    TSharedPtr<FJsonObject> LayoutInfo;
    bool bSuccess = UMGService->GetWidgetComponentLayout(LayoutParams.WidgetName, LayoutInfo);
    
    if (!bSuccess || !LayoutInfo.IsValid())
    {
        UE_LOG(LogGetWidgetComponentLayoutCommand, Warning, TEXT("Service layer failed to get widget component layout"));
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("Failed to get component layout for widget '%s'"), 
                          *LayoutParams.WidgetName));
        return CreateErrorResponse(Error);
    }
    
    UE_LOG(LogGetWidgetComponentLayoutCommand, Log, TEXT("Widget component layout retrieved successfully"));
    return CreateSuccessResponse(LayoutParams, LayoutInfo);
}

FString FGetWidgetComponentLayoutCommand::GetCommandName() const
{
    return TEXT("get_widget_component_layout");
}

bool FGetWidgetComponentLayoutCommand::ValidateParams(const FString& Parameters) const
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

bool FGetWidgetComponentLayoutCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
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

    return true;
}

// JSON Utility Methods (following centralized JSON utilities pattern)
TSharedPtr<FJsonObject> FGetWidgetComponentLayoutCommand::ParseJsonParameters(const FString& Parameters) const
{
    if (Parameters.IsEmpty())
    {
        UE_LOG(LogGetWidgetComponentLayoutCommand, Warning, TEXT("Empty parameters provided"));
        return nullptr;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogGetWidgetComponentLayoutCommand, Error, TEXT("Failed to parse JSON parameters: %s"), *Parameters);
        return nullptr;
    }

    return JsonObject;
}

FString FGetWidgetComponentLayoutCommand::SerializeJsonResponse(const TSharedPtr<FJsonObject>& Response) const
{
    if (!Response.IsValid())
    {
        UE_LOG(LogGetWidgetComponentLayoutCommand, Error, TEXT("Invalid response object for serialization"));
        return TEXT("{}");
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetWidgetComponentLayoutCommand::SerializeErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);
    return SerializeJsonResponse(ErrorResponse);
}

// Parameter Extraction (following structured parameter extraction pattern)
bool FGetWidgetComponentLayoutCommand::ExtractWidgetComponentLayoutParameters(const TSharedPtr<FJsonObject>& Params, FWidgetComponentLayoutParams& OutParams) const
{
    // Extract widget name (required)
    if (!Params->HasField(TEXT("widget_name")))
    {
        UE_LOG(LogGetWidgetComponentLayoutCommand, Error, TEXT("Missing widget_name parameter"));
        return false;
    }
    OutParams.WidgetName = Params->GetStringField(TEXT("widget_name"));

    return true;
}

// Response Creation (following structured error handling pattern)
TSharedPtr<FJsonObject> FGetWidgetComponentLayoutCommand::CreateSuccessResponse(const FWidgetComponentLayoutParams& Params, const TSharedPtr<FJsonObject>& LayoutInfo) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    
    // Copy all fields from the layout info to the response
    if (LayoutInfo.IsValid())
    {
        for (auto& Pair : LayoutInfo->Values)
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
            FString::Printf(TEXT("Successfully retrieved component layout for widget '%s'"), 
                           *Params.WidgetName));
    }

    return ResponseObj;
}

TSharedPtr<FJsonObject> FGetWidgetComponentLayoutCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to get widget component layout: %s"), *Error.ErrorMessage));

    return ResponseObj;
}

