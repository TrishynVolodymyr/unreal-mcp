#include "Commands/UMG/BindWidgetEventCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FBindWidgetEventCommand::FBindWidgetEventCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FBindWidgetEventCommand::Execute(const FString& Parameters)
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

FString FBindWidgetEventCommand::GetCommandName() const
{
    return TEXT("bind_widget_component_event");
}

bool FBindWidgetEventCommand::ValidateParams(const FString& Parameters) const
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

TSharedPtr<FJsonObject> FBindWidgetEventCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!UMGService.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }

    // Validate parameters using the new validation framework
    FString ValidationError;
    if (!ValidateParamsInternal(Params, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return CreateErrorResponse(Error);
    }

    // Extract parameters
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString ComponentName = Params->GetStringField(TEXT("widget_component_name"));
    FString EventName = Params->GetStringField(TEXT("event_name"));
    FString FunctionName = Params->GetStringField(TEXT("function_name"));
    
    // Generate default function name if not provided
    if (FunctionName.IsEmpty())
    {
        FunctionName = ComponentName + TEXT("_") + EventName;
    }

    // Use the UMG service to bind the widget event
    FString ActualFunctionName;
    bool bResult = UMGService->BindWidgetEvent(BlueprintName, ComponentName, EventName, FunctionName, ActualFunctionName);
    
    if (!bResult)
    {
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("Failed to bind event '%s' on widget component: %s"), *EventName, *ComponentName));
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ComponentName, EventName, ActualFunctionName);
}

bool FBindWidgetEventCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    if (!Params.IsValid())
    {
        OutError = TEXT("Invalid parameters object");
        return false;
    }

    // Check required parameters
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    if (BlueprintName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'blueprint_name' parameter");
        return false;
    }

    FString ComponentName = Params->GetStringField(TEXT("widget_component_name"));
    if (ComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_component_name' parameter");
        return false;
    }

    FString EventName = Params->GetStringField(TEXT("event_name"));
    if (EventName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'event_name' parameter");
        return false;
    }

    // function_name is optional, so no validation needed
    return true;
}

TSharedPtr<FJsonObject> FBindWidgetEventCommand::CreateSuccessResponse(const FString& ComponentName, const FString& EventName, const FString& FunctionName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    ResponseObj->SetStringField(TEXT("event_name"), EventName);
    ResponseObj->SetStringField(TEXT("function_name"), FunctionName);

    return ResponseObj;
}

TSharedPtr<FJsonObject> FBindWidgetEventCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
    ResponseObj->SetNumberField(TEXT("error_type"), static_cast<int32>(Error.ErrorType));

    return ResponseObj;
}

