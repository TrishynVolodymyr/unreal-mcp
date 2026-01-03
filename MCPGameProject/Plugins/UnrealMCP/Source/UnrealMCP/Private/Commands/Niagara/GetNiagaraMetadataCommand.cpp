#include "Commands/Niagara/GetNiagaraMetadataCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetNiagaraMetadataCommand::FGetNiagaraMetadataCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetNiagaraMetadataCommand::Execute(const FString& Parameters)
{
    FGetMetadataParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> Metadata;
    const TArray<FString>* FieldsPtr = Params.Fields.Num() > 0 ? &Params.Fields : nullptr;

    bool bSuccess = NiagaraService.GetMetadata(Params.AssetPath, FieldsPtr, Metadata);

    if (!bSuccess || !Metadata.IsValid())
    {
        FString ErrorMsg;
        if (Metadata.IsValid() && Metadata->HasField(TEXT("error")))
        {
            ErrorMsg = Metadata->GetStringField(TEXT("error"));
        }
        else
        {
            ErrorMsg = TEXT("Failed to get metadata");
        }
        return CreateErrorResponse(ErrorMsg);
    }

    // The metadata object already has success set, serialize it
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Metadata.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetNiagaraMetadataCommand::GetCommandName() const
{
    return TEXT("get_niagara_metadata");
}

bool FGetNiagaraMetadataCommand::ValidateParams(const FString& Parameters) const
{
    FGetMetadataParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FGetNiagaraMetadataCommand::ParseParameters(const FString& JsonString, FGetMetadataParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("asset_path"), OutParams.AssetPath))
    {
        OutError = TEXT("Missing 'asset_path' parameter");
        return false;
    }

    // Parse optional fields array
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray) && FieldsArray)
    {
        for (const TSharedPtr<FJsonValue>& Value : *FieldsArray)
        {
            FString FieldName;
            if (Value->TryGetString(FieldName))
            {
                OutParams.Fields.Add(FieldName);
            }
        }
    }

    if (OutParams.AssetPath.IsEmpty())
    {
        OutError = TEXT("Asset path cannot be empty");
        return false;
    }

    return true;
}

FString FGetNiagaraMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
