#include "Commands/UMG/SetWidgetPropertyCommand.h"
#include "Services/UMG/IUMGService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FSetWidgetPropertyCommand::FSetWidgetPropertyCommand(IUMGService& InUMGService)
    : UMGService(InUMGService)
{
}

FString FSetWidgetPropertyCommand::Execute(const FString& Parameters)
{
    FString BlueprintName, ComponentName, Error;
    TSharedPtr<FJsonObject> Properties;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentName, Properties, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Use the UMG service to set widget properties
    TArray<FString> SuccessProperties, FailedProperties;
    bool bResult = UMGService.SetWidgetProperties(BlueprintName, ComponentName, Properties, SuccessProperties, FailedProperties);
    
    if (!bResult && SuccessProperties.Num() == 0)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to set any properties on widget component: %s"), *ComponentName));
    }

    return CreateSuccessResponse(ComponentName, SuccessProperties, FailedProperties);
}

FString FSetWidgetPropertyCommand::GetCommandName() const
{
    return TEXT("set_widget_component_property");
}

bool FSetWidgetPropertyCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName, ComponentName, Error;
    TSharedPtr<FJsonObject> Properties;
    return ParseParameters(Parameters, BlueprintName, ComponentName, Properties, Error);
}

bool FSetWidgetPropertyCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, FString& OutComponentName, 
                                              TSharedPtr<FJsonObject>& OutProperties, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        return false;
    }

    // Required parameters
    if (!JsonObject->TryGetStringField(TEXT("widget_name"), OutBlueprintName) || OutBlueprintName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'widget_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("component_name"), OutComponentName) || OutComponentName.IsEmpty())
    {
        OutError = TEXT("Missing or empty 'component_name' parameter");
        return false;
    }

    // Parse kwargs parameter which contains the properties to set
    FString KwargsString;
    if (JsonObject->TryGetStringField(TEXT("kwargs"), KwargsString))
    {
        TSharedRef<TJsonReader<>> KwargsReader = TJsonReaderFactory<>::Create(KwargsString);
        if (!FJsonSerializer::Deserialize(KwargsReader, OutProperties) || !OutProperties.IsValid())
        {
            OutError = TEXT("Invalid kwargs JSON format");
            return false;
        }
    }
    else
    {
        OutError = TEXT("Missing 'kwargs' parameter with properties to set");
        return false;
    }

    return true;
}

FString FSetWidgetPropertyCommand::CreateSuccessResponse(const FString& ComponentName, const TArray<FString>& SuccessProperties, 
                                                       const TArray<FString>& FailedProperties) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    
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

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetWidgetPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}