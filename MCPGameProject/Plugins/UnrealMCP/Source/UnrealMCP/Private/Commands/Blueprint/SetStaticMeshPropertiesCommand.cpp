#include "Commands/Blueprint/SetStaticMeshPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"

FSetStaticMeshPropertiesCommand::FSetStaticMeshPropertiesCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FSetStaticMeshPropertiesCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    FString ComponentName;
    FString StaticMeshPath;
    FString ParseError;
    
    if (!ParseParameters(Parameters, BlueprintName, ComponentName, StaticMeshPath, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    
    // Set static mesh properties using the service
    if (!BlueprintService.SetStaticMeshProperties(Blueprint, ComponentName, StaticMeshPath))
    {
        return CreateErrorResponse(TEXT("Failed to set static mesh properties"));
    }
    
    return CreateSuccessResponse(BlueprintName, ComponentName);
}

FString FSetStaticMeshPropertiesCommand::GetCommandName() const
{
    return TEXT("set_static_mesh_properties");
}

bool FSetStaticMeshPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    FString ComponentName;
    FString StaticMeshPath;
    FString ParseError;
    
    return ParseParameters(Parameters, BlueprintName, ComponentName, StaticMeshPath, ParseError);
}

bool FSetStaticMeshPropertiesCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, 
                                                     FString& OutComponentName, FString& OutStaticMeshPath, 
                                                     FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required parameters
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("component_name"), OutComponentName))
    {
        OutError = TEXT("Missing required 'component_name' parameter");
        return false;
    }
    
    // Static mesh path is optional, defaults to cube
    if (!JsonObject->TryGetStringField(TEXT("static_mesh"), OutStaticMeshPath))
    {
        OutStaticMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
    }
    
    return true;
}

FString FSetStaticMeshPropertiesCommand::CreateSuccessResponse(const FString& BlueprintName, const FString& ComponentName) const
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

FString FSetStaticMeshPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}



