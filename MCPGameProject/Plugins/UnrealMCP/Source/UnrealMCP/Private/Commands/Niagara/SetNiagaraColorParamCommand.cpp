#include "Commands/Niagara/SetNiagaraColorParamCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetNiagaraColorParamCommand::FSetNiagaraColorParamCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetNiagaraColorParamCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SystemPath;
    if (!JsonObject->TryGetStringField(TEXT("system"), SystemPath))
    {
        return CreateErrorResponse(TEXT("Missing 'system' parameter"));
    }

    FString ParamName;
    if (!JsonObject->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    double R, G, B;
    double A = 1.0; // Default alpha

    if (!JsonObject->TryGetNumberField(TEXT("r"), R))
    {
        return CreateErrorResponse(TEXT("Missing 'r' parameter"));
    }
    if (!JsonObject->TryGetNumberField(TEXT("g"), G))
    {
        return CreateErrorResponse(TEXT("Missing 'g' parameter"));
    }
    if (!JsonObject->TryGetNumberField(TEXT("b"), B))
    {
        return CreateErrorResponse(TEXT("Missing 'b' parameter"));
    }
    // Alpha is optional
    JsonObject->TryGetNumberField(TEXT("a"), A);

    // Create JSON array value for the color
    TArray<TSharedPtr<FJsonValue>> ColorArray;
    ColorArray.Add(MakeShared<FJsonValueNumber>(R));
    ColorArray.Add(MakeShared<FJsonValueNumber>(G));
    ColorArray.Add(MakeShared<FJsonValueNumber>(B));
    ColorArray.Add(MakeShared<FJsonValueNumber>(A));
    TSharedPtr<FJsonValue> JsonValue = MakeShared<FJsonValueArray>(ColorArray);

    FString Error;
    if (!NiagaraService.SetParameter(SystemPath, ParamName, JsonValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ParamName, FLinearColor(R, G, B, A));
}

bool FSetNiagaraColorParamCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SystemPath, ParamName;
    double R, G, B;
    return JsonObject->TryGetStringField(TEXT("system"), SystemPath) &&
           JsonObject->TryGetStringField(TEXT("param_name"), ParamName) &&
           JsonObject->TryGetNumberField(TEXT("r"), R) &&
           JsonObject->TryGetNumberField(TEXT("g"), G) &&
           JsonObject->TryGetNumberField(TEXT("b"), B);
}

FString FSetNiagaraColorParamCommand::CreateSuccessResponse(const FString& ParamName, const FLinearColor& Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("param_name"), ParamName);

    TArray<TSharedPtr<FJsonValue>> ValueArray;
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.R));
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.G));
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.B));
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.A));
    ResponseObj->SetArrayField(TEXT("value"), ValueArray);

    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Set color parameter '%s' to (%f, %f, %f, %f)"), *ParamName, Value.R, Value.G, Value.B, Value.A));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetNiagaraColorParamCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
