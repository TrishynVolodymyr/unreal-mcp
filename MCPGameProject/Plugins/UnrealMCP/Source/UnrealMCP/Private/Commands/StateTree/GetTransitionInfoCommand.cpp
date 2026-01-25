#include "Commands/StateTree/GetTransitionInfoCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetTransitionInfoCommand::FGetTransitionInfoCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetTransitionInfoCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FGetTransitionInfoParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    Params.SourceStateName = ParamsObj->GetStringField(TEXT("source_state_name"));
    ParamsObj->TryGetNumberField(TEXT("transition_index"), Params.TransitionIndex);

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    TSharedPtr<FJsonObject> InfoObj;
    if (!Service.GetTransitionInfo(Params, InfoObj))
    {
        return CreateErrorResponse(TEXT("Failed to get transition info"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("transition_info"), InfoObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetTransitionInfoCommand::GetCommandName() const
{
    return TEXT("get_transition_info");
}

bool FGetTransitionInfoCommand::ValidateParams(const FString& Parameters) const
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

FString FGetTransitionInfoCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
