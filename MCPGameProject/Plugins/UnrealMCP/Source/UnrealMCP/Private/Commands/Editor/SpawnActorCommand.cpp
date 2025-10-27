#include "Commands/Editor/SpawnActorCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSpawnActorCommand::FSpawnActorCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FSpawnActorCommand::Execute(const FString& Parameters)
{
    FActorSpawnParams SpawnParams;
    FString Error;
    
    if (!ParseParameters(Parameters, SpawnParams, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    AActor* SpawnedActor = EditorService.SpawnActor(SpawnParams, Error);
    if (!SpawnedActor)
    {
        return CreateErrorResponse(Error);
    }
    
    return CreateSuccessResponse(SpawnedActor);
}

FString FSpawnActorCommand::GetCommandName() const
{
    return TEXT("spawn_actor");
}

bool FSpawnActorCommand::ValidateParams(const FString& Parameters) const
{
    FActorSpawnParams SpawnParams;
    FString Error;
    return ParseParameters(Parameters, SpawnParams, Error);
}

bool FSpawnActorCommand::ParseParameters(const FString& JsonString, FActorSpawnParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Get required parameters
    if (!JsonObject->TryGetStringField(TEXT("type"), OutParams.Type))
    {
        OutError = TEXT("Missing 'type' parameter");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing 'name' parameter");
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

FString FSpawnActorCommand::CreateSuccessResponse(AActor* Actor) const
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

FString FSpawnActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

