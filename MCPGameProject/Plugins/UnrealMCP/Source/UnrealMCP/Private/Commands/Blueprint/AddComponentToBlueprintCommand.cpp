#include "Commands/Blueprint/AddComponentToBlueprintCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Services/ComponentService.h"
#include "Services/ComponentService.h"

FAddComponentToBlueprintCommand::FAddComponentToBlueprintCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FAddComponentToBlueprintCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FComponentCreationParams ComponentParams;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentParams, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Validate component parameters
    FString ValidationError;
    if (!ComponentParams.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Add component using the service
    FString ErrorMessage;
    if (!BlueprintService.AddComponentToBlueprint(Blueprint, ComponentParams, ErrorMessage))
    {
        return CreateErrorResponse(ErrorMessage.IsEmpty() ? TEXT("Failed to add component to blueprint") : ErrorMessage);
    }
    
    return CreateSuccessResponse(BlueprintName, ComponentParams.ComponentName);
}

FString FAddComponentToBlueprintCommand::GetCommandName() const
{
    return TEXT("add_component_to_blueprint");
}

bool FAddComponentToBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FComponentCreationParams ComponentParams;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentParams, ParseError))
    {
        return false;
    }
    
    FString ValidationError;
    return ComponentParams.IsValid(ValidationError);
}

bool FAddComponentToBlueprintCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                                     FComponentCreationParams& OutParams, FString& OutError) const
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
    
    // Parse required component_name parameter
    if (!JsonObject->TryGetStringField(TEXT("component_name"), OutParams.ComponentName))
    {
        OutError = TEXT("Missing required 'component_name' parameter");
        return false;
    }
    
    // Parse required component_type parameter
    if (!JsonObject->TryGetStringField(TEXT("component_type"), OutParams.ComponentType))
    {
        OutError = TEXT("Missing required 'component_type' parameter");
        return false;
    }
    
    // Component type validation will be handled by ComponentService during creation
    // This allows for dynamic discovery of both built-in and Blueprint components
    
    // Parse optional location parameter
    const TArray<TSharedPtr<FJsonValue>>* LocationArray;
    if (JsonObject->TryGetArrayField(TEXT("location"), LocationArray))
    {
        OutParams.Location.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *LocationArray)
        {
            double DoubleValue;
            if (Value->TryGetNumber(DoubleValue))
            {
                OutParams.Location.Add(static_cast<float>(DoubleValue));
            }
        }
    }
    
    // Parse optional rotation parameter
    const TArray<TSharedPtr<FJsonValue>>* RotationArray;
    if (JsonObject->TryGetArrayField(TEXT("rotation"), RotationArray))
    {
        OutParams.Rotation.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *RotationArray)
        {
            double DoubleValue;
            if (Value->TryGetNumber(DoubleValue))
            {
                OutParams.Rotation.Add(static_cast<float>(DoubleValue));
            }
        }
    }
    
    // Parse optional scale parameter
    const TArray<TSharedPtr<FJsonValue>>* ScaleArray;
    if (JsonObject->TryGetArrayField(TEXT("scale"), ScaleArray))
    {
        OutParams.Scale.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *ScaleArray)
        {
            double DoubleValue;
            if (Value->TryGetNumber(DoubleValue))
            {
                OutParams.Scale.Add(static_cast<float>(DoubleValue));
            }
        }
    }
    
    // Parse optional parent_component_name parameter
    FString ParentComponentName;
    if (JsonObject->TryGetStringField(TEXT("parent_component_name"), ParentComponentName))
    {
        OutParams.ParentComponentName = ParentComponentName;
    }
    
    // Parse optional component_properties parameter
    const TSharedPtr<FJsonObject>* ComponentPropsPtr;
    if (JsonObject->TryGetObjectField(TEXT("component_properties"), ComponentPropsPtr))
    {
        OutParams.ComponentProperties = *ComponentPropsPtr;
    }
    
    return true;
}



FString FAddComponentToBlueprintCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResponseObj->SetStringField(TEXT("component_name"), ComponentName);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FAddComponentToBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



