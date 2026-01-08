#include "Commands/Niagara/GetNiagaraSystemMetadataCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetNiagaraSystemMetadataCommand::FGetNiagaraSystemMetadataCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetNiagaraSystemMetadataCommand::Execute(const FString& Parameters)
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

    TSharedPtr<FJsonObject> ResultJson;
    if (!NiagaraService.GetMetadata(SystemPath, nullptr, ResultJson))
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to get metadata for system '%s'"), *SystemPath));
    }

    // Return the result JSON directly
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
    return OutputString;
}

bool FGetNiagaraSystemMetadataCommand::ValidateParams(const FString& Parameters) const
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

FString FGetNiagaraSystemMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
