#include "Commands/Niagara/GetModuleInputsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetModuleInputsCommand::FGetModuleInputsCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetModuleInputsCommand::Execute(const FString& Parameters)
{
    FString SystemPath, EmitterName, ModuleName, Stage, Error;

    if (!ParseParameters(Parameters, SystemPath, EmitterName, ModuleName, Stage, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> InputsResult;
    if (!NiagaraService.GetModuleInputs(SystemPath, EmitterName, ModuleName, Stage, InputsResult))
    {
        FString ErrorMsg = InputsResult.IsValid() ? InputsResult->GetStringField(TEXT("error")) : TEXT("Unknown error");
        return CreateErrorResponse(ErrorMsg);
    }

    // The service already builds a complete JSON response
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(InputsResult.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetModuleInputsCommand::GetCommandName() const
{
    return TEXT("get_module_inputs");
}

bool FGetModuleInputsCommand::ValidateParams(const FString& Parameters) const
{
    FString SystemPath, EmitterName, ModuleName, Stage, Error;
    return ParseParameters(Parameters, SystemPath, EmitterName, ModuleName, Stage, Error);
}

bool FGetModuleInputsCommand::ParseParameters(const FString& JsonString, FString& OutSystemPath, FString& OutEmitterName,
                                              FString& OutModuleName, FString& OutStage, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("module_name"), OutModuleName) || OutModuleName.IsEmpty())
    {
        OutError = TEXT("module_name is required");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("stage"), OutStage) || OutStage.IsEmpty())
    {
        OutError = TEXT("stage is required");
        return false;
    }

    return true;
}

FString FGetModuleInputsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
