#include "Commands/StateTree/GetTransitionConditionsCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetTransitionConditionsCommand::FGetTransitionConditionsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetTransitionConditionsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FString StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    FString SourceStateName = ParamsObj->GetStringField(TEXT("source_state_name"));
    int32 TransitionIndex = 0;
    ParamsObj->TryGetNumberField(TEXT("transition_index"), TransitionIndex);

    if (StateTreePath.IsEmpty() || SourceStateName.IsEmpty())
    {
        return CreateErrorResponse(TEXT("state_tree_path and source_state_name are required"));
    }

    TSharedPtr<FJsonObject> ConditionsObj;
    if (!Service.GetTransitionConditions(StateTreePath, SourceStateName, TransitionIndex, ConditionsObj))
    {
        return CreateErrorResponse(TEXT("Failed to get transition conditions"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("result"), ConditionsObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetTransitionConditionsCommand::GetCommandName() const
{
    return TEXT("get_transition_conditions");
}

bool FGetTransitionConditionsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("source_state_name"));
}

FString FGetTransitionConditionsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
