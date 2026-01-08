#include "Commands/Animation/LinkAnimationLayerCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FLinkAnimationLayerCommand::FLinkAnimationLayerCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FLinkAnimationLayerCommand::Execute(const FString& Parameters)
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

    FAnimLayerLinkParams Params;
    if (!JsonObject->TryGetStringField(TEXT("layer_interface"), Params.LayerInterfaceName))
    {
        return CreateErrorResponse(TEXT("Missing required 'layer_interface' parameter"));
    }

    JsonObject->TryGetStringField(TEXT("layer_class"), Params.LayerClassName);

    FString Error;
    if (!Service.LinkAnimationLayer(AnimBlueprint, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.LayerInterfaceName);
}

FString FLinkAnimationLayerCommand::GetCommandName() const
{
    return TEXT("link_animation_layer");
}

bool FLinkAnimationLayerCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, LayerInterface;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("layer_interface"), LayerInterface);
}

FString FLinkAnimationLayerCommand::CreateSuccessResponse(const FString& LayerName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("layer"), LayerName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully linked animation layer '%s'"), *LayerName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FLinkAnimationLayerCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
