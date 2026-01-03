#include "Commands/Niagara/SetModuleInputCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetModuleInputCommand::FSetModuleInputCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetModuleInputCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleInputParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!NiagaraService.SetModuleInput(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    // For now, return simple success - could enhance to return previous value
    return CreateSuccessResponse(TEXT(""), TEXT(""));
}

FString FSetModuleInputCommand::GetCommandName() const
{
    return TEXT("set_module_input");
}

bool FSetModuleInputCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleInputParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetModuleInputCommand::ParseParameters(const FString& JsonString, FNiagaraModuleInputParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("input_name"), OutParams.InputName))
    {
        OutError = TEXT("Missing 'input_name' parameter");
        return false;
    }

    // Get value as string - will be parsed by service based on type
    FString ValueStr;
    if (!JsonObject->TryGetStringField(TEXT("value"), ValueStr))
    {
        OutError = TEXT("Missing 'value' parameter");
        return false;
    }
    OutParams.Value = MakeShared<FJsonValueString>(ValueStr);

    // Optional value type hint
    JsonObject->TryGetStringField(TEXT("value_type"), OutParams.ValueType);

    return OutParams.IsValid(OutError);
}

FString FSetModuleInputCommand::CreateSuccessResponse(const FString& PreviousValue, const FString& NewValue) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    if (!PreviousValue.IsEmpty())
    {
        ResponseObj->SetStringField(TEXT("previous_value"), PreviousValue);
    }
    if (!NewValue.IsEmpty())
    {
        ResponseObj->SetStringField(TEXT("new_value"), NewValue);
    }
    ResponseObj->SetStringField(TEXT("message"), TEXT("Module input set successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetModuleInputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
