#include "Commands/Niagara/AddModuleToEmitterCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FAddModuleToEmitterCommand::FAddModuleToEmitterCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FAddModuleToEmitterCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleAddParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString ModuleId;
    if (!NiagaraService.AddModule(Params, ModuleId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ModuleId);
}

FString FAddModuleToEmitterCommand::GetCommandName() const
{
    return TEXT("add_module_to_emitter");
}

bool FAddModuleToEmitterCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleAddParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FAddModuleToEmitterCommand::ParseParameters(const FString& JsonString, FNiagaraModuleAddParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("module_path"), OutParams.ModulePath))
    {
        OutError = TEXT("Missing 'module_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("stage"), OutParams.Stage))
    {
        OutError = TEXT("Missing 'stage' parameter");
        return false;
    }

    // Optional: index defaults to -1 (append)
    if (!JsonObject->TryGetNumberField(TEXT("index"), OutParams.Index))
    {
        OutParams.Index = -1;
    }

    return OutParams.IsValid(OutError);
}

FString FAddModuleToEmitterCommand::CreateSuccessResponse(const FString& ModuleId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("node_id"), ModuleId);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Module added successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddModuleToEmitterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
