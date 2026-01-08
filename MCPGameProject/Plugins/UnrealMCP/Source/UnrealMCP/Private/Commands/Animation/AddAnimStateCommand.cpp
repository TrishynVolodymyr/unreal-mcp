#include "Commands/Animation/AddAnimStateCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddAnimStateCommand::FAddAnimStateCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FAddAnimStateCommand::Execute(const FString& Parameters)
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

    FAnimStateParams Params;
    if (!JsonObject->TryGetStringField(TEXT("state_name"), Params.StateName))
    {
        return CreateErrorResponse(TEXT("Missing required 'state_name' parameter"));
    }

    JsonObject->TryGetStringField(TEXT("animation_asset_path"), Params.AnimationAssetPath);
    JsonObject->TryGetBoolField(TEXT("is_default_state"), Params.bIsDefaultState);

    double PosX = 0.0, PosY = 0.0;
    if (JsonObject->TryGetNumberField(TEXT("node_position_x"), PosX) &&
        JsonObject->TryGetNumberField(TEXT("node_position_y"), PosY))
    {
        Params.NodePosition = FVector2D(PosX, PosY);
    }

    FString Error;
    if (!Service.AddStateToStateMachine(AnimBlueprint, StateMachineName, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.StateName, StateMachineName);
}

FString FAddAnimStateCommand::GetCommandName() const
{
    return TEXT("add_anim_state");
}

bool FAddAnimStateCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, StateMachineName, StateName;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("state_machine_name"), StateMachineName) &&
           JsonObject->TryGetStringField(TEXT("state_name"), StateName);
}

FString FAddAnimStateCommand::CreateSuccessResponse(const FString& StateName, const FString& StateMachineName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("state"), StateName);
    ResponseObj->SetStringField(TEXT("state_machine"), StateMachineName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added state '%s' to state machine '%s'"), *StateName, *StateMachineName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddAnimStateCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
