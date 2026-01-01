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
    FString Error;

    // Try batch format first
    TArray<TPair<FString, TSharedPtr<FJsonValue>>> BatchProperties;
    if (ParseBatchParameters(Parameters, ActorName, BatchProperties, Error))
    {
        // Batch mode
        AActor* Actor = EditorService.FindActorByName(ActorName);
        if (!Actor)
        {
            return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
        }

        int32 SuccessCount = 0;
        TArray<FString> FailedProperties;

        for (const auto& PropPair : BatchProperties)
        {
            FString PropError;
            if (EditorService.SetActorProperty(Actor, PropPair.Key, PropPair.Value, PropError))
            {
                SuccessCount++;
            }
            else
            {
                FailedProperties.Add(FString::Printf(TEXT("%s: %s"), *PropPair.Key, *PropError));
            }
        }

        return CreateBatchSuccessResponse(Actor, SuccessCount, FailedProperties);
    }

    // Single property mode
    FString PropertyName;
    TSharedPtr<FJsonValue> PropertyValue;

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

bool FSetActorPropertyCommand::ParseBatchParameters(const FString& JsonString, FString& OutActorName,
                                                    TArray<TPair<FString, TSharedPtr<FJsonValue>>>& OutProperties,
                                                    FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Check if this is batch format (has "properties" array)
    if (!JsonObject->HasField(TEXT("properties")))
    {
        return false; // Not batch format
    }

    // Get actor name
    if (!JsonObject->TryGetStringField(TEXT("name"), OutActorName))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    // Get properties array
    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray;
    if (!JsonObject->TryGetArrayField(TEXT("properties"), PropertiesArray))
    {
        OutError = TEXT("'properties' must be an array");
        return false;
    }

    // Parse each property
    for (const TSharedPtr<FJsonValue>& PropValue : *PropertiesArray)
    {
        const TSharedPtr<FJsonObject>* PropObj;
        if (!PropValue->TryGetObject(PropObj))
        {
            OutError = TEXT("Each property must be an object with 'name' and 'value' fields");
            return false;
        }

        FString PropName;
        if (!(*PropObj)->TryGetStringField(TEXT("name"), PropName))
        {
            OutError = TEXT("Each property must have a 'name' field");
            return false;
        }

        TSharedPtr<FJsonValue> PropVal = (*PropObj)->TryGetField(TEXT("value"));
        if (!PropVal.IsValid())
        {
            OutError = FString::Printf(TEXT("Property '%s' is missing 'value' field"), *PropName);
            return false;
        }

        OutProperties.Add(TPair<FString, TSharedPtr<FJsonValue>>(PropName, PropVal));
    }

    return true;
}

FString FSetActorPropertyCommand::CreateBatchSuccessResponse(AActor* Actor, int32 SuccessCount,
                                                             const TArray<FString>& FailedProperties) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), FailedProperties.Num() == 0);
    ResponseObj->SetStringField(TEXT("actor_name"), Actor ? Actor->GetActorLabel() : TEXT("Unknown"));
    ResponseObj->SetNumberField(TEXT("properties_set"), SuccessCount);
    ResponseObj->SetNumberField(TEXT("properties_failed"), FailedProperties.Num());

    if (FailedProperties.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> FailedArray;
        for (const FString& Failed : FailedProperties)
        {
            FailedArray.Add(MakeShared<FJsonValueString>(Failed));
        }
        ResponseObj->SetArrayField(TEXT("failed_properties"), FailedArray);
        ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Set %d properties, %d failed"),
            SuccessCount, FailedProperties.Num()));
    }
    else
    {
        ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully set %d properties"),
            SuccessCount));
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

