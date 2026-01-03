#include "Commands/Niagara/SetNiagaraParameterCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetNiagaraParameterCommand::FSetNiagaraParameterCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetNiagaraParameterCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SystemPath;
    if (!JsonObject->TryGetStringField(TEXT("system_path"), SystemPath))
    {
        return CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
    }

    FString ParameterName;
    if (!JsonObject->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
    }

    FString ValueStr;
    if (!JsonObject->TryGetStringField(TEXT("value"), ValueStr))
    {
        return CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueString>(ValueStr);
    FString Error;

    if (!NiagaraService.SetParameter(SystemPath, ParameterName, Value, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ParameterName, ValueStr);
}

FString FSetNiagaraParameterCommand::GetCommandName() const
{
    return TEXT("set_niagara_parameter");
}

bool FSetNiagaraParameterCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("system_path")) &&
           JsonObject->HasField(TEXT("parameter_name")) &&
           JsonObject->HasField(TEXT("value"));
}

FString FSetNiagaraParameterCommand::CreateSuccessResponse(const FString& ParameterName, const FString& Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("parameter_name"), ParameterName);
    ResponseObj->SetStringField(TEXT("new_value"), Value);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Parameter '%s' set to '%s'"), *ParameterName, *Value));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetNiagaraParameterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
