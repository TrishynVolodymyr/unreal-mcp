#include "Commands/Niagara/CompileNiagaraSystemCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCompileNiagaraSystemCommand::FCompileNiagaraSystemCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FCompileNiagaraSystemCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SystemPath;
    if (!JsonObject->TryGetStringField(TEXT("system"), SystemPath))
    {
        return CreateErrorResponse(TEXT("Missing 'system' parameter"));
    }

    FString Error;
    if (!NiagaraService.CompileAsset(SystemPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(SystemPath);
}

bool FCompileNiagaraSystemCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SystemPath;
    return JsonObject->TryGetStringField(TEXT("system"), SystemPath);
}

FString FCompileNiagaraSystemCommand::CreateSuccessResponse(const FString& SystemName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("system"), SystemName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully compiled Niagara system '%s'"), *SystemName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FCompileNiagaraSystemCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
