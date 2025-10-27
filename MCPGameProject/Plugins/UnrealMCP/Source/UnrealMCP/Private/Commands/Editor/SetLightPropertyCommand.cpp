#include "Commands/Editor/SetLightPropertyCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetLightPropertyCommand::FSetLightPropertyCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FSetLightPropertyCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    FString PropertyName;
    FString PropertyValue;
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
    
    // Set the light property
    if (!EditorService.SetLightProperty(Actor, PropertyName, PropertyValue, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    return CreateSuccessResponse(Actor);
}

FString FSetLightPropertyCommand::GetCommandName() const
{
    return TEXT("set_light_property");
}

bool FSetLightPropertyCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    FString PropertyName;
    FString PropertyValue;
    FString Error;
    return ParseParameters(Parameters, ActorName, PropertyName, PropertyValue, Error);
}

bool FSetLightPropertyCommand::ParseParameters(const FString& JsonString, FString& OutActorName, 
                                              FString& OutPropertyName, FString& OutPropertyValue, 
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
    
    // Get property value - can be string, number, or array
    if (!JsonObject->TryGetStringField(TEXT("property_value"), OutPropertyValue))
    {
        // Try to get as number and convert to string
        double NumValue;
        if (JsonObject->TryGetNumberField(TEXT("property_value"), NumValue))
        {
            OutPropertyValue = FString::Printf(TEXT("%.6f"), NumValue);
        }
        else
        {
            // Try to get as array (for colors, vectors, etc.)
            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
            if (JsonObject->TryGetArrayField(TEXT("property_value"), ArrayValue))
            {
                TArray<FString> StringValues;
                for (const TSharedPtr<FJsonValue>& Value : *ArrayValue)
                {
                    if (Value.IsValid())
                    {
                        if (Value->Type == EJson::Number)
                        {
                            StringValues.Add(FString::Printf(TEXT("%.6f"), Value->AsNumber()));
                        }
                        else if (Value->Type == EJson::String)
                        {
                            StringValues.Add(Value->AsString());
                        }
                    }
                }
                OutPropertyValue = FString::Join(StringValues, TEXT(","));
            }
            else
            {
                OutError = TEXT("Missing or invalid 'property_value' parameter - must be string, number, or array");
                return false;
            }
        }
    }
    
    return true;
}

FString FSetLightPropertyCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Light property updated successfully"));
    ResponseObj->SetStringField(TEXT("actor_name"), Actor->GetName());
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSetLightPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

