#include "Commands/StateTree/AddEnterConditionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddEnterConditionCommand::FAddEnterConditionCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddEnterConditionCommand::Execute(const FString& Parameters)
{
    FAddEnterConditionParams Params;
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
    if (!Service.AddEnterCondition(Params, AddError))
    {
        return CreateErrorResponse(AddError.IsEmpty() ? TEXT("Failed to add enter condition") : AddError);
    }

    return CreateSuccessResponse(Params.StateName);
}

FString FAddEnterConditionCommand::GetCommandName() const
{
    return TEXT("add_enter_condition");
}

bool FAddEnterConditionCommand::ValidateParams(const FString& Parameters) const
{
    FAddEnterConditionParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FAddEnterConditionCommand::ParseParameters(const FString& JsonString, FAddEnterConditionParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("condition_struct_path"), OutParams.ConditionStructPath))
    {
        OutError = TEXT("Missing required 'condition_struct_path' parameter");
        return false;
    }

    const TSharedPtr<FJsonObject>* ConditionPropertiesObj;
    if (JsonObject->TryGetObjectField(TEXT("condition_properties"), ConditionPropertiesObj))
    {
        OutParams.ConditionProperties = *ConditionPropertiesObj;
    }

    return true;
}

FString FAddEnterConditionCommand::CreateSuccessResponse(const FString& StateName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Enter condition added successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddEnterConditionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
