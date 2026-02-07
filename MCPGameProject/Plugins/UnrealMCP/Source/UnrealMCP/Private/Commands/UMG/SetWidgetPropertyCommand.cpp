#include "Commands/UMG/SetWidgetPropertyCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSetWidgetPropertyCommand, Log, All);

FSetWidgetPropertyCommand::FSetWidgetPropertyCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FSetWidgetPropertyCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogSetWidgetPropertyCommand, Log, TEXT("SetWidgetPropertyCommand::Execute - Command execution started"));
    UE_LOG(LogSetWidgetPropertyCommand, Verbose, TEXT("Parameters: %s"), *Parameters);
    
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

FString FSetWidgetPropertyCommand::GetCommandName() const
{
    return TEXT("set_widget_component_property");
}

bool FSetWidgetPropertyCommand::ValidateParams(const FString& Parameters) const
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

TSharedPtr<FJsonObject> FSetWidgetPropertyCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    // Validate service availability using dependency injection pattern
    if (!UMGService.IsValid())
    {
        UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("UMG service is not available - dependency injection failed"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }

    // Extract and validate parameters using structured parameter extraction
    FWidgetPropertyParams PropertyParams;
    if (!ExtractWidgetPropertyParameters(Params, PropertyParams))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Failed to extract widget property parameters"));
        return CreateErrorResponse(Error);
    }

    UE_LOG(LogSetWidgetPropertyCommand, Log, TEXT("Setting properties on component '%s' in widget '%s'"), 
           *PropertyParams.ComponentName, *PropertyParams.WidgetName);

    // Delegate to service layer following single responsibility principle
    TArray<FString> SuccessProperties, FailedProperties;
    bool bResult = UMGService->SetWidgetProperties(
        PropertyParams.WidgetName, 
        PropertyParams.ComponentName, 
        PropertyParams.Properties, 
        SuccessProperties, 
        FailedProperties
    );
    
    if (!bResult && SuccessProperties.Num() == 0)
    {
        UE_LOG(LogSetWidgetPropertyCommand, Warning, TEXT("Service layer failed to set any properties on widget component"));
        
        // Build detailed failure message
        FString FailedList = FString::Join(FailedProperties, TEXT(", "));
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("Failed to set properties on '%s': [%s]. These properties were not found on the widget or its slot."), 
                           *PropertyParams.ComponentName, *FailedList));
        return CreateErrorResponse(Error);
    }
    
    // Log with appropriate level based on partial failures
    if (FailedProperties.Num() > 0)
    {
        UE_LOG(LogSetWidgetPropertyCommand, Warning, TEXT("Widget properties partially set: %d succeeded, %d failed. Failed: [%s]"), 
               SuccessProperties.Num(), FailedProperties.Num(), *FString::Join(FailedProperties, TEXT(", ")));
    }
    else
    {
        UE_LOG(LogSetWidgetPropertyCommand, Log, TEXT("Widget properties set successfully: %d succeeded, %d failed"), 
               SuccessProperties.Num(), FailedProperties.Num());
    }
    return CreateSuccessResponse(PropertyParams, SuccessProperties, FailedProperties);
}

bool FSetWidgetPropertyCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
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

    // Check for component_name (required)
    if (!Params->HasField(TEXT("component_name")))
    {
        OutError = TEXT("Missing required parameter: component_name");
        return false;
    }

    FString ComponentName = Params->GetStringField(TEXT("component_name"));
    if (ComponentName.IsEmpty())
    {
        OutError = TEXT("component_name cannot be empty");
        return false;
    }

    // Check for kwargs (required)
    if (!Params->HasField(TEXT("kwargs")))
    {
        OutError = TEXT("Missing required parameter: kwargs");
        return false;
    }

    // Validate kwargs format (can be object or string)
    const TSharedPtr<FJsonObject>* KwargsObject;
    if (Params->TryGetObjectField(TEXT("kwargs"), KwargsObject))
    {
        if (!KwargsObject->IsValid() || (*KwargsObject)->Values.Num() == 0)
        {
            OutError = TEXT("kwargs object is empty or invalid");
            return false;
        }
    }
    else
    {
        FString KwargsString;
        if (Params->TryGetStringField(TEXT("kwargs"), KwargsString))
        {
            if (KwargsString.IsEmpty())
            {
                OutError = TEXT("kwargs string cannot be empty");
                return false;
            }
        }
        else
        {
            OutError = TEXT("kwargs must be either an object or a string");
            return false;
        }
    }

    return true;
}

// JSON Utility Methods (following centralized JSON utilities pattern)
TSharedPtr<FJsonObject> FSetWidgetPropertyCommand::ParseJsonParameters(const FString& Parameters) const
{
    if (Parameters.IsEmpty())
    {
        UE_LOG(LogSetWidgetPropertyCommand, Warning, TEXT("Empty parameters provided"));
        return nullptr;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("Failed to parse JSON parameters: %s"), *Parameters);
        return nullptr;
    }

    return JsonObject;
}

FString FSetWidgetPropertyCommand::SerializeJsonResponse(const TSharedPtr<FJsonObject>& Response) const
{
    if (!Response.IsValid())
    {
        UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("Invalid response object for serialization"));
        return TEXT("{}");
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetWidgetPropertyCommand::SerializeErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);
    return SerializeJsonResponse(ErrorResponse);
}

// Parameter Extraction (following structured parameter extraction pattern)
bool FSetWidgetPropertyCommand::ExtractWidgetPropertyParameters(const TSharedPtr<FJsonObject>& Params, FWidgetPropertyParams& OutParams) const
{
    // Extract widget name (required)
    if (!Params->HasField(TEXT("widget_name")))
    {
        UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("Missing widget_name parameter"));
        return false;
    }
    OutParams.WidgetName = Params->GetStringField(TEXT("widget_name"));

    // Extract component name (required)
    if (!Params->HasField(TEXT("component_name")))
    {
        UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("Missing component_name parameter"));
        return false;
    }
    OutParams.ComponentName = Params->GetStringField(TEXT("component_name"));

    // Extract kwargs (required) - try object first, then string for backward compatibility
    const TSharedPtr<FJsonObject>* KwargsObject;
    if (Params->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
    {
        OutParams.Properties = *KwargsObject;
    }
    else
    {
        // Fallback to string format for backward compatibility
        FString KwargsString;
        if (Params->TryGetStringField(TEXT("kwargs"), KwargsString))
        {
            TSharedRef<TJsonReader<>> KwargsReader = TJsonReaderFactory<>::Create(KwargsString);
            if (!FJsonSerializer::Deserialize(KwargsReader, OutParams.Properties) || !OutParams.Properties.IsValid())
            {
                UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("Invalid kwargs JSON format"));
                return false;
            }
        }
        else
        {
            UE_LOG(LogSetWidgetPropertyCommand, Error, TEXT("Missing or invalid kwargs parameter"));
            return false;
        }
    }

    return true;
}

// Response Creation (following structured error handling pattern)
TSharedPtr<FJsonObject> FSetWidgetPropertyCommand::CreateSuccessResponse(const FWidgetPropertyParams& Params, 
                                                                        const TArray<FString>& SuccessProperties, 
                                                                        const TArray<FString>& FailedProperties) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    
    // success = true only if ALL properties were set (no failures)
    bool bFullSuccess = FailedProperties.Num() == 0;
    ResponseObj->SetBoolField(TEXT("success"), bFullSuccess);
    
    // If some succeeded and some failed, mark as partial
    if (!bFullSuccess && SuccessProperties.Num() > 0)
    {
        ResponseObj->SetBoolField(TEXT("partial_success"), true);
    }
    
    ResponseObj->SetStringField(TEXT("component_name"), Params.ComponentName);
    
    // Add success properties array
    TArray<TSharedPtr<FJsonValue>> SuccessArray;
    for (const FString& Property : SuccessProperties)
    {
        SuccessArray.Add(MakeShared<FJsonValueString>(Property));
    }
    ResponseObj->SetArrayField(TEXT("success_properties"), SuccessArray);
    
    // Add failed properties array
    TArray<TSharedPtr<FJsonValue>> FailedArray;
    for (const FString& Property : FailedProperties)
    {
        FailedArray.Add(MakeShared<FJsonValueString>(Property));
    }
    ResponseObj->SetArrayField(TEXT("failed_properties"), FailedArray);
    
    // Build informative message
    if (bFullSuccess)
    {
        ResponseObj->SetStringField(TEXT("message"), 
            FString::Printf(TEXT("Successfully set %d properties on component '%s' in widget '%s'"), 
                           SuccessProperties.Num(), *Params.ComponentName, *Params.WidgetName));
    }
    else
    {
        FString FailedList = FString::Join(FailedProperties, TEXT(", "));
        ResponseObj->SetStringField(TEXT("message"), 
            FString::Printf(TEXT("Set %d properties on '%s', but %d FAILED: [%s]. Check property names and types."), 
                           SuccessProperties.Num(), *Params.ComponentName, FailedProperties.Num(), *FailedList));
    }

    return ResponseObj;
}

TSharedPtr<FJsonObject> FSetWidgetPropertyCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to set widget properties: %s"), *Error.ErrorMessage));

    return ResponseObj;
}

