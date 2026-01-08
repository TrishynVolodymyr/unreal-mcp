#include "Commands/Niagara/DuplicateNiagaraSystemCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FDuplicateNiagaraSystemCommand::FDuplicateNiagaraSystemCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FDuplicateNiagaraSystemCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SourceSystem;
    if (!JsonObject->TryGetStringField(TEXT("source_system"), SourceSystem))
    {
        return CreateErrorResponse(TEXT("Missing 'source_system' parameter"));
    }

    FString NewName;
    if (!JsonObject->TryGetStringField(TEXT("new_name"), NewName))
    {
        return CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
    }

    FString FolderPath;
    JsonObject->TryGetStringField(TEXT("folder_path"), FolderPath);

    FString Error;
    FString NewPath;
    if (!NiagaraService.DuplicateSystem(SourceSystem, NewName, FolderPath, NewPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(NewName, NewPath);
}

bool FDuplicateNiagaraSystemCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SourceSystem, NewName;
    return JsonObject->TryGetStringField(TEXT("source_system"), SourceSystem) &&
           JsonObject->TryGetStringField(TEXT("new_name"), NewName);
}

FString FDuplicateNiagaraSystemCommand::CreateSuccessResponse(const FString& Name, const FString& Path) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), Name);
    ResponseObj->SetStringField(TEXT("path"), Path);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully duplicated Niagara system to '%s'"), *Path));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FDuplicateNiagaraSystemCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
