#include "Commands/Animation/ConfigureAnimSlotCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FConfigureAnimSlotCommand::FConfigureAnimSlotCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FConfigureAnimSlotCommand::Execute(const FString& Parameters)
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

    FString SlotName;
    if (!JsonObject->TryGetStringField(TEXT("slot_name"), SlotName))
    {
        return CreateErrorResponse(TEXT("Missing required 'slot_name' parameter"));
    }

    UAnimBlueprint* AnimBlueprint = Service.FindAnimBlueprint(AnimBlueprintName);
    if (!AnimBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Blueprint '%s' not found"), *AnimBlueprintName));
    }

    FString SlotGroupName;
    JsonObject->TryGetStringField(TEXT("slot_group"), SlotGroupName);

    FString Error;
    if (!Service.ConfigureAnimSlot(AnimBlueprint, SlotName, SlotGroupName, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(SlotName);
}

FString FConfigureAnimSlotCommand::GetCommandName() const
{
    return TEXT("configure_anim_slot");
}

bool FConfigureAnimSlotCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, SlotName;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("slot_name"), SlotName);
}

FString FConfigureAnimSlotCommand::CreateSuccessResponse(const FString& SlotName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("slot"), SlotName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully configured slot '%s'"), *SlotName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FConfigureAnimSlotCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
