#include "Commands/Editor/DeleteActorCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FDeleteActorCommand::FDeleteActorCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FDeleteActorCommand::Execute(const FString& Parameters)
{
    FString ActorName;
    FString Error;
    
    if (!ParseParameters(Parameters, ActorName, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    // Get actor info before deletion for the response
    AActor* Actor = EditorService.FindActorByName(ActorName);
    if (!Actor)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }
    
    if (!EditorService.DeleteActor(ActorName, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    return CreateSuccessResponse(ActorName);
}

FString FDeleteActorCommand::GetCommandName() const
{
    return TEXT("delete_actor");
}

bool FDeleteActorCommand::ValidateParams(const FString& Parameters) const
{
    FString ActorName;
    FString Error;
    return ParseParameters(Parameters, ActorName, Error);
}

bool FDeleteActorCommand::ParseParameters(const FString& JsonString, FString& OutActorName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("name"), OutActorName))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }
    
    return true;
}

FString FDeleteActorCommand::CreateSuccessResponse(const FString& ActorName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetStringField(TEXT("deleted_actor"), ActorName);
    ResponseObj->SetBoolField(TEXT("success"), true);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FDeleteActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

