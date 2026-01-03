#include "Commands/Niagara/CompileNiagaraAssetCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCompileNiagaraAssetCommand::FCompileNiagaraAssetCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FCompileNiagaraAssetCommand::Execute(const FString& Parameters)
{
    FCompileParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = NiagaraService.CompileAsset(Params.AssetPath, Error);

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse();
}

FString FCompileNiagaraAssetCommand::GetCommandName() const
{
    return TEXT("compile_niagara_asset");
}

bool FCompileNiagaraAssetCommand::ValidateParams(const FString& Parameters) const
{
    FCompileParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FCompileNiagaraAssetCommand::ParseParameters(const FString& JsonString, FCompileParams& OutParams, FString& OutError) const
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

    if (OutParams.AssetPath.IsEmpty())
    {
        OutError = TEXT("Asset path cannot be empty");
        return false;
    }

    return true;
}

FString FCompileNiagaraAssetCommand::CreateSuccessResponse() const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Asset compiled successfully"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCompileNiagaraAssetCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
