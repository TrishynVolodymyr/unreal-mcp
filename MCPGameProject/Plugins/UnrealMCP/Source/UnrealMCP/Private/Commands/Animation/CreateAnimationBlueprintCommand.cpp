#include "Commands/Animation/CreateAnimationBlueprintCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FCreateAnimationBlueprintCommand::FCreateAnimationBlueprintCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FCreateAnimationBlueprintCommand::Execute(const FString& Parameters)
{
    FAnimBlueprintCreationParams Params;
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

    UAnimBlueprint* AnimBlueprint = Service.CreateAnimBlueprint(Params);
    if (!AnimBlueprint)
    {
        return CreateErrorResponse(TEXT("Failed to create Animation Blueprint"));
    }

    return CreateSuccessResponse(AnimBlueprint);
}

FString FCreateAnimationBlueprintCommand::GetCommandName() const
{
    return TEXT("create_animation_blueprint");
}

bool FCreateAnimationBlueprintCommand::ValidateParams(const FString& Parameters) const
{
    FAnimBlueprintCreationParams Params;
    FString ParseError;
    if (!ParseParameters(Parameters, Params, ParseError))
    {
        return false;
    }
    FString ValidationError;
    return Params.IsValid(ValidationError);
}

bool FCreateAnimationBlueprintCommand::ParseParameters(const FString& JsonString, FAnimBlueprintCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing required 'name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("skeleton_path"), OutParams.SkeletonPath))
    {
        OutError = TEXT("Missing required 'skeleton_path' parameter");
        return false;
    }

    JsonObject->TryGetStringField(TEXT("folder_path"), OutParams.FolderPath);
    JsonObject->TryGetStringField(TEXT("parent_class"), OutParams.ParentClassName);
    JsonObject->TryGetBoolField(TEXT("compile_on_creation"), OutParams.bCompileOnCreation);

    return true;
}

FString FCreateAnimationBlueprintCommand::CreateSuccessResponse(UAnimBlueprint* AnimBlueprint) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), AnimBlueprint->GetName());
    ResponseObj->SetStringField(TEXT("path"), AnimBlueprint->GetPathName());

    if (AnimBlueprint->TargetSkeleton)
    {
        ResponseObj->SetStringField(TEXT("skeleton"), AnimBlueprint->TargetSkeleton->GetPathName());
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCreateAnimationBlueprintCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
