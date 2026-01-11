#include "Commands/Niagara/SetModuleStaticSwitchCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetModuleStaticSwitchCommand::FSetModuleStaticSwitchCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetModuleStaticSwitchCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleStaticSwitchParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!NiagaraService.SetModuleStaticSwitch(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.SwitchName, Params.Value);
}

FString FSetModuleStaticSwitchCommand::GetCommandName() const
{
    return TEXT("set_module_static_switch");
}

bool FSetModuleStaticSwitchCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleStaticSwitchParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetModuleStaticSwitchCommand::ParseParameters(const FString& JsonString, FNiagaraModuleStaticSwitchParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("switch_name"), OutParams.SwitchName))
    {
        OutError = TEXT("Missing 'switch_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("value"), OutParams.Value))
    {
        OutError = TEXT("Missing 'value' parameter");
        return false;
    }

    return OutParams.IsValid(OutError);
}

FString FSetModuleStaticSwitchCommand::CreateSuccessResponse(const FString& SwitchName, const FString& Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("switch_name"), SwitchName);
    ResponseObj->SetStringField(TEXT("value"), Value);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Static switch set successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetModuleStaticSwitchCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
