#include "Commands/Niagara/AddEmitterToSystemCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FAddEmitterToSystemCommand::FAddEmitterToSystemCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FAddEmitterToSystemCommand::Execute(const FString& Parameters)
{
    FAddEmitterParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FGuid EmitterHandleId;
    bool bSuccess = NiagaraService.AddEmitterToSystem(
        Params.SystemPath,
        Params.EmitterPath,
        Params.EmitterName,
        EmitterHandleId,
        Error
    );

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(EmitterHandleId);
}

FString FAddEmitterToSystemCommand::GetCommandName() const
{
    return TEXT("add_emitter_to_system");
}

bool FAddEmitterToSystemCommand::ValidateParams(const FString& Parameters) const
{
    FAddEmitterParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FAddEmitterToSystemCommand::ParseParameters(const FString& JsonString, FAddEmitterParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("emitter_path"), OutParams.EmitterPath))
    {
        OutError = TEXT("Missing 'emitter_path' parameter");
        return false;
    }

    // Optional emitter name
    JsonObject->TryGetStringField(TEXT("emitter_name"), OutParams.EmitterName);

    if (OutParams.SystemPath.IsEmpty())
    {
        OutError = TEXT("System path cannot be empty");
        return false;
    }

    if (OutParams.EmitterPath.IsEmpty())
    {
        OutError = TEXT("Emitter path cannot be empty");
        return false;
    }

    return true;
}

FString FAddEmitterToSystemCommand::CreateSuccessResponse(const FGuid& EmitterHandleId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("emitter_handle_id"), EmitterHandleId.ToString());
    ResponseObj->SetStringField(TEXT("message"), TEXT("Emitter added to system successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddEmitterToSystemCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
