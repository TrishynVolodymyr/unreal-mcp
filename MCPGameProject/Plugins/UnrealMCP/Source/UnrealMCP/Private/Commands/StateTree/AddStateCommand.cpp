#include "Commands/StateTree/AddStateCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddStateCommand::FAddStateCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddStateCommand::Execute(const FString& Parameters)
{
    FAddStateParams Params;
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
    if (!Service.AddState(Params, AddError))
    {
        return CreateErrorResponse(AddError.IsEmpty() ? TEXT("Failed to add state") : AddError);
    }

    return CreateSuccessResponse(Params.StateName);
}

FString FAddStateCommand::GetCommandName() const
{
    return TEXT("add_state");
}

bool FAddStateCommand::ValidateParams(const FString& Parameters) const
{
    FAddStateParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FAddStateCommand::ParseParameters(const FString& JsonString, FAddStateParams& OutParams, FString& OutError) const
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

    JsonObject->TryGetStringField(TEXT("parent_state_name"), OutParams.ParentStateName);
    JsonObject->TryGetStringField(TEXT("state_type"), OutParams.StateType);
    JsonObject->TryGetStringField(TEXT("selection_behavior"), OutParams.SelectionBehavior);
    JsonObject->TryGetBoolField(TEXT("enabled"), OutParams.bEnabled);

    return true;
}

FString FAddStateCommand::CreateSuccessResponse(const FString& StateName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("State '%s' added successfully"), *StateName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddStateCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
