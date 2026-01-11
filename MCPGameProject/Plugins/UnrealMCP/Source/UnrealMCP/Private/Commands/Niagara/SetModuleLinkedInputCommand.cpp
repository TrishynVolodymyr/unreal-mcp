#include "Commands/Niagara/SetModuleLinkedInputCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetModuleLinkedInputCommand::FSetModuleLinkedInputCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetModuleLinkedInputCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleLinkedInputParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = NiagaraService.SetModuleLinkedInput(Params, Error);

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.ModuleName, Params.InputName, Params.LinkedValue);
}

FString FSetModuleLinkedInputCommand::GetCommandName() const
{
    return TEXT("set_module_linked_input");
}

bool FSetModuleLinkedInputCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleLinkedInputParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetModuleLinkedInputCommand::ParseParameters(const FString& JsonString, FNiagaraModuleLinkedInputParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Required parameters
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

    if (!JsonObject->TryGetStringField(TEXT("input_name"), OutParams.InputName))
    {
        OutError = TEXT("Missing 'input_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("linked_value"), OutParams.LinkedValue))
    {
        OutError = TEXT("Missing 'linked_value' parameter");
        return false;
    }

    // Validate using the struct's validation
    return OutParams.IsValid(OutError);
}

FString FSetModuleLinkedInputCommand::CreateSuccessResponse(const FString& ModuleName, const FString& InputName, const FString& LinkedValue) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("module_name"), ModuleName);
    ResponseObj->SetStringField(TEXT("input_name"), InputName);
    ResponseObj->SetStringField(TEXT("linked_value"), LinkedValue);
    ResponseObj->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Set linked input '%s' on module '%s' to '%s'"),
            *InputName, *ModuleName, *LinkedValue));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetModuleLinkedInputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
