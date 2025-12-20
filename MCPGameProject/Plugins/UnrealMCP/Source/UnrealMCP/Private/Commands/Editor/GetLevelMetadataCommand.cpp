#include "Commands/Editor/GetLevelMetadataCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetLevelMetadataCommand::FGetLevelMetadataCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FGetLevelMetadataCommand::GetCommandName() const
{
    return TEXT("get_level_metadata");
}

bool FGetLevelMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return true; // No strictly required parameters
}

FString FGetLevelMetadataCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString ActorFilter;
    JsonObject->TryGetStringField(TEXT("actor_filter"), ActorFilter);

    // Get fields array
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray = nullptr;
    JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray);

    // Determine which fields to include
    bool bIncludeAll = !FieldsArray || FieldsArray->Num() == 0;
    if (!bIncludeAll && FieldsArray)
    {
        for (const auto& Field : *FieldsArray)
        {
            if (Field->AsString() == TEXT("*"))
            {
                bIncludeAll = true;
                break;
            }
        }
    }

    // Build response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);

    // Add requested fields
    if (bIncludeAll || IsFieldRequested(FieldsArray, TEXT("actors")))
    {
        TSharedPtr<FJsonObject> ActorsInfo = BuildActorsInfo(ActorFilter);
        ResponseObj->SetObjectField(TEXT("actors"), ActorsInfo);
    }

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

bool FGetLevelMetadataCommand::IsFieldRequested(const TArray<TSharedPtr<FJsonValue>>* FieldsArray, const FString& FieldName) const
{
    if (!FieldsArray)
    {
        return false;
    }

    for (const auto& Field : *FieldsArray)
    {
        if (Field->AsString() == FieldName || Field->AsString() == TEXT("*"))
        {
            return true;
        }
    }
    return false;
}

TSharedPtr<FJsonObject> FGetLevelMetadataCommand::BuildActorsInfo(const FString& ActorFilter) const
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    TArray<AActor*> Actors;

    if (ActorFilter.IsEmpty())
    {
        // Get all actors
        Actors = EditorService.GetActorsInLevel();
    }
    else
    {
        // Get actors matching filter pattern
        Actors = EditorService.FindActorsByName(ActorFilter);
    }

    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }

    if (!ActorFilter.IsEmpty())
    {
        Result->SetStringField(TEXT("filter"), ActorFilter);
    }
    Result->SetNumberField(TEXT("count"), ActorArray.Num());
    Result->SetArrayField(TEXT("items"), ActorArray);

    return Result;
}
