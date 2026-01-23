#include "Commands/StateTree/BatchAddTransitionsCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FBatchAddTransitionsCommand::FBatchAddTransitionsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FBatchAddTransitionsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FBatchAddTransitionsParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));

    const TArray<TSharedPtr<FJsonValue>>* TransitionsArray;
    if (ParamsObj->TryGetArrayField(TEXT("transitions"), TransitionsArray))
    {
        for (const TSharedPtr<FJsonValue>& TransValue : *TransitionsArray)
        {
            const TSharedPtr<FJsonObject>* TransObj;
            if (TransValue->TryGetObject(TransObj))
            {
                FBatchTransitionDefinition TransDef;
                (*TransObj)->TryGetStringField(TEXT("source_state_name"), TransDef.SourceStateName);
                (*TransObj)->TryGetStringField(TEXT("target_state_name"), TransDef.TargetStateName);
                (*TransObj)->TryGetStringField(TEXT("trigger"), TransDef.Trigger);
                (*TransObj)->TryGetStringField(TEXT("transition_type"), TransDef.TransitionType);
                (*TransObj)->TryGetStringField(TEXT("priority"), TransDef.Priority);
                Params.Transitions.Add(TransDef);
            }
        }
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.BatchAddTransitions(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Batch added %d transitions"),
        Params.Transitions.Num()));
    ResponseObj->SetNumberField(TEXT("transitions_added"), Params.Transitions.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FBatchAddTransitionsCommand::GetCommandName() const
{
    return TEXT("batch_add_transitions");
}

bool FBatchAddTransitionsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("transitions"));
}

FString FBatchAddTransitionsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
