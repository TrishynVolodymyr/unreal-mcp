#include "Commands/StateTree/AddTransitionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddTransitionCommand::FAddTransitionCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddTransitionCommand::Execute(const FString& Parameters)
{
    FAddTransitionParams Params;
    FString ParseError;

    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString AddError;
    if (!Service.AddTransition(Params, AddError))
    {
        return CreateErrorResponse(AddError.IsEmpty() ? TEXT("Failed to add transition") : AddError);
    }

    return CreateSuccessResponse(Params.SourceStateName, Params.TargetStateName);
}

FString FAddTransitionCommand::GetCommandName() const
{
    return TEXT("add_transition");
}

bool FAddTransitionCommand::ValidateParams(const FString& Parameters) const
{
    FAddTransitionParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FAddTransitionCommand::ParseParameters(const FString& JsonString, FAddTransitionParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("state_tree_path"), OutParams.StateTreePath))
    {
        OutError = TEXT("Missing required 'state_tree_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("source_state_name"), OutParams.SourceStateName))
    {
        OutError = TEXT("Missing required 'source_state_name' parameter");
        return false;
    }

    JsonObject->TryGetStringField(TEXT("trigger"), OutParams.Trigger);
    JsonObject->TryGetStringField(TEXT("target_state_name"), OutParams.TargetStateName);
    JsonObject->TryGetStringField(TEXT("transition_type"), OutParams.TransitionType);
    JsonObject->TryGetStringField(TEXT("event_tag"), OutParams.EventTag);
    JsonObject->TryGetStringField(TEXT("priority"), OutParams.Priority);
    JsonObject->TryGetBoolField(TEXT("delay_transition"), OutParams.bDelayTransition);

    double DelayDuration;
    if (JsonObject->TryGetNumberField(TEXT("delay_duration"), DelayDuration))
    {
        OutParams.DelayDuration = static_cast<float>(DelayDuration);
    }

    return true;
}

FString FAddTransitionCommand::CreateSuccessResponse(const FString& SourceState, const FString& TargetState) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("source_state"), SourceState);
    ResponseObj->SetStringField(TEXT("target_state"), TargetState);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Transition from '%s' to '%s' added successfully"), *SourceState, *TargetState));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddTransitionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
