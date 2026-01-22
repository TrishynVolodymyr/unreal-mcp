#include "Commands/StateTree/SetTransitionPropertiesCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FSetTransitionPropertiesCommand::FSetTransitionPropertiesCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FSetTransitionPropertiesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FSetTransitionPropertiesParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    Params.SourceStateName = ParamsObj->GetStringField(TEXT("source_state_name"));
    ParamsObj->TryGetNumberField(TEXT("transition_index"), Params.TransitionIndex);
    ParamsObj->TryGetStringField(TEXT("trigger"), Params.Trigger);
    ParamsObj->TryGetStringField(TEXT("target_state_name"), Params.TargetStateName);
    ParamsObj->TryGetStringField(TEXT("priority"), Params.Priority);

    bool bDelayTransition;
    if (ParamsObj->TryGetBoolField(TEXT("delay_transition"), bDelayTransition))
    {
        Params.bDelayTransition = bDelayTransition;
    }

    double DelayDuration;
    if (ParamsObj->TryGetNumberField(TEXT("delay_duration"), DelayDuration))
    {
        Params.DelayDuration = static_cast<float>(DelayDuration);
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.SetTransitionProperties(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Updated transition %d in state '%s'"),
        Params.TransitionIndex, *Params.SourceStateName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetTransitionPropertiesCommand::GetCommandName() const
{
    return TEXT("set_transition_properties");
}

bool FSetTransitionPropertiesCommand::ValidateParams(const FString& Parameters) const
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

FString FSetTransitionPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
