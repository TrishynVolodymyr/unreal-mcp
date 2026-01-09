#include "Commands/Niagara/SetModuleRandomInputCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetModuleRandomInputCommand::FSetModuleRandomInputCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetModuleRandomInputCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleRandomInputParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = NiagaraService.SetModuleRandomInput(Params, Error);

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.ModuleName, Params.InputName, Params.MinValue, Params.MaxValue);
}

FString FSetModuleRandomInputCommand::GetCommandName() const
{
    return TEXT("set_module_random_input");
}

bool FSetModuleRandomInputCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleRandomInputParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FSetModuleRandomInputCommand::ParseParameters(const FString& JsonString, FNiagaraModuleRandomInputParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("min_value"), OutParams.MinValue))
    {
        OutError = TEXT("Missing 'min_value' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("max_value"), OutParams.MaxValue))
    {
        OutError = TEXT("Missing 'max_value' parameter");
        return false;
    }

    // Validate using the struct's validation
    return OutParams.IsValid(OutError);
}

FString FSetModuleRandomInputCommand::CreateSuccessResponse(const FString& ModuleName, const FString& InputName, const FString& MinValue, const FString& MaxValue) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("module_name"), ModuleName);
    ResponseObj->SetStringField(TEXT("input_name"), InputName);
    ResponseObj->SetStringField(TEXT("min_value"), MinValue);
    ResponseObj->SetStringField(TEXT("max_value"), MaxValue);
    ResponseObj->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Set random input '%s' on module '%s' with range [%s, %s]"),
            *InputName, *ModuleName, *MinValue, *MaxValue));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetModuleRandomInputCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
