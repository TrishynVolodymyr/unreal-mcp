#include "Commands/StateTree/RemoveStateCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FRemoveStateCommand::FRemoveStateCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FRemoveStateCommand::Execute(const FString& Parameters)
{
    FRemoveStateParams Params;
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
    if (!Service.RemoveState(Params, RemoveError))
    {
        return CreateErrorResponse(RemoveError.IsEmpty() ? TEXT("Failed to remove state") : RemoveError);
    }

    return CreateSuccessResponse(Params.StateName);
}

FString FRemoveStateCommand::GetCommandName() const
{
    return TEXT("remove_state");
}

bool FRemoveStateCommand::ValidateParams(const FString& Parameters) const
{
    FRemoveStateParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FRemoveStateCommand::ParseParameters(const FString& JsonString, FRemoveStateParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("state_name"), OutParams.StateName))
    {
        OutError = TEXT("Missing required 'state_name' parameter");
        return false;
    }

    JsonObject->TryGetBoolField(TEXT("remove_children"), OutParams.bRemoveChildren);

    return true;
}

FString FRemoveStateCommand::CreateSuccessResponse(const FString& StateName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("State '%s' removed successfully"), *StateName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FRemoveStateCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
