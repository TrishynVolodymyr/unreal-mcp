#include "Commands/Animation/CreateAnimStateMachineCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FCreateAnimStateMachineCommand::FCreateAnimStateMachineCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FCreateAnimStateMachineCommand::Execute(const FString& Parameters)
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

    FString Error;
    if (!Service.CreateStateMachine(AnimBlueprint, StateMachineName, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(StateMachineName);
}

FString FCreateAnimStateMachineCommand::GetCommandName() const
{
    return TEXT("create_anim_state_machine");
}

bool FCreateAnimStateMachineCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, StateMachineName;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("state_machine_name"), StateMachineName);
}

FString FCreateAnimStateMachineCommand::CreateSuccessResponse(const FString& StateMachineName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state_machine"), StateMachineName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created state machine '%s'"), *StateMachineName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCreateAnimStateMachineCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
