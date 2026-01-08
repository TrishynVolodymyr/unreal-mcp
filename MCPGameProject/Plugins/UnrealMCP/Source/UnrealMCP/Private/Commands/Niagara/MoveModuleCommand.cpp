#include "Commands/Niagara/MoveModuleCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FMoveModuleCommand::FMoveModuleCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FMoveModuleCommand::Execute(const FString& Parameters)
{
    FNiagaraModuleMoveParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    if (!NiagaraService.MoveModule(Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Params.ModuleName, Params.NewIndex);
}

FString FMoveModuleCommand::GetCommandName() const
{
    return TEXT("move_module");
}

bool FMoveModuleCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraModuleMoveParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FMoveModuleCommand::ParseParameters(const FString& JsonString, FNiagaraModuleMoveParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetNumberField(TEXT("new_index"), OutParams.NewIndex))
    {
        OutError = TEXT("Missing 'new_index' parameter");
        return false;
    }

    return OutParams.IsValid(OutError);
}

FString FMoveModuleCommand::CreateSuccessResponse(const FString& ModuleName, int32 NewIndex) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("module_name"), ModuleName);
    ResponseObj->SetNumberField(TEXT("new_index"), NewIndex);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Module '%s' moved to index %d"), *ModuleName, NewIndex));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FMoveModuleCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
