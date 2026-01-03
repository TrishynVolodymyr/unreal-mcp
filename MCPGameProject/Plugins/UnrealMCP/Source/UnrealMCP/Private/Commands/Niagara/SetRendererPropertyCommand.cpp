#include "Commands/Niagara/SetRendererPropertyCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetRendererPropertyCommand::FSetRendererPropertyCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetRendererPropertyCommand::Execute(const FString& Parameters)
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

    FString RendererName;
    if (!JsonObject->TryGetStringField(TEXT("renderer_name"), RendererName))
    {
        return CreateErrorResponse(TEXT("Missing 'renderer_name' parameter"));
    }

    FString PropertyName;
    if (!JsonObject->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    FString PropertyValue;
    if (!JsonObject->TryGetStringField(TEXT("property_value"), PropertyValue))
    {
        return CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueString>(PropertyValue);
    FString Error;

    if (!NiagaraService.SetRendererProperty(SystemPath, EmitterName, RendererName, PropertyName, Value, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(PropertyName);
}

FString FSetRendererPropertyCommand::GetCommandName() const
{
    return TEXT("set_renderer_property");
}

bool FSetRendererPropertyCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("system_path")) &&
           JsonObject->HasField(TEXT("emitter_name")) &&
           JsonObject->HasField(TEXT("renderer_name")) &&
           JsonObject->HasField(TEXT("property_name")) &&
           JsonObject->HasField(TEXT("property_value"));
}

FString FSetRendererPropertyCommand::CreateSuccessResponse(const FString& PropertyName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("property_name"), PropertyName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Renderer property '%s' set successfully"), *PropertyName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetRendererPropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
