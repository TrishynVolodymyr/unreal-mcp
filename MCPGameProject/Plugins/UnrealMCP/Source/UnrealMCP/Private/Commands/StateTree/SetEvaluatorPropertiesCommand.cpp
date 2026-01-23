#include "Commands/StateTree/SetEvaluatorPropertiesCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FSetEvaluatorPropertiesCommand::FSetEvaluatorPropertiesCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FSetEvaluatorPropertiesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse parameters"));
    }

    FSetEvaluatorPropertiesParams Params;
    Params.StateTreePath = ParamsObj->GetStringField(TEXT("state_tree_path"));
    ParamsObj->TryGetNumberField(TEXT("evaluator_index"), Params.EvaluatorIndex);

    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (ParamsObj->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        Params.Properties = *PropertiesPtr;
    }

    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        return CreateErrorResponse(ValidationError);
    }

    FString Error;
    if (!Service.SetEvaluatorProperties(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Updated evaluator %d"),
        Params.EvaluatorIndex));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetEvaluatorPropertiesCommand::GetCommandName() const
{
    return TEXT("set_evaluator_properties");
}

bool FSetEvaluatorPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return false;
    }

    return ParamsObj->HasField(TEXT("state_tree_path"));
}

FString FSetEvaluatorPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
