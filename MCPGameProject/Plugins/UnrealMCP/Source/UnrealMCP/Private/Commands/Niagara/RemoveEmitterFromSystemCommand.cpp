#include "Commands/Niagara/RemoveEmitterFromSystemCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FRemoveEmitterFromSystemCommand::FRemoveEmitterFromSystemCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FRemoveEmitterFromSystemCommand::Execute(const FString& Parameters)
{
    FRemoveEmitterParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = NiagaraService.RemoveEmitterFromSystem(
        Params.SystemPath,
        Params.EmitterName,
        Error
    );

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.SystemPath, Params.EmitterName);
}

FString FRemoveEmitterFromSystemCommand::GetCommandName() const
{
    return TEXT("remove_emitter_from_system");
}

bool FRemoveEmitterFromSystemCommand::ValidateParams(const FString& Parameters) const
{
    FRemoveEmitterParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FRemoveEmitterFromSystemCommand::ParseParameters(const FString& JsonString, FRemoveEmitterParams& OutParams, FString& OutError) const
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

FString FRemoveEmitterFromSystemCommand::CreateSuccessResponse(const FString& SystemPath, const FString& EmitterName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("system"), SystemPath);
    ResponseObj->SetStringField(TEXT("emitter"), EmitterName);
    ResponseObj->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Emitter '%s' removed from system successfully"), *EmitterName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FRemoveEmitterFromSystemCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
