#include "Commands/StateTree/AddEvaluatorCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddEvaluatorCommand::FAddEvaluatorCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddEvaluatorCommand::Execute(const FString& Parameters)
{
    FAddEvaluatorParams Params;
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
    if (!Service.AddEvaluator(Params, AddError))
    {
        return CreateErrorResponse(AddError.IsEmpty() ? TEXT("Failed to add evaluator") : AddError);
    }

    return CreateSuccessResponse(Params.EvaluatorStructPath);
}

FString FAddEvaluatorCommand::GetCommandName() const
{
    return TEXT("add_evaluator");
}

bool FAddEvaluatorCommand::ValidateParams(const FString& Parameters) const
{
    FAddEvaluatorParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FAddEvaluatorCommand::ParseParameters(const FString& JsonString, FAddEvaluatorParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("evaluator_struct_path"), OutParams.EvaluatorStructPath))
    {
        OutError = TEXT("Missing required 'evaluator_struct_path' parameter");
        return false;
    }

    JsonObject->TryGetStringField(TEXT("evaluator_name"), OutParams.EvaluatorName);

    const TSharedPtr<FJsonObject>* EvaluatorPropertiesObj;
    if (JsonObject->TryGetObjectField(TEXT("evaluator_properties"), EvaluatorPropertiesObj))
    {
        OutParams.EvaluatorProperties = *EvaluatorPropertiesObj;
    }

    return true;
}

FString FAddEvaluatorCommand::CreateSuccessResponse(const FString& EvaluatorType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("evaluator_type"), EvaluatorType);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Evaluator added successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddEvaluatorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
