#include "Commands/Niagara/AddNiagaraParameterCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FAddNiagaraParameterCommand::FAddNiagaraParameterCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FAddNiagaraParameterCommand::Execute(const FString& Parameters)
{
    FNiagaraParameterAddParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!NiagaraService.AddParameter(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.ParameterName);
}

FString FAddNiagaraParameterCommand::GetCommandName() const
{
    return TEXT("add_niagara_parameter");
}

bool FAddNiagaraParameterCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraParameterAddParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FAddNiagaraParameterCommand::ParseParameters(const FString& JsonString, FNiagaraParameterAddParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("system_path"), OutParams.SystemPath))
    {
        OutError = TEXT("Missing 'system_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("parameter_name"), OutParams.ParameterName))
    {
        OutError = TEXT("Missing 'parameter_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("parameter_type"), OutParams.ParameterType))
    {
        OutError = TEXT("Missing 'parameter_type' parameter");
        return false;
    }

    // Optional parameters
    JsonObject->TryGetStringField(TEXT("scope"), OutParams.Scope);

    FString DefaultValueStr;
    if (JsonObject->TryGetStringField(TEXT("default_value"), DefaultValueStr))
    {
        OutParams.DefaultValue = MakeShared<FJsonValueString>(DefaultValueStr);
    }

    return OutParams.IsValid(OutError);
}

FString FAddNiagaraParameterCommand::CreateSuccessResponse(const FString& ParameterName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("parameter_name"), ParameterName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Parameter '%s' added successfully"), *ParameterName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddNiagaraParameterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
