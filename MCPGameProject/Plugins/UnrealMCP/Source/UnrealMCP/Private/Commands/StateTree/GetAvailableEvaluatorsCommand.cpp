#include "Commands/StateTree/GetAvailableEvaluatorsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetAvailableEvaluatorsCommand::FGetAvailableEvaluatorsCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetAvailableEvaluatorsCommand::Execute(const FString& Parameters)
{
    TArray<TPair<FString, FString>> Evaluators;
    if (!Service.GetAvailableEvaluatorTypes(Evaluators))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve available evaluator types"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);

    TArray<TSharedPtr<FJsonValue>> EvaluatorsArray;
    for (const auto& Evaluator : Evaluators)
    {
        TSharedPtr<FJsonObject> EvaluatorObj = MakeShared<FJsonObject>();
        EvaluatorObj->SetStringField(TEXT("path"), Evaluator.Key);
        EvaluatorObj->SetStringField(TEXT("name"), Evaluator.Value);
        EvaluatorsArray.Add(MakeShared<FJsonValueObject>(EvaluatorObj));
    }
    ResponseObj->SetArrayField(TEXT("evaluators"), EvaluatorsArray);
    ResponseObj->SetNumberField(TEXT("count"), Evaluators.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetAvailableEvaluatorsCommand::GetCommandName() const
{
    return TEXT("get_available_evaluators");
}

bool FGetAvailableEvaluatorsCommand::ValidateParams(const FString& Parameters) const
{
    // No required parameters for this command
    return true;
}

FString FGetAvailableEvaluatorsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
