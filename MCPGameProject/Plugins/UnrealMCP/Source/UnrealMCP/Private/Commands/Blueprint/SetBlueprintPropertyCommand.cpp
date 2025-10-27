#include "Commands/Blueprint/SetBlueprintPropertyCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FSetBlueprintPropertyCommand::FSetBlueprintPropertyCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FSetBlueprintPropertyCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString PropertyName;
    TSharedPtr<FJsonValue> PropertyValue;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, PropertyName, PropertyValue, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Set blueprint property using the service
    if (!BlueprintService.SetBlueprintProperty(Blueprint, PropertyName, PropertyValue))
    {
        return CreateErrorResponse(TEXT("Failed to set blueprint property"));
    }
    
    return CreateSuccessResponse(BlueprintName, PropertyName);
}

FString FSetBlueprintPropertyCommand::GetCommandName() const
{
    return TEXT("set_blueprint_property");
}

bool FSetBlueprintPropertyCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString PropertyName;
    TSharedPtr<FJsonValue> PropertyValue;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, PropertyName, PropertyValue, ParseError);
}

bool FSetBlueprintPropertyCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                                  FString& OutPropertyName, TSharedPtr<FJsonValue>& OutPropertyValue, 
                                                  FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required blueprint_name parameter
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    // Parse required property_name parameter
    if (!JsonObject->TryGetStringField(TEXT("property_name"), OutPropertyName))
    {
        OutError = TEXT("Missing required 'property_name' parameter");
        return false;
    }
    
    // Parse required property_value parameter
    OutPropertyValue = JsonObject->TryGetField(TEXT("property_value"));
    if (!OutPropertyValue.IsValid())
    {
        OutError = TEXT("Missing required 'property_value' parameter");
        return false;
    }
    
    return true;
}

FString FSetBlueprintPropertyCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& PropertyName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("property_name"), PropertyName);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetBlueprintPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



