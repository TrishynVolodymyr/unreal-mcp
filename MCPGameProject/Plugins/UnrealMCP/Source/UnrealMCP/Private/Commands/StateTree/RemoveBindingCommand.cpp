#include "Commands/StateTree/RemoveBindingCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FRemoveBindingCommand::FRemoveBindingCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FRemoveBindingCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FRemoveBindingParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    Params.TargetNodeName = ParamsObj->GetStringField(TEXT("target_node_name"));
    Params.TargetPropertyName = ParamsObj->GetStringField(TEXT("target_property_name"));
    ParamsObj->TryGetNumberField(TEXT("task_index"), Params.TaskIndex);
    ParamsObj->TryGetNumberField(TEXT("transition_index"), Params.TransitionIndex);
    ParamsObj->TryGetNumberField(TEXT("condition_index"), Params.ConditionIndex);

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.RemoveBinding(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Removed binding from %s.%s"),
        *Params.TargetNodeName, *Params.TargetPropertyName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FRemoveBindingCommand::GetCommandName() const
{
    return TEXT("remove_state_tree_binding");
}

bool FRemoveBindingCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path")) &&
           ParamsObj->HasField(TEXT("target_node_name")) &&
           ParamsObj->HasField(TEXT("target_property_name"));
}

FString FRemoveBindingCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
