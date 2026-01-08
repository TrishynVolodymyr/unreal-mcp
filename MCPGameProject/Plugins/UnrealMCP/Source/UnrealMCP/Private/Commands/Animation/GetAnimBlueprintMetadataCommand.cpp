#include "Commands/Animation/GetAnimBlueprintMetadataCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetAnimBlueprintMetadataCommand::FGetAnimBlueprintMetadataCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FGetAnimBlueprintMetadataCommand::Execute(const FString& Parameters)
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

    UAnimBlueprint* AnimBlueprint = Service.FindAnimBlueprint(AnimBlueprintName);
    if (!AnimBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Blueprint '%s' not found"), *AnimBlueprintName));
    }

    TSharedPtr<FJsonObject> MetadataObj;
    if (!Service.GetAnimBlueprintMetadata(AnimBlueprint, MetadataObj))
    {
        return CreateErrorResponse(TEXT("Failed to retrieve Animation Blueprint metadata"));
    }

    // Wrap the metadata in a success response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), MetadataObj);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetAnimBlueprintMetadataCommand::GetCommandName() const
{
    return TEXT("get_anim_blueprint_metadata");
}

bool FGetAnimBlueprintMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName);
}

FString FGetAnimBlueprintMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
