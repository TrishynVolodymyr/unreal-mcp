#include "Commands/StateTree/RemoveTransitionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FRemoveTransitionCommand::FRemoveTransitionCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FRemoveTransitionCommand::Execute(const FString& Parameters)
{
    FRemoveTransitionParams Params;
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

    FString RemoveError;
    if (!Service.RemoveTransition(Params, RemoveError))
    {
        return CreateErrorResponse(RemoveError.IsEmpty() ? TEXT("Failed to remove transition") : RemoveError);
    }

    return CreateSuccessResponse(Params.SourceStateName, Params.TransitionIndex);
}

FString FRemoveTransitionCommand::GetCommandName() const
{
    return TEXT("remove_transition");
}

bool FRemoveTransitionCommand::ValidateParams(const FString& Parameters) const
{
    FRemoveTransitionParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FRemoveTransitionCommand::ParseParameters(const FString& JsonString, FRemoveTransitionParams& OutParams, FString& OutError) const
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

    int32 TransitionIndex;
    if (JsonObject->TryGetNumberField(TEXT("transition_index"), TransitionIndex))
    {
        OutParams.TransitionIndex = TransitionIndex;
    }

    return true;
}

FString FRemoveTransitionCommand::CreateSuccessResponse(const FString& StateName, int32 TransitionIndex) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetNumberField(TEXT("transition_index"), TransitionIndex);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Transition removed successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FRemoveTransitionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
