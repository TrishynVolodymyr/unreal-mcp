#include "Commands/Editor/GetActorsInLevelCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetActorsInLevelCommand::FGetActorsInLevelCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FGetActorsInLevelCommand::Execute(const FString& Parameters)
{
    // This command doesn't need parameters, just get all actors
    TArray<AActor*> Actors = EditorService.GetActorsInLevel();
    return CreateSuccessResponse(Actors);
}

FString FGetActorsInLevelCommand::GetCommandName() const
{
    return TEXT("get_actors_in_level");
}

bool FGetActorsInLevelCommand::ValidateParams(const FString& Parameters) const
{
    // This command doesn't require any parameters
    return true;
}

FString FGetActorsInLevelCommand::CreateSuccessResponse(const TArray<AActor*>& Actors) const
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

FString FGetActorsInLevelCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}

