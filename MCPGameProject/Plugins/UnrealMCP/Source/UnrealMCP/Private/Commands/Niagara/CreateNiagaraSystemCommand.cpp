#include "Commands/Niagara/CreateNiagaraSystemCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateNiagaraSystemCommand::FCreateNiagaraSystemCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FCreateNiagaraSystemCommand::Execute(const FString& Parameters)
{
    FNiagaraSystemCreationParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString SystemPath;
    UNiagaraSystem* System = NiagaraService.CreateSystem(Params, SystemPath, Error);

    if (!System)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(SystemPath);
}

FString FCreateNiagaraSystemCommand::GetCommandName() const
{
    return TEXT("create_niagara_system");
}

bool FCreateNiagaraSystemCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraSystemCreationParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FCreateNiagaraSystemCommand::ParseParameters(const FString& JsonString, FNiagaraSystemCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    // Optional parameters with defaults - accept both 'path' and 'folder_path' for MCP compatibility
    if (!JsonObject->TryGetStringField(TEXT("path"), OutParams.Path))
    {
        JsonObject->TryGetStringField(TEXT("folder_path"), OutParams.Path);
    }
    JsonObject->TryGetStringField(TEXT("template"), OutParams.Template);

    return OutParams.IsValid(OutError);
}

FString FCreateNiagaraSystemCommand::CreateSuccessResponse(const FString& SystemPath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("system_path"), SystemPath);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Niagara System created at %s"), *SystemPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateNiagaraSystemCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
