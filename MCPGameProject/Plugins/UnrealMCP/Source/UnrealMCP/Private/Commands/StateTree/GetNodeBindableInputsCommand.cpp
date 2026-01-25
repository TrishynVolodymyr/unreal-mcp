#include "Commands/StateTree/GetNodeBindableInputsCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetNodeBindableInputsCommand::FGetNodeBindableInputsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetNodeBindableInputsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FString StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    FString NodeIdentifier = ParamsObj->GetStringField(TEXT("node_identifier"));
    int32 TaskIndex = -1;
    ParamsObj->TryGetNumberField(TEXT("task_index"), TaskIndex);

    if (StateTreePath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("state_tree_path is required"));
    }
    if (NodeIdentifier.IsEmpty())
    {
        return CreateErrorResponse(TEXT("node_identifier is required"));
    }

    TSharedPtr<FJsonObject> Inputs;
    if (!Service.GetNodeBindableInputs(StateTreePath, NodeIdentifier, TaskIndex, Inputs))
    {
        return CreateErrorResponse(TEXT("Failed to get bindable inputs"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("data"), Inputs);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetNodeBindableInputsCommand::GetCommandName() const
{
    return TEXT("get_node_bindable_inputs");
}

bool FGetNodeBindableInputsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("node_identifier"));
}

FString FGetNodeBindableInputsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
