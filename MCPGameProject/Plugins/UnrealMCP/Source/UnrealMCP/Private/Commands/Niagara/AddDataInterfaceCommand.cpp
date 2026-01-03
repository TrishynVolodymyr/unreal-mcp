#include "Commands/Niagara/AddDataInterfaceCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FAddDataInterfaceCommand::FAddDataInterfaceCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FAddDataInterfaceCommand::Execute(const FString& Parameters)
{
    FNiagaraDataInterfaceParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString InterfaceId;
    if (!NiagaraService.AddDataInterface(Params, InterfaceId, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(InterfaceId);
}

FString FAddDataInterfaceCommand::GetCommandName() const
{
    return TEXT("add_data_interface");
}

bool FAddDataInterfaceCommand::ValidateParams(const FString& Parameters) const
{
    FNiagaraDataInterfaceParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FAddDataInterfaceCommand::ParseParameters(const FString& JsonString, FNiagaraDataInterfaceParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("interface_type"), OutParams.InterfaceType))
    {
        OutError = TEXT("Missing 'interface_type' parameter");
        return false;
    }

    // Optional
    JsonObject->TryGetStringField(TEXT("interface_name"), OutParams.InterfaceName);

    return OutParams.IsValid(OutError);
}

FString FAddDataInterfaceCommand::CreateSuccessResponse(const FString& InterfaceId) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("interface_id"), InterfaceId);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Data interface '%s' added successfully"), *InterfaceId));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FAddDataInterfaceCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
