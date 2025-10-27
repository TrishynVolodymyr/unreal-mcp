#include "Commands/UMG/CreateWidgetBlueprintCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "WidgetBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateWidgetBlueprintCommand::FCreateWidgetBlueprintCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FCreateWidgetBlueprintCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogTemp, Warning, TEXT("=== NEW ARCHITECTURE PROOF ==="));
    UE_LOG(LogTemp, Warning, TEXT("FCreateWidgetBlueprintCommand::Execute: NEW ARCHITECTURE COMMAND CALLED!"));
    UE_LOG(LogTemp, Warning, TEXT("Using service layer delegation instead of direct legacy calls"));
    
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

TSharedPtr<FJsonObject> FCreateWidgetBlueprintCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!UMGService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("NEW ARCHITECTURE: UMGService is not valid!"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("NEW ARCHITECTURE: UMGService is valid, proceeding with service call"));

    // Validate parameters using the new validation framework
    FString ValidationError;
    if (!ValidateParamsInternal(Params, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return CreateErrorResponse(Error);
    }

    // Extract parameters
    FString Name = Params->GetStringField(TEXT("name"));
    FString ParentClass = Params->GetStringField(TEXT("parent_class"));
    FString Path = Params->GetStringField(TEXT("path"));
    
    // Set defaults if not provided
    if (ParentClass.IsEmpty())
    {
        ParentClass = TEXT("UserWidget");
    }
    
    if (Path.IsEmpty())
    {
        Path = TEXT("/Game/Widgets");
    }

    UE_LOG(LogTemp, Warning, TEXT("NEW ARCHITECTURE: Creating widget blueprint with Name=%s, ParentClass=%s, Path=%s"), *Name, *ParentClass, *Path);

    // Check if it already exists BEFORE creating it
    bool bAlreadyExists = UMGService->DoesWidgetBlueprintExist(Name, Path);

    // Use the UMG service to create the widget blueprint
    UWidgetBlueprint* CreatedBlueprint = UMGService->CreateWidgetBlueprint(Name, ParentClass, Path);
    if (!CreatedBlueprint)
    {
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(FString::Printf(TEXT("Failed to create widget blueprint: %s"), *Name));
        return CreateErrorResponse(Error);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("NEW ARCHITECTURE: Widget blueprint created successfully, returning success response"));
    return CreateSuccessResponse(CreatedBlueprint, Path, bAlreadyExists);
}

FString FCreateWidgetBlueprintCommand::GetCommandName() const
{
    return TEXT("create_umg_widget_blueprint");
}

bool FCreateWidgetBlueprintCommand::ValidateParams(const FString& Parameters) const
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

bool FCreateWidgetBlueprintCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    if (!Params.IsValid())
    {
        OutError = TEXT("Invalid parameters object");
        return false;
    }

    // Simple validation without the parameter validator for now
    // Check required parameter: name
    FString Name = Params->GetStringField(TEXT("name"));
    if (Name.IsEmpty())
    {
        OutError = TEXT("Widget name is required and cannot be empty");
        return false;
    }

    // Optional parameters don't need validation as they have defaults
    return true;
}

TSharedPtr<FJsonObject> FCreateWidgetBlueprintCommand::CreateSuccessResponse(UWidgetBlueprint* WidgetBlueprint, const FString& Path, bool bAlreadyExists) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), WidgetBlueprint->GetName());
    ResponseObj->SetStringField(TEXT("path"), Path + TEXT("/") + WidgetBlueprint->GetName());
    ResponseObj->SetBoolField(TEXT("already_exists"), bAlreadyExists);

    return ResponseObj;
}

TSharedPtr<FJsonObject> FCreateWidgetBlueprintCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
    ResponseObj->SetNumberField(TEXT("error_type"), static_cast<int32>(Error.ErrorType));

    return ResponseObj;
}

