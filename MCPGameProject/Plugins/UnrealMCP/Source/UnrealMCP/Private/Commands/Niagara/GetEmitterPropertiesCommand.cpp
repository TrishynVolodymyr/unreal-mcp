#include "Commands/Niagara/GetEmitterPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetEmitterPropertiesCommand::FGetEmitterPropertiesCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetEmitterPropertiesCommand::Execute(const FString& Parameters)
{
    FString SystemPath, EmitterName, Error;

    if (!ParseParameters(Parameters, SystemPath, EmitterName, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> Properties;
    if (!NiagaraService.GetEmitterProperties(SystemPath, EmitterName, Properties, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Serialize the properties JSON to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Properties.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetEmitterPropertiesCommand::GetCommandName() const
{
    return TEXT("get_emitter_properties");
}

bool FGetEmitterPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    FString SystemPath, EmitterName, Error;
    return ParseParameters(Parameters, SystemPath, EmitterName, Error);
}

bool FGetEmitterPropertiesCommand::ParseParameters(const FString& JsonString, FString& OutSystemPath, FString& OutEmitterName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("system_path"), OutSystemPath))
    {
        OutError = TEXT("Missing 'system_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("emitter_name"), OutEmitterName))
    {
        OutError = TEXT("Missing 'emitter_name' parameter");
        return false;
    }

    return true;
}

FString FGetEmitterPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
