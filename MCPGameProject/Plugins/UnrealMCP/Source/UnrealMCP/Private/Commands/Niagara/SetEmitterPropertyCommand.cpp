#include "Commands/Niagara/SetEmitterPropertyCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetEmitterPropertyCommand::FSetEmitterPropertyCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetEmitterPropertyCommand::Execute(const FString& Parameters)
{
    FNiagaraEmitterPropertyParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!NiagaraService.SetEmitterProperty(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.PropertyName, Params.PropertyValue);
}

FString FSetEmitterPropertyCommand::GetCommandName() const
{
    return TEXT("set_emitter_property");
}

bool FSetEmitterPropertyCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraEmitterPropertyParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetEmitterPropertyCommand::ParseParameters(const FString& JsonString, FNiagaraEmitterPropertyParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("emitter_name"), OutParams.EmitterName))
    {
        OutError = TEXT("Missing 'emitter_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("property_name"), OutParams.PropertyName))
    {
        OutError = TEXT("Missing 'property_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("property_value"), OutParams.PropertyValue))
    {
        OutError = TEXT("Missing 'property_value' parameter");
        return false;
    }

    return OutParams.IsValid(OutError);
}

FString FSetEmitterPropertyCommand::CreateSuccessResponse(const FString& PropertyName, const FString& PropertyValue) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("property_name"), PropertyName);
    ResponseObj->SetStringField(TEXT("property_value"), PropertyValue);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Emitter property '%s' set to '%s'"), *PropertyName, *PropertyValue));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetEmitterPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
