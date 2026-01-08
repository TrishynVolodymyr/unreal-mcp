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

    // Check if module_inputs is requested
    bool bRequestModuleInputs = Params.Fields.Contains(TEXT("module_inputs"));

    if (bRequestModuleInputs)
    {
        // Validate required params for module_inputs
        if (Params.EmitterName.IsEmpty() || Params.ModuleName.IsEmpty() || Params.Stage.IsEmpty())
        {
            return CreateErrorResponse(TEXT("module_inputs field requires emitter_name, module_name, and stage parameters"));
        }

        TSharedPtr<FJsonObject> InputsMetadata;
        bool bSuccess = NiagaraService.GetModuleInputs(Params.AssetPath, Params.EmitterName, Params.ModuleName, Params.Stage, InputsMetadata);

        if (!bSuccess || !InputsMetadata.IsValid())
        {
            FString ErrorMsg;
            if (InputsMetadata.IsValid() && InputsMetadata->HasField(TEXT("error")))
            {
                ErrorMsg = InputsMetadata->GetStringField(TEXT("error"));
            }
            else
            {
                ErrorMsg = TEXT("Failed to get module inputs");
            }
            return CreateErrorResponse(ErrorMsg);
        }

        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(InputsMetadata.ToSharedRef(), Writer);
        return OutputString;
    }

    // Standard metadata request
    TSharedPtr<FJsonObject> Metadata;
    const TArray<FString>* FieldsPtr = Params.Fields.Num() > 0 ? &Params.Fields : nullptr;

    bool bSuccess = NiagaraService.GetMetadata(Params.AssetPath, FieldsPtr, Metadata, Params.EmitterName, Params.Stage);

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

    // Parse optional params for module_inputs field
    JsonObject->TryGetStringField(TEXT("emitter_name"), OutParams.EmitterName);
    JsonObject->TryGetStringField(TEXT("module_name"), OutParams.ModuleName);
    JsonObject->TryGetStringField(TEXT("stage"), OutParams.Stage);

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
