#include "Commands/Editor/FindActorsByNameCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FFindActorsByNameCommand::FFindActorsByNameCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FFindActorsByNameCommand::Execute(const FString& Parameters)
{
    FString Pattern;
    FString Error;
    
    if (!ParseParameters(Parameters, Pattern, Error))
    {
        return CreateErrorResponse(Error);
    }
    
    TArray<AActor*> MatchingActors = EditorService.FindActorsByName(Pattern);
    return CreateSuccessResponse(MatchingActors);
}

FString FFindActorsByNameCommand::GetCommandName() const
{
    return TEXT("find_actors_by_name");
}

bool FFindActorsByNameCommand::ValidateParams(const FString& Parameters) const
{
    FString Pattern;
    FString Error;
    return ParseParameters(Parameters, Pattern, Error);
}

bool FFindActorsByNameCommand::ParseParameters(const FString& JsonString, FString& OutPattern, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    if (!JsonObject->TryGetStringField(TEXT("pattern"), OutPattern))
    {
        OutError = TEXT("Missing 'pattern' parameter");
        return false;
    }
    
    return true;
}

FString FFindActorsByNameCommand::CreateSuccessResponse(const TArray<AActor*>& Actors) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    ResponseObj->SetArrayField(TEXT("actors"), ActorArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString FFindActorsByNameCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

