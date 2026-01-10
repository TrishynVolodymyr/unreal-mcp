#include "Commands/Niagara/GetRendererPropertiesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetRendererPropertiesCommand::FGetRendererPropertiesCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetRendererPropertiesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SystemPath;
    if (!JsonObject->TryGetStringField(TEXT("system_path"), SystemPath))
    {
        return CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
    }

    FString EmitterName;
    if (!JsonObject->TryGetStringField(TEXT("emitter_name"), EmitterName))
    {
        return CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
    }

    FString RendererName = TEXT("Renderer");
    JsonObject->TryGetStringField(TEXT("renderer_name"), RendererName);

    TSharedPtr<FJsonObject> OutProperties;
    FString Error;

    if (!NiagaraService.GetRendererProperties(SystemPath, EmitterName, RendererName, OutProperties, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Serialize the response
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(OutProperties.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetRendererPropertiesCommand::GetCommandName() const
{
    return TEXT("get_renderer_properties");
}

bool FGetRendererPropertiesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("system_path")) &&
           JsonObject->HasField(TEXT("emitter_name"));
}

FString FGetRendererPropertiesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
