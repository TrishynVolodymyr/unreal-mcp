#include "Commands/Niagara/RemoveModuleFromEmitterCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FRemoveModuleFromEmitterCommand::FRemoveModuleFromEmitterCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FRemoveModuleFromEmitterCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleRemoveParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!NiagaraService.RemoveModule(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.ModuleName);
}

FString FRemoveModuleFromEmitterCommand::GetCommandName() const
{
    return TEXT("remove_module_from_emitter");
}

bool FRemoveModuleFromEmitterCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleRemoveParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FRemoveModuleFromEmitterCommand::ParseParameters(const FString& JsonString, FNiagaraModuleRemoveParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("module_name"), OutParams.ModuleName))
    {
        OutError = TEXT("Missing 'module_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("stage"), OutParams.Stage))
    {
        OutError = TEXT("Missing 'stage' parameter");
        return false;
    }

    return OutParams.IsValid(OutError);
}

FString FRemoveModuleFromEmitterCommand::CreateSuccessResponse(const FString& ModuleName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("module_name"), ModuleName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Module '%s' removed successfully"), *ModuleName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FRemoveModuleFromEmitterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
