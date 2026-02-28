#include "Commands/UMG/WrapWidgetComponentCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FWrapWidgetComponentCommand::FWrapWidgetComponentCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FWrapWidgetComponentCommand::Execute(const FString& Parameters)
{
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

    TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

TSharedPtr<FJsonObject> FWrapWidgetComponentCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    if (!UMGService.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG Service is not available"));
        return CreateErrorResponse(Error);
    }

    FString ValidationError;
    if (!ValidateParamsInternal(Params, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return CreateErrorResponse(Error);
    }

    FString BlueprintName = Params->GetStringField(TEXT("widget_name"));
    FString ComponentName = Params->GetStringField(TEXT("component_name"));
    FString WrapperType = Params->GetStringField(TEXT("wrapper_type"));
    
    FString WrapperName;
    Params->TryGetStringField(TEXT("wrapper_name"), WrapperName);
    if (WrapperName.IsEmpty())
    {
        WrapperName = FString::Printf(TEXT("Wrap_%s"), *ComponentName);
    }

    // Extract optional wrapper properties
    const TSharedPtr<FJsonObject>* WrapperProperties = nullptr;
    Params->TryGetObjectField(TEXT("wrapper_properties"), WrapperProperties);

    // Delegate to service
    FString OutError;
    bool bSuccess = UMGService->WrapWidgetComponent(BlueprintName, ComponentName, WrapperType, WrapperName,
                                                     WrapperProperties ? *WrapperProperties : nullptr, OutError);
    if (!bSuccess)
    {
        FString ErrorMessage = OutError.IsEmpty()
            ? FString::Printf(TEXT("Failed to wrap widget '%s' with '%s' in '%s'"), *ComponentName, *WrapperType, *BlueprintName)
            : OutError;
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(ErrorMessage);
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(BlueprintName, ComponentName, WrapperName, WrapperType);
}

FString FWrapWidgetComponentCommand::GetCommandName() const
{
    return TEXT("wrap_widget_component");
}

bool FWrapWidgetComponentCommand::ValidateParams(const FString& Parameters) const
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

bool FWrapWidgetComponentCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_name' parameter");
        return false;
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName) || ComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'component_name' parameter");
        return false;
    }

    FString WrapperType;
    if (!Params->TryGetStringField(TEXT("wrapper_type"), WrapperType) || WrapperType.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'wrapper_type' parameter");
        return false;
    }

    // Validate wrapper_type is a known container type
    static const TArray<FString> ValidWrapperTypes = {
        TEXT("SizeBox"), TEXT("Border"), TEXT("VerticalBox"), TEXT("HorizontalBox"),
        TEXT("Overlay"), TEXT("ScaleBox"), TEXT("CanvasPanel"), TEXT("WrapBox"),
        TEXT("GridPanel"), TEXT("UniformGridPanel"), TEXT("ScrollBox"), TEXT("WidgetSwitcher"),
        TEXT("SafeZone"), TEXT("BackgroundBlur")
    };

    bool bValidType = false;
    for (const FString& ValidType : ValidWrapperTypes)
    {
        if (WrapperType.Equals(ValidType, ESearchCase::IgnoreCase))
        {
            bValidType = true;
            break;
        }
    }

    if (!bValidType)
    {
        OutError = FString::Printf(TEXT("Invalid wrapper_type '%s'. Valid types: SizeBox, Border, VerticalBox, HorizontalBox, Overlay, ScaleBox, CanvasPanel, WrapBox, GridPanel, UniformGridPanel, ScrollBox, WidgetSwitcher, SafeZone, BackgroundBlur"), *WrapperType);
        return false;
    }

    return true;
}

TSharedPtr<FJsonObject> FWrapWidgetComponentCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName,
                                                                            const FString& WrapperName, const FString& WrapperType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("widget_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    ResponseObj->SetStringField(TEXT("wrapper_name"), WrapperName);
    ResponseObj->SetStringField(TEXT("wrapper_type"), WrapperType);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Widget '%s' wrapped with %s '%s' successfully"), *ComponentName, *WrapperType, *WrapperName));
    return ResponseObj;
}

TSharedPtr<FJsonObject> FWrapWidgetComponentCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("error_details"), Error.ErrorDetails);
    ResponseObj->SetNumberField(TEXT("error_code"), Error.ErrorCode);
    return ResponseObj;
}
