#include "Commands/Editor/SpawnBlueprintActorCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSpawnBlueprintActorCommand::FSpawnBlueprintActorCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FSpawnBlueprintActorCommand::Execute(const FString& Parameters)
{
    FBlueprintActorSpawnParams SpawnParams;
    FString Error;
    
    if (!ParseParameters(Parameters, SpawnParams, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    AActor* SpawnedActor = EditorService.SpawnBlueprintActor(SpawnParams, Error);
    if (!SpawnedActor)
    {
        return CreateErrorResponse(Error);
    }
    
    return CreateSuccessResponse(SpawnedActor);
}

FString FSpawnBlueprintActorCommand::GetCommandName() const
{
    return TEXT("spawn_blueprint_actor");
}

bool FSpawnBlueprintActorCommand::ValidateParams(const FString& Parameters) const
{
    FBlueprintActorSpawnParams SpawnParams;
    FString Error;
    return ParseParameters(Parameters, SpawnParams, Error);
}

bool FSpawnBlueprintActorCommand::ParseParameters(const FString& JsonString, FBlueprintActorSpawnParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Get required parameters
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutParams.BlueprintName))
    {
        OutError = TEXT("Missing 'blueprint_name' parameter");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("actor_name"), OutParams.ActorName))
    {
        OutError = TEXT("Missing 'actor_name' parameter");
        return false;
    }
    
    // Get optional transform parameters
    if (JsonObject->HasField(TEXT("location")))
    {
        OutParams.Location = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("location"));
    }
    if (JsonObject->HasField(TEXT("rotation")))
    {
        OutParams.Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(JsonObject, TEXT("rotation"));
    }
    if (JsonObject->HasField(TEXT("scale")))
    {
        OutParams.Scale = FUnrealMCPCommonUtils::GetVectorFromJson(JsonObject, TEXT("scale"));
    }
    
    // Validate parameters
    return OutParams.IsValid(OutError);
}

FString FSpawnBlueprintActorCommand::CreateSuccessResponse(AActor* Actor) const
{
    if (!Actor)
    {
        return CreateErrorResponse(TEXT("Invalid actor"));
    }
    
    TSharedPtr<FJsonObject> ActorJson = FUnrealMCPCommonUtils::ActorToJsonObject(Actor, true);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ActorJson.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FSpawnBlueprintActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

