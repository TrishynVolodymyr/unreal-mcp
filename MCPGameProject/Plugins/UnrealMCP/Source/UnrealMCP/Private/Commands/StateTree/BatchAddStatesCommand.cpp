#include "Commands/StateTree/BatchAddStatesCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FBatchAddStatesCommand::FBatchAddStatesCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FBatchAddStatesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FBatchAddStatesParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));

    const TArray<TSharedPtr<FJsonValue>>* StatesArray;
    if (ParamsObj->TryGetArrayField(TEXT("states"), StatesArray))
    {
        for (const TSharedPtr<FJsonValue>& StateValue : *StatesArray)
        {
            const TSharedPtr<FJsonObject>* StateObj;
            if (StateValue->TryGetObject(StateObj))
            {
                FBatchStateDefinition StateDef;
                (*StateObj)->TryGetStringField(TEXT("state_name"), StateDef.StateName);
                (*StateObj)->TryGetStringField(TEXT("parent_state_name"), StateDef.ParentStateName);
                (*StateObj)->TryGetStringField(TEXT("state_type"), StateDef.StateType);
                (*StateObj)->TryGetStringField(TEXT("selection_behavior"), StateDef.SelectionBehavior);
                (*StateObj)->TryGetBoolField(TEXT("enabled"), StateDef.bEnabled);
                Params.States.Add(StateDef);
            }
        }
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.BatchAddStates(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Batch added %d states"),
        Params.States.Num()));
    ResponseObj->SetNumberField(TEXT("states_added"), Params.States.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FBatchAddStatesCommand::GetCommandName() const
{
    return TEXT("batch_add_states");
}

bool FBatchAddStatesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("states"));
}

FString FBatchAddStatesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
