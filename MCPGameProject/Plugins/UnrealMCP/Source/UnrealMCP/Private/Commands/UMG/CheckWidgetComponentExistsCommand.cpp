#include "Commands/UMG/CheckWidgetComponentExistsCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCheckWidgetComponentExistsCommand::FCheckWidgetComponentExistsCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FCheckWidgetComponentExistsCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogTemp, Log, TEXT("FCheckWidgetComponentExistsCommand::Execute: NEW ARCHITECTURE COMMAND CALLED!"));
    UE_LOG(LogTemp, Log, TEXT("Using service layer delegation for component existence checking"));
    
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

TSharedPtr<FJsonObject> FCheckWidgetComponentExistsCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!UMGService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("NEW ARCHITECTURE: UMGService is not valid!"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }

    // Validate parameters using new architecture
    FString ValidationError;
    if (!ValidateParamsInternal(Params, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return CreateErrorResponse(Error);
    }

    // Extract parameters
    FString WidgetName, ComponentName;
    
    // Support both widget_name and blueprint_name for compatibility
    if (Params->HasField(TEXT("widget_name")))
    {
        WidgetName = Params->GetStringField(TEXT("widget_name"));
    }
    else if (Params->HasField(TEXT("blueprint_name")))
    {
        WidgetName = Params->GetStringField(TEXT("blueprint_name"));
    }
    
    ComponentName = Params->GetStringField(TEXT("component_name"));

    UE_LOG(LogTemp, Log, TEXT("NEW ARCHITECTURE: Checking component '%s' in widget '%s'"), *ComponentName, *WidgetName);

    // Use service layer to check component existence
    bool bExists = UMGService->DoesWidgetComponentExist(WidgetName, ComponentName);
    
    UE_LOG(LogTemp, Log, TEXT("NEW ARCHITECTURE: Component existence check result: %s"), bExists ? TEXT("EXISTS") : TEXT("NOT EXISTS"));

    return CreateSuccessResponse(bExists, WidgetName, ComponentName);
}

FString FCheckWidgetComponentExistsCommand::GetCommandName() const
{
    return TEXT("check_widget_component_exists");
}

bool FCheckWidgetComponentExistsCommand::ValidateParams(const FString& Parameters) const
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

bool FCheckWidgetComponentExistsCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    if (!Params.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Check for widget_name or blueprint_name
    if (!Params->HasField(TEXT("widget_name")) && !Params->HasField(TEXT("blueprint_name")))
    {
        OutError = TEXT("Missing required parameter: widget_name or blueprint_name");
        return false;
    }

    FString WidgetName;
    if (Params->HasField(TEXT("widget_name")))
    {
        WidgetName = Params->GetStringField(TEXT("widget_name"));
    }
    else if (Params->HasField(TEXT("blueprint_name")))
    {
        WidgetName = Params->GetStringField(TEXT("blueprint_name"));
    }
    
    if (WidgetName.IsEmpty())
    {
        OutError = TEXT("widget_name/blueprint_name cannot be empty");
        return false;
    }

    // Check for component_name
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

    return true;
}

TSharedPtr<FJsonObject> FCheckWidgetComponentExistsCommand::CreateSuccessResponse(bool bExists, const FString& WidgetName, const FString& ComponentName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetBoolField(TEXT("exists"), bExists);
    ResponseObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    ResponseObj->SetStringField(TEXT("message"), bExists ? 
        FString::Printf(TEXT("Component '%s' exists in widget '%s'"), *ComponentName, *WidgetName) :
        FString::Printf(TEXT("Component '%s' does not exist in widget '%s'"), *ComponentName, *WidgetName));

    return ResponseObj;
}

TSharedPtr<FJsonObject> FCheckWidgetComponentExistsCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetBoolField(TEXT("exists"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to check component existence: %s"), *Error.ErrorMessage));

    return ResponseObj;
}

