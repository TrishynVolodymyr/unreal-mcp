#include "Commands/Animation/AddAnimVariableCommand.h"
#include "Animation/AnimBlueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FAddAnimVariableCommand::FAddAnimVariableCommand(IAnimationBlueprintService& InService)
    : Service(InService)
{
}

FString FAddAnimVariableCommand::Execute(const FString& Parameters)
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

    FString VariableName;
    if (!JsonObject->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return CreateErrorResponse(TEXT("Missing required 'variable_name' parameter"));
    }

    FString VariableType;
    if (!JsonObject->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return CreateErrorResponse(TEXT("Missing required 'variable_type' parameter"));
    }

    UAnimBlueprint* AnimBlueprint = Service.FindAnimBlueprint(AnimBlueprintName);
    if (!AnimBlueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Animation Blueprint '%s' not found"), *AnimBlueprintName));
    }

    FString DefaultValue;
    JsonObject->TryGetStringField(TEXT("default_value"), DefaultValue);

    FString Error;
    if (!Service.AddAnimVariable(AnimBlueprint, VariableName, VariableType, DefaultValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(VariableName);
}

FString FAddAnimVariableCommand::GetCommandName() const
{
    return TEXT("add_anim_variable");
}

bool FAddAnimVariableCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString AnimBlueprintName, VariableName, VariableType;
    return JsonObject->TryGetStringField(TEXT("anim_blueprint_name"), AnimBlueprintName) &&
           JsonObject->TryGetStringField(TEXT("variable_name"), VariableName) &&
           JsonObject->TryGetStringField(TEXT("variable_type"), VariableType);
}

FString FAddAnimVariableCommand::CreateSuccessResponse(const FString& VariableName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("variable"), VariableName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added variable '%s'"), *VariableName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FAddAnimVariableCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
