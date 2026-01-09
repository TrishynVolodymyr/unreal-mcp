#include "Commands/Niagara/SetEmitterEnabledCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetEmitterEnabledCommand::FSetEmitterEnabledCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetEmitterEnabledCommand::Execute(const FString& Parameters)
{
    FSetEmitterEnabledParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = NiagaraService.SetEmitterEnabled(
        Params.SystemPath,
        Params.EmitterName,
        Params.bEnabled,
        Error
    );

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.EmitterName, Params.bEnabled);
}

FString FSetEmitterEnabledCommand::GetCommandName() const
{
    return TEXT("set_emitter_enabled");
}

bool FSetEmitterEnabledCommand::ValidateParams(const FString& Parameters) const
{
    FSetEmitterEnabledParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetEmitterEnabledCommand::ParseParameters(const FString& JsonString, FSetEmitterEnabledParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Try both "system_path" and "system" for flexibility
    if (!JsonObject->TryGetStringField(TEXT("system_path"), OutParams.SystemPath))
    {
        if (!JsonObject->TryGetStringField(TEXT("system"), OutParams.SystemPath))
        {
            OutError = TEXT("Missing 'system_path' or 'system' parameter");
            return false;
        }
    }

    // Try both "emitter_name" and "emitter" for flexibility
    if (!JsonObject->TryGetStringField(TEXT("emitter_name"), OutParams.EmitterName))
    {
        if (!JsonObject->TryGetStringField(TEXT("emitter"), OutParams.EmitterName))
        {
            OutError = TEXT("Missing 'emitter_name' or 'emitter' parameter");
            return false;
        }
    }

    // Default to enabled=true if not specified
    if (!JsonObject->TryGetBoolField(TEXT("enabled"), OutParams.bEnabled))
    {
        OutParams.bEnabled = true;
    }

    if (OutParams.SystemPath.IsEmpty())
    {
        OutError = TEXT("System path cannot be empty");
        return false;
    }

    if (OutParams.EmitterName.IsEmpty())
    {
        OutError = TEXT("Emitter name cannot be empty");
        return false;
    }

    return true;
}

FString FSetEmitterEnabledCommand::CreateSuccessResponse(const FString& EmitterName, bool bEnabled) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("emitter"), EmitterName);
    ResponseObj->SetBoolField(TEXT("enabled"), bEnabled);
    ResponseObj->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Emitter '%s' %s successfully"),
            *EmitterName, bEnabled ? TEXT("enabled") : TEXT("disabled")));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetEmitterEnabledCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
