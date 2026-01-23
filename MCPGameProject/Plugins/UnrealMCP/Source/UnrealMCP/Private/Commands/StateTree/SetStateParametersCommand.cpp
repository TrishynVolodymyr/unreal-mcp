#include "Commands/StateTree/SetStateParametersCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FSetStateParametersCommand::FSetStateParametersCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FSetStateParametersCommand::Execute(const FString& Parameters)
{
    FSetStateParametersParams Params;
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

    FString SetError;
    if (!Service.SetStateParameters(Params, SetError))
    {
        return CreateErrorResponse(SetError.IsEmpty() ? TEXT("Failed to set state parameters") : SetError);
    }

    return CreateSuccessResponse(Params.StateName);
}

FString FSetStateParametersCommand::GetCommandName() const
{
    return TEXT("set_state_parameters");
}

bool FSetStateParametersCommand::ValidateParams(const FString& Parameters) const
{
    FSetStateParametersParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FSetStateParametersCommand::ParseParameters(const FString& JsonString, FSetStateParametersParams& OutParams, FString& OutError) const
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

    const TSharedPtr<FJsonObject>* ParametersObj;
    if (JsonObject->TryGetObjectField(TEXT("parameters"), ParametersObj))
    {
        OutParams.Parameters = *ParametersObj;
    }

    return true;
}

FString FSetStateParametersCommand::CreateSuccessResponse(const FString& StateName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetStringField(TEXT("message"), TEXT("State parameters set successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetStateParametersCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
