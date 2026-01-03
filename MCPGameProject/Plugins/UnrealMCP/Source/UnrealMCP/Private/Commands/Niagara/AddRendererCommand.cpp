#include "Commands/Niagara/AddRendererCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FAddRendererCommand::FAddRendererCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FAddRendererCommand::Execute(const FString& Parameters)
{
    FNiagaraRendererParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString RendererId;
    if (!NiagaraService.AddRenderer(Params, RendererId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(RendererId);
}

FString FAddRendererCommand::GetCommandName() const
{
    return TEXT("add_renderer");
}

bool FAddRendererCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraRendererParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FAddRendererCommand::ParseParameters(const FString& JsonString, FNiagaraRendererParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("renderer_type"), OutParams.RendererType))
    {
        OutError = TEXT("Missing 'renderer_type' parameter");
        return false;
    }

    // Optional
    JsonObject->TryGetStringField(TEXT("renderer_name"), OutParams.RendererName);

    return OutParams.IsValid(OutError);
}

FString FAddRendererCommand::CreateSuccessResponse(const FString& RendererId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("renderer_id"), RendererId);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Renderer '%s' added successfully"), *RendererId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddRendererCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
