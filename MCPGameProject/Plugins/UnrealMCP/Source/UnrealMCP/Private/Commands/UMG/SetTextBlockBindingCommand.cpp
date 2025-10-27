#include "Commands/UMG/SetTextBlockBindingCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FSetTextBlockBindingCommand::FSetTextBlockBindingCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FSetTextBlockBindingCommand::Execute(const FString& Parameters)
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

FString FSetTextBlockBindingCommand::GetCommandName() const
{
    return TEXT("set_text_block_widget_component_binding");
}

bool FSetTextBlockBindingCommand::ValidateParams(const FString& Parameters) const
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

TSharedPtr<FJsonObject> FSetTextBlockBindingCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
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
    FString TextBlockName = Params->GetStringField(TEXT("widget_name"));
    FString BindingProperty = Params->GetStringField(TEXT("binding_name"));
    FString VariableType = Params->GetStringField(TEXT("variable_type"));
    
    // Use default variable type if not provided
    if (VariableType.IsEmpty())
    {
        VariableType = TEXT("Text");
    }

    // Use the UMG service to set the text block binding
    bool bResult = UMGService->SetTextBlockBinding(BlueprintName, TextBlockName, BindingProperty, VariableType);
    
    if (!bResult)
    {
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("Failed to set text block binding for '%s' on widget: %s"), *TextBlockName, *BlueprintName));
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(TextBlockName, BindingProperty, VariableType);
}

bool FSetTextBlockBindingCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
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

    FString TextBlockName = Params->GetStringField(TEXT("widget_name"));
    if (TextBlockName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_name' parameter");
        return false;
    }

    FString BindingProperty = Params->GetStringField(TEXT("binding_name"));
    if (BindingProperty.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'binding_name' parameter");
        return false;
    }

    // variable_type is optional, defaults to "Text"
    return true;
}

TSharedPtr<FJsonObject> FSetTextBlockBindingCommand::CreateSuccessResponse(const FString& TextBlockName, const FString& BindingProperty, const FString& VariableType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("text_block_name"), TextBlockName);
    ResponseObj->SetStringField(TEXT("binding_property"), BindingProperty);
    ResponseObj->SetStringField(TEXT("variable_type"), VariableType);
    ResponseObj->SetStringField(TEXT("function_name"), FString::Printf(TEXT("Get%s"), *BindingProperty));
    ResponseObj->SetStringField(TEXT("message"), TEXT("Text block binding created successfully"));

    return ResponseObj;
}

TSharedPtr<FJsonObject> FSetTextBlockBindingCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
    ResponseObj->SetNumberField(TEXT("error_type"), static_cast<int32>(Error.ErrorType));

    return ResponseObj;
}



