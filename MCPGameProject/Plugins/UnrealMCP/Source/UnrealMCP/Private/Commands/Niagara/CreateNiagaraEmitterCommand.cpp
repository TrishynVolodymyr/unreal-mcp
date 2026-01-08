#include "Commands/Niagara/CreateNiagaraEmitterCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateNiagaraEmitterCommand::FCreateNiagaraEmitterCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FCreateNiagaraEmitterCommand::Execute(const FString& Parameters)
{
    FNiagaraEmitterCreationParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString EmitterPath;
    UNiagaraEmitter* Emitter = NiagaraService.CreateEmitter(Params, EmitterPath, Error);

    if (!Emitter)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(EmitterPath);
}

FString FCreateNiagaraEmitterCommand::GetCommandName() const
{
    return TEXT("create_niagara_emitter");
}

bool FCreateNiagaraEmitterCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraEmitterCreationParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FCreateNiagaraEmitterCommand::ParseParameters(const FString& JsonString, FNiagaraEmitterCreationParams& OutParams, FString& OutError) const
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

FString FCreateNiagaraEmitterCommand::CreateSuccessResponse(const FString& EmitterPath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("emitter_path"), EmitterPath);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Niagara Emitter created at %s"), *EmitterPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateNiagaraEmitterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
