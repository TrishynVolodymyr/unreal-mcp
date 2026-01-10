#include "Commands/Niagara/GetEmitterModulesCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetEmitterModulesCommand::FGetEmitterModulesCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetEmitterModulesCommand::Execute(const FString& Parameters)
{
    FString SystemPath, EmitterName, Error;

    if (!ParseParameters(Parameters, SystemPath, EmitterName, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> ModulesResult;
    if (!NiagaraService.GetEmitterModules(SystemPath, EmitterName, ModulesResult))
    {
        FString ErrorMsg = ModulesResult.IsValid() ? ModulesResult->GetStringField(TEXT("error")) : TEXT("Unknown error");
        return CreateErrorResponse(ErrorMsg);
    }

    // The service already builds a complete JSON response
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ModulesResult.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetEmitterModulesCommand::GetCommandName() const
{
    return TEXT("get_emitter_modules");
}

bool FGetEmitterModulesCommand::ValidateParams(const FString& Parameters) const
{
    FString SystemPath, EmitterName, Error;
    return ParseParameters(Parameters, SystemPath, EmitterName, Error);
}

bool FGetEmitterModulesCommand::ParseParameters(const FString& JsonString, FString& OutSystemPath, FString& OutEmitterName, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("system_path"), OutSystemPath) || OutSystemPath.IsEmpty())
    {
        OutError = TEXT("system_path is required");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("emitter_name"), OutEmitterName) || OutEmitterName.IsEmpty())
    {
        OutError = TEXT("emitter_name is required");
        return false;
    }

    return true;
}

FString FGetEmitterModulesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
