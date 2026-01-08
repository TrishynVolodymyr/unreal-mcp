#include "Commands/Niagara/SetNiagaraFloatParamCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetNiagaraFloatParamCommand::FSetNiagaraFloatParamCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetNiagaraFloatParamCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SystemPath;
    if (!JsonObject->TryGetStringField(TEXT("system"), SystemPath))
    {
        return CreateErrorResponse(TEXT("Missing 'system' parameter"));
    }

    FString ParamName;
    if (!JsonObject->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    double Value;
    if (!JsonObject->TryGetNumberField(TEXT("value"), Value))
    {
        return CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    // Create JSON value for the float
    TSharedPtr<FJsonValue> JsonValue = MakeShared<FJsonValueNumber>(Value);

    FString Error;
    if (!NiagaraService.SetParameter(SystemPath, ParamName, JsonValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ParamName, static_cast<float>(Value));
}

bool FSetNiagaraFloatParamCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SystemPath, ParamName;
    double Value;
    return JsonObject->TryGetStringField(TEXT("system"), SystemPath) &&
           JsonObject->TryGetStringField(TEXT("param_name"), ParamName) &&
           JsonObject->TryGetNumberField(TEXT("value"), Value);
}

FString FSetNiagaraFloatParamCommand::CreateSuccessResponse(const FString& ParamName, float Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("param_name"), ParamName);
    ResponseObj->SetNumberField(TEXT("value"), Value);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Set float parameter '%s' to %f"), *ParamName, Value));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetNiagaraFloatParamCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
