#include "Commands/Material/ApplyMaterialToActorCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FApplyMaterialToActorCommand::FApplyMaterialToActorCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FApplyMaterialToActorCommand::Execute(const FString& Parameters)
{
    FApplyMaterialRequest Request;
    FString Error;

    if (!ParseParameters(Parameters, Request, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!MaterialService.ApplyMaterialToActor(Request.ActorName, Request.MaterialPath, Request.SlotIndex, Request.ComponentName, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Request.ActorName, Request.MaterialPath, Request.SlotIndex);
}

FString FApplyMaterialToActorCommand::GetCommandName() const
{
    return TEXT("apply_material_to_actor");
}

bool FApplyMaterialToActorCommand::ValidateParams(const FString& Parameters) const
{
    FApplyMaterialRequest Request;
    FString Error;
    return ParseParameters(Parameters, Request, Error);
}

bool FApplyMaterialToActorCommand::ParseParameters(const FString& JsonString, FApplyMaterialRequest& OutRequest, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("actor_name"), OutRequest.ActorName))
    {
        OutError = TEXT("Missing 'actor_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("material_path"), OutRequest.MaterialPath))
    {
        OutError = TEXT("Missing 'material_path' parameter");
        return false;
    }

    // Optional parameters
    JsonObject->TryGetNumberField(TEXT("slot_index"), OutRequest.SlotIndex);
    JsonObject->TryGetStringField(TEXT("component_name"), OutRequest.ComponentName);

    return true;
}

FString FApplyMaterialToActorCommand::CreateSuccessResponse(const FString& ActorName, const FString& MaterialPath, int32 SlotIndex) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("actor_name"), ActorName);
    ResponseObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResponseObj->SetNumberField(TEXT("slot_index"), SlotIndex);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Material applied to %s slot %d"), *ActorName, SlotIndex));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FApplyMaterialToActorCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
