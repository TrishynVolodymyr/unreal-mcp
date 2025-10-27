#include "Commands/UMG/GetWidgetContainerDimensionsCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogGetWidgetContainerDimensionsCommand, Log, All);

FGetWidgetContainerDimensionsCommand::FGetWidgetContainerDimensionsCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FGetWidgetContainerDimensionsCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogGetWidgetContainerDimensionsCommand, Log, TEXT("GetWidgetContainerDimensionsCommand::Execute - Command execution started"));
    UE_LOG(LogGetWidgetContainerDimensionsCommand, Verbose, TEXT("Parameters: %s"), *Parameters);
    
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

TSharedPtr<FJsonObject> FGetWidgetContainerDimensionsCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    // Validate service availability using dependency injection pattern
    if (!UMGService.IsValid())
    {
        UE_LOG(LogGetWidgetContainerDimensionsCommand, Error, TEXT("UMG service is not available - dependency injection failed"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }

    // Extract and validate parameters using structured parameter extraction
    FWidgetContainerDimensionsParams DimensionsParams;
    if (!ExtractContainerDimensionsParameters(Params, DimensionsParams))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Failed to extract container dimensions parameters"));
        return CreateErrorResponse(Error);
    }

    UE_LOG(LogGetWidgetContainerDimensionsCommand, Log, TEXT("Getting dimensions for container '%s' in widget '%s'"), 
           *DimensionsParams.ContainerName, *DimensionsParams.WidgetName);

    // Delegate to service layer following single responsibility principle
    FVector2D Dimensions;
    bool bSuccess = UMGService->GetWidgetContainerDimensions(
        DimensionsParams.WidgetName, 
        DimensionsParams.ContainerName, 
        Dimensions
    );
    
    if (!bSuccess)
    {
        UE_LOG(LogGetWidgetContainerDimensionsCommand, Warning, TEXT("Service layer failed to get widget container dimensions"));
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("Failed to get dimensions for container '%s' in widget '%s'"), 
                          *DimensionsParams.ContainerName, *DimensionsParams.WidgetName));
        return CreateErrorResponse(Error);
    }
    
    UE_LOG(LogGetWidgetContainerDimensionsCommand, Log, TEXT("Widget container dimensions retrieved successfully: %fx%f"), 
           Dimensions.X, Dimensions.Y);
    return CreateSuccessResponse(DimensionsParams, Dimensions);
}

FString FGetWidgetContainerDimensionsCommand::GetCommandName() const
{
    return TEXT("get_widget_container_component_dimensions");
}

bool FGetWidgetContainerDimensionsCommand::ValidateParams(const FString& Parameters) const
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

bool FGetWidgetContainerDimensionsCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
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

    // container_name is optional, defaults to "CanvasPanel_0"
    if (Params->HasField(TEXT("container_name")))
    {
        FString ContainerName = Params->GetStringField(TEXT("container_name"));
        if (ContainerName.IsEmpty())
        {
            OutError = TEXT("container_name cannot be empty if provided");
            return false;
        }
    }

    return true;
}

// JSON Utility Methods (following centralized JSON utilities pattern)
TSharedPtr<FJsonObject> FGetWidgetContainerDimensionsCommand::ParseJsonParameters(const FString& Parameters) const
{
    if (Parameters.IsEmpty())
    {
        UE_LOG(LogGetWidgetContainerDimensionsCommand, Warning, TEXT("Empty parameters provided"));
        return nullptr;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogGetWidgetContainerDimensionsCommand, Error, TEXT("Failed to parse JSON parameters: %s"), *Parameters);
        return nullptr;
    }

    return JsonObject;
}

FString FGetWidgetContainerDimensionsCommand::SerializeJsonResponse(const TSharedPtr<FJsonObject>& Response) const
{
    if (!Response.IsValid())
    {
        UE_LOG(LogGetWidgetContainerDimensionsCommand, Error, TEXT("Invalid response object for serialization"));
        return TEXT("{}");
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetWidgetContainerDimensionsCommand::SerializeErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);
    return SerializeJsonResponse(ErrorResponse);
}

// Parameter Extraction (following structured parameter extraction pattern)
bool FGetWidgetContainerDimensionsCommand::ExtractContainerDimensionsParameters(const TSharedPtr<FJsonObject>& Params, FWidgetContainerDimensionsParams& OutParams) const
{
    // Extract widget name (required)
    if (!Params->HasField(TEXT("widget_name")))
    {
        UE_LOG(LogGetWidgetContainerDimensionsCommand, Error, TEXT("Missing widget_name parameter"));
        return false;
    }
    OutParams.WidgetName = Params->GetStringField(TEXT("widget_name"));

    // Extract container name (optional, defaults to "CanvasPanel_0")
    if (Params->HasField(TEXT("container_name")))
    {
        OutParams.ContainerName = Params->GetStringField(TEXT("container_name"));
    }
    else
    {
        OutParams.ContainerName = TEXT("CanvasPanel_0");
    }

    return true;
}

// Response Creation (following structured error handling pattern)
TSharedPtr<FJsonObject> FGetWidgetContainerDimensionsCommand::CreateSuccessResponse(const FWidgetContainerDimensionsParams& Params, const FVector2D& Dimensions) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("widget_name"), Params.WidgetName);
    ResponseObj->SetStringField(TEXT("container_name"), Params.ContainerName);
    
    // Add dimension information
    ResponseObj->SetNumberField(TEXT("width"), Dimensions.X);
    ResponseObj->SetNumberField(TEXT("height"), Dimensions.Y);
    
    // Add position information (containers typically start at 0,0)
    ResponseObj->SetNumberField(TEXT("x"), 0.0f);
    ResponseObj->SetNumberField(TEXT("y"), 0.0f);
    
    ResponseObj->SetStringField(TEXT("message"), 
        FString::Printf(TEXT("Successfully retrieved dimensions for container '%s' in widget '%s': %fx%f"), 
                       *Params.ContainerName, *Params.WidgetName, Dimensions.X, Dimensions.Y));

    return ResponseObj;
}

TSharedPtr<FJsonObject> FGetWidgetContainerDimensionsCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to get widget container dimensions: %s"), *Error.ErrorMessage));

    return ResponseObj;
}

