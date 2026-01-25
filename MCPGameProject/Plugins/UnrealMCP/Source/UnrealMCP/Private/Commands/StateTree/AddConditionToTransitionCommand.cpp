#include "Commands/StateTree/AddConditionToTransitionCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddConditionToTransitionCommand::FAddConditionToTransitionCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FAddConditionToTransitionCommand::Execute(const FString& Parameters)
{
    FAddConditionParams Params;
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
    if (!Service.AddConditionToTransition(Params, AddError))
    {
        return CreateErrorResponse(AddError.IsEmpty() ? TEXT("Failed to add condition") : AddError);
    }

    return CreateSuccessResponse(Params.SourceStateName, Params.TransitionIndex);
}

FString FAddConditionToTransitionCommand::GetCommandName() const
{
    return TEXT("add_condition_to_transition");
}

bool FAddConditionToTransitionCommand::ValidateParams(const FString& Parameters) const
{
    FAddConditionParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FAddConditionToTransitionCommand::ParseParameters(const FString& JsonString, FAddConditionParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("condition_struct_path"), OutParams.ConditionStructPath))
    {
        OutError = TEXT("Missing required 'condition_struct_path' parameter");
        return false;
    }

    int32 TransitionIndex;
    if (JsonObject->TryGetNumberField(TEXT("transition_index"), TransitionIndex))
    {
        OutParams.TransitionIndex = TransitionIndex;
    }

    JsonObject->TryGetStringField(TEXT("combine_mode"), OutParams.CombineMode);

    const TSharedPtr<FJsonObject>* ConditionPropertiesObj;
    if (JsonObject->TryGetObjectField(TEXT("condition_properties"), ConditionPropertiesObj))
    {
        OutParams.ConditionProperties = *ConditionPropertiesObj;
    }

    return true;
}

FString FAddConditionToTransitionCommand::CreateSuccessResponse(const FString& StateName, int32 TransitionIndex) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_name"), StateName);
    ResponseObj->SetNumberField(TEXT("transition_index"), TransitionIndex);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Condition added to transition successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddConditionToTransitionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
