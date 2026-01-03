#include "Commands/Niagara/SetDataInterfacePropertyCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetDataInterfacePropertyCommand::FSetDataInterfacePropertyCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetDataInterfacePropertyCommand::Execute(const FString& Parameters)
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

    FString InterfaceName;
    if (!JsonObject->TryGetStringField(TEXT("interface_name"), InterfaceName))
    {
        return CreateErrorResponse(TEXT("Missing 'interface_name' parameter"));
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

    if (!NiagaraService.SetDataInterfaceProperty(SystemPath, EmitterName, InterfaceName, PropertyName, Value, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(PropertyName);
}

FString FSetDataInterfacePropertyCommand::GetCommandName() const
{
    return TEXT("set_data_interface_property");
}

bool FSetDataInterfacePropertyCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("system_path")) &&
           JsonObject->HasField(TEXT("emitter_name")) &&
           JsonObject->HasField(TEXT("interface_name")) &&
           JsonObject->HasField(TEXT("property_name")) &&
           JsonObject->HasField(TEXT("property_value"));
}

FString FSetDataInterfacePropertyCommand::CreateSuccessResponse(const FString& PropertyName) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("property_name"), PropertyName);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Property '%s' set successfully"), *PropertyName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetDataInterfacePropertyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
