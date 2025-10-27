#include "Commands/Blueprint/SetPhysicsPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FSetPhysicsPropertiesCommand::FSetPhysicsPropertiesCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FSetPhysicsPropertiesCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString ComponentName;
    TMap<FString, float> PhysicsParams;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentName, PhysicsParams, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Set physics properties using the service
    if (!BlueprintService.SetPhysicsProperties(Blueprint, ComponentName, PhysicsParams))
    {
        return CreateErrorResponse(TEXT("Failed to set physics properties"));
    }
    
    return CreateSuccessResponse(BlueprintName, ComponentName);
}

FString FSetPhysicsPropertiesCommand::GetCommandName() const
{
    return TEXT("set_physics_properties");
}

bool FSetPhysicsPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString ComponentName;
    TMap<FString, float> PhysicsParams;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, ComponentName, PhysicsParams, ParseError);
}

bool FSetPhysicsPropertiesCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                                  FString& OutComponentName, TMap<FString, float>& OutPhysicsParams, 
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
    
    // Parse required component_name parameter
    if (!JsonObject->TryGetStringField(TEXT("component_name"), OutComponentName))
    {
        OutError = TEXT("Missing required 'component_name' parameter");
        return false;
    }
    
    // Parse optional physics parameters
    double DoubleValue;
    if (JsonObject->TryGetNumberField(TEXT("simulate_physics"), DoubleValue))
    {
        OutPhysicsParams.Add(TEXT("simulate_physics"), static_cast<float>(DoubleValue));
    }
    if (JsonObject->TryGetNumberField(TEXT("gravity_enabled"), DoubleValue))
    {
        OutPhysicsParams.Add(TEXT("gravity_enabled"), static_cast<float>(DoubleValue));
    }
    if (JsonObject->TryGetNumberField(TEXT("mass"), DoubleValue))
    {
        OutPhysicsParams.Add(TEXT("mass"), static_cast<float>(DoubleValue));
    }
    if (JsonObject->TryGetNumberField(TEXT("linear_damping"), DoubleValue))
    {
        OutPhysicsParams.Add(TEXT("linear_damping"), static_cast<float>(DoubleValue));
    }
    if (JsonObject->TryGetNumberField(TEXT("angular_damping"), DoubleValue))
    {
        OutPhysicsParams.Add(TEXT("angular_damping"), static_cast<float>(DoubleValue));
    }
    
    return true;
}

FString FSetPhysicsPropertiesCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName) const
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

FString FSetPhysicsPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



