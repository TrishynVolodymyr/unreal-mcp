#include "Commands/Animation/AddAnimTransitionCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddAnimTransitionCommand::FAddAnimTransitionCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FAddAnimTransitionCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString AnimBlueprintName;
    if (!JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName))
    {
        return CreateErrorResponse(TEXT("Missing required 'anim_blueprint_name' parameter"));
    }

    FString StateMachineName;
    if (!JsonObject->TryGetStringField(TEXT("state_machine_name"), StateMachineName))
    {
        return CreateErrorResponse(TEXT("Missing required 'state_machine_name' parameter"));
    }

    UAnimBlueprint* AnimBlueprint = Service.FindAnimBlueprint(AnimBlueprintName);
    if (!AnimBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Blueprint '%s' not found"), *AnimBlueprintName));
    }

    FAnimTransitionParams Params;
    if (!JsonObject->TryGetStringField(TEXT("from_state"), Params.FromStateName))
    {
        return CreateErrorResponse(TEXT("Missing required 'from_state' parameter"));
    }

    if (!JsonObject->TryGetStringField(TEXT("to_state"), Params.ToStateName))
    {
        return CreateErrorResponse(TEXT("Missing required 'to_state' parameter"));
    }

    JsonObject->TryGetStringField(TEXT("transition_rule_type"), Params.TransitionRuleType);

    double BlendDuration = 0.2;
    if (JsonObject->TryGetNumberField(TEXT("blend_duration"), BlendDuration))
    {
        Params.BlendDuration = static_cast<float>(BlendDuration);
    }

    JsonObject->TryGetStringField(TEXT("condition_variable"), Params.ConditionVariableName);

    FString Error;
    if (!Service.AddStateTransition(AnimBlueprint, StateMachineName, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.FromStateName, Params.ToStateName);
}

FString FAddAnimTransitionCommand::GetCommandName() const
{
    return TEXT("add_anim_transition");
}

bool FAddAnimTransitionCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, StateMachineName, FromState, ToState;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("state_machine_name"), StateMachineName) &&
           JsonObject->TryGetStringField(TEXT("from_state"), FromState) &&
           JsonObject->TryGetStringField(TEXT("to_state"), ToState);
}

FString FAddAnimTransitionCommand::CreateSuccessResponse(const FString& FromState, const FString& ToState) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("from_state"), FromState);
    ResponseObj->SetStringField(TEXT("to_state"), ToState);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added transition from '%s' to '%s'"), *FromState, *ToState));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddAnimTransitionCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
