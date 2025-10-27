#include "Commands/Editor/SetActorPropertyCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetActorPropertyCommand::FSetActorPropertyCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FSetActorPropertyCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    FString PropertyName;
    TSharedPtr<FJsonValue> PropertyValue;
    FString Error;
    
    if (!ParseParameters(Parameters, ActorName, PropertyName, PropertyValue, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    // Find the actor
    AActor* Actor = EditorService.FindActorByName(ActorName);
    if (!Actor)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }
    
    // Set the property
    if (!EditorService.SetActorProperty(Actor, PropertyName, PropertyValue, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    return CreateSuccessResponse(Actor);
}

FString FSetActorPropertyCommand::GetCommandName() const
{
    return TEXT("set_actor_property");
}

bool FSetActorPropertyCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    FString PropertyName;
    TSharedPtr<FJsonValue> PropertyValue;
    FString Error;
    return ParseParameters(Parameters, ActorName, PropertyName, PropertyValue, Error);
}

bool FSetActorPropertyCommand::ParseParameters(const FString& JsonString, FString& OutActorName, 
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
    
    // Get required parameters
    if (!JsonObject->TryGetStringField(TEXT("name"), OutActorName))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("property_name"), OutPropertyName))
    {
        OutError = TEXT("Missing 'property_name' parameter");
        return false;
    }
    
    // Get property value (can be any JSON type)
    OutPropertyValue = JsonObject->TryGetField(TEXT("property_value"));
    if (!OutPropertyValue.IsValid())
    {
        OutError = TEXT("Missing 'property_value' parameter");
        return false;
    }
    
    return true;
}

FString FSetActorPropertyCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Actor property updated successfully"));
    ResponseObj->SetStringField(TEXT("actor_name"), Actor->GetName());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetActorPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

