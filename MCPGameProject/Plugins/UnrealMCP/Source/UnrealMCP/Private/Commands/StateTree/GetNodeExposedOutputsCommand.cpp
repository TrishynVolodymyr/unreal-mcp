#include "Commands/StateTree/GetNodeExposedOutputsCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetNodeExposedOutputsCommand::FGetNodeExposedOutputsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetNodeExposedOutputsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FString StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    FString NodeIdentifier = ParamsObj->GetStringField(TEXT("node_identifier"));

    if (StateTreePath.IsEmpty())
    {
        return CreateErrorResponse(TEXT("state_tree_path is required"));
    }
    if (NodeIdentifier.IsEmpty())
    {
        return CreateErrorResponse(TEXT("node_identifier is required"));
    }

    TSharedPtr<FJsonObject> Outputs;
    if (!Service.GetNodeExposedOutputs(StateTreePath, NodeIdentifier, Outputs))
    {
        return CreateErrorResponse(TEXT("Failed to get exposed outputs"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("data"), Outputs);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetNodeExposedOutputsCommand::GetCommandName() const
{
    return TEXT("get_node_exposed_outputs");
}

bool FGetNodeExposedOutputsCommand::ValidateParams(const FString& Parameters) const
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

FString FGetNodeExposedOutputsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
