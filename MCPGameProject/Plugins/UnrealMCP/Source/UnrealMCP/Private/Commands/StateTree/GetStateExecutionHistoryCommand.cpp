#include "Commands/StateTree/GetStateExecutionHistoryCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetStateExecutionHistoryCommand::FGetStateExecutionHistoryCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetStateExecutionHistoryCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FString StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    FString ActorPath;
    ParamsObj->TryGetStringField(TEXT("actor_path"), ActorPath);
    int32 MaxEntries = 100;
    ParamsObj->TryGetNumberField(TEXT("max_entries"), MaxEntries);

    if (StateTreePath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("state_tree_path is required"));
    }

    TSharedPtr<FJsonObject> HistoryObj;
    if (!Service.GetStateExecutionHistory(StateTreePath, ActorPath, MaxEntries, HistoryObj))
    {
        return CreateErrorResponse(TEXT("Failed to get execution history"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("execution_history"), HistoryObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetStateExecutionHistoryCommand::GetCommandName() const
{
    return TEXT("get_state_execution_history");
}

bool FGetStateExecutionHistoryCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path"));
}

FString FGetStateExecutionHistoryCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
