#include "Commands/UMG/BindWidgetEventCommand.h"
#include "Services/UMG/IUMGService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FBindWidgetEventCommand::FBindWidgetEventCommand(IUMGService& InUMGService)
    : UMGService(InUMGService)
{
}

FString FBindWidgetEventCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, ComponentName, EventName, FunctionName, Error;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentName, EventName, FunctionName, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Use the UMG service to bind the widget event
    FString ActualFunctionName;
    bool bResult = UMGService.BindWidgetEvent(BlueprintName, ComponentName, EventName, FunctionName, ActualFunctionName);
    
    if (!bResult)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to bind event '%s' on widget component: %s"), *EventName, *ComponentName));
    }

    return CreateSuccessResponse(ComponentName, EventName, ActualFunctionName);
}

FString FBindWidgetEventCommand::GetCommandName() const
{
    return TEXT("bind_widget_component_event");
}

bool FBindWidgetEventCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, ComponentName, EventName, FunctionName, Error;
    return ParseParameters(Parameters, BlueprintName, ComponentName, EventName, FunctionName, Error);
}

bool FBindWidgetEventCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName, 
                                            FString& OutEventName, FString& OutFunctionName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        return false;
    }

    // Required parameters
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName) || OutBlueprintName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'blueprint_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("widget_component_name"), OutComponentName) || OutComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_component_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("event_name"), OutEventName) || OutEventName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'event_name' parameter");
        return false;
    }

    // Optional function name parameter
    JsonObject->TryGetStringField(TEXT("function_name"), OutFunctionName);
    
    // Generate default function name if not provided
    if (OutFunctionName.IsEmpty())
    {
        OutFunctionName = OutComponentName + TEXT("_") + OutEventName;
    }

    return true;
}

FString FBindWidgetEventCommand::CreateSuccessResponse(const FString& ComponentName, const FString& EventName, const FString& FunctionName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    ResponseObj->SetStringField(TEXT("event_name"), EventName);
    ResponseObj->SetStringField(TEXT("function_name"), FunctionName);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FBindWidgetEventCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}