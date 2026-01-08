#include "Commands/Editor/BatchDeleteActorsCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FBatchDeleteActorsCommand::FBatchDeleteActorsCommand(IEditorService& InEditorService)
    : EditorService(InEditorService)
{
}

FString FBatchDeleteActorsCommand::Execute(const FString& Parameters)
{
    TArray<FString> ActorNames;
    FString Error;

    if (!ParseParameters(Parameters, ActorNames, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (ActorNames.Num() == 0)
    {
        return CreateErrorResponse(TEXT("No actor names provided"));
    }

    TArray<TSharedPtr<FJsonObject>> Results;
    int32 SuccessCount = 0;
    int32 FailedCount = 0;

    for (const FString& ActorName : ActorNames)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("name"), ActorName);

        // Check if actor exists
        AActor* Actor = EditorService.FindActorByName(ActorName);
        if (!Actor)
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetBoolField(TEXT("deleted"), false);
            ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Actor not found: %s"), *ActorName));
            FailedCount++;
            Results.Add(ResultObj);
            continue;
        }

        // Attempt deletion
        FString DeleteError;
        if (EditorService.DeleteActor(ActorName, DeleteError))
        {
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetBoolField(TEXT("deleted"), true);
            SuccessCount++;
        }
        else
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetBoolField(TEXT("deleted"), false);
            ResultObj->SetStringField(TEXT("error"), DeleteError);
            FailedCount++;
        }

        Results.Add(ResultObj);
    }

    return CreateSuccessResponse(Results, ActorNames.Num(), SuccessCount, FailedCount);
}

FString FBatchDeleteActorsCommand::GetCommandName() const
{
    return TEXT("batch_delete_actors");
}

bool FBatchDeleteActorsCommand::ValidateParams(const FString& Parameters) const
{
    TArray<FString> ActorNames;
    FString Error;
    return ParseParameters(Parameters, ActorNames, Error);
}

bool FBatchDeleteActorsCommand::ParseParameters(const FString& JsonString, TArray<FString>& OutActorNames, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Get the names array
    const TArray<TSharedPtr<FJsonValue>>* NamesArray;
    if (!JsonObject->TryGetArrayField(TEXT("names"), NamesArray))
    {
        OutError = TEXT("Missing 'names' array parameter");
        return false;
    }

    // Parse each name from the array
    for (const TSharedPtr<FJsonValue>& Value : *NamesArray)
    {
        FString Name;
        if (Value.IsValid() && Value->TryGetString(Name))
        {
            OutActorNames.Add(Name);
        }
    }

    return true;
}

FString FBatchDeleteActorsCommand::CreateSuccessResponse(const TArray<TSharedPtr<FJsonObject>>& Results, int32 TotalCount, int32 SuccessCount, int32 FailedCount) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();

    // Convert results array to JSON values
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    for (const TSharedPtr<FJsonObject>& Result : Results)
    {
        ResultsArray.Add(MakeShared<FJsonValueObject>(Result));
    }

    ResponseObj->SetArrayField(TEXT("results"), ResultsArray);
    ResponseObj->SetNumberField(TEXT("total"), TotalCount);
    ResponseObj->SetNumberField(TEXT("succeeded"), SuccessCount);
    ResponseObj->SetNumberField(TEXT("failed"), FailedCount);
    ResponseObj->SetBoolField(TEXT("success"), true);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FBatchDeleteActorsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    ErrorObj->SetBoolField(TEXT("success"), false);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
