#include "Commands/UMG/SetWidgetParentClassCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FSetWidgetParentClassCommand::FSetWidgetParentClassCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FSetWidgetParentClassCommand::Execute(const FString& Parameters)
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

TSharedPtr<FJsonObject> FSetWidgetParentClassCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!UMGService.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG Service is not available"));
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
    FString NewParentClass = Params->GetStringField(TEXT("new_parent_class"));

    // Use the UMG service to change parent class
    FString OldParentClass;
    FString OutError;
    bool bSuccess = UMGService->SetWidgetParentClass(WidgetName, NewParentClass, OldParentClass, OutError);
    if (!bSuccess)
    {
        FString ErrorMessage = OutError.IsEmpty()
            ? FString::Printf(TEXT("Failed to set parent class for widget '%s'"), *WidgetName)
            : OutError;
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(ErrorMessage);
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(WidgetName, NewParentClass, OldParentClass);
}

FString FSetWidgetParentClassCommand::GetCommandName() const
{
    return TEXT("set_widget_parent_class");
}

bool FSetWidgetParentClassCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ValidationError;
    return ValidateParamsInternal(JsonObject, ValidationError);
}

bool FSetWidgetParentClassCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_name' parameter");
        return false;
    }

    FString NewParentClass;
    if (!Params->TryGetStringField(TEXT("new_parent_class"), NewParentClass) || NewParentClass.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'new_parent_class' parameter");
        return false;
    }

    return true;
}

TSharedPtr<FJsonObject> FSetWidgetParentClassCommand::CreateSuccessResponse(const FString& WidgetName, const FString& NewParentClass,
                                                                             const FString& OldParentClass) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResponseObj->SetStringField(TEXT("new_parent_class"), NewParentClass);
    ResponseObj->SetStringField(TEXT("old_parent_class"), OldParentClass);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Parent class changed from '%s' to '%s'"), *OldParentClass, *NewParentClass));

    return ResponseObj;
}

TSharedPtr<FJsonObject> FSetWidgetParentClassCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);

    return ResponseObj;
}
