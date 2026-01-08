#include "Commands/Niagara/SetNiagaraVectorParamCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetNiagaraVectorParamCommand::FSetNiagaraVectorParamCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FSetNiagaraVectorParamCommand::Execute(const FString& Parameters)
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

    double X, Y, Z;
    if (!JsonObject->TryGetNumberField(TEXT("x"), X))
    {
        return CreateErrorResponse(TEXT("Missing 'x' parameter"));
    }
    if (!JsonObject->TryGetNumberField(TEXT("y"), Y))
    {
        return CreateErrorResponse(TEXT("Missing 'y' parameter"));
    }
    if (!JsonObject->TryGetNumberField(TEXT("z"), Z))
    {
        return CreateErrorResponse(TEXT("Missing 'z' parameter"));
    }

    // Create JSON array value for the vector
    TArray<TSharedPtr<FJsonValue>> VectorArray;
    VectorArray.Add(MakeShared<FJsonValueNumber>(X));
    VectorArray.Add(MakeShared<FJsonValueNumber>(Y));
    VectorArray.Add(MakeShared<FJsonValueNumber>(Z));
    TSharedPtr<FJsonValue> JsonValue = MakeShared<FJsonValueArray>(VectorArray);

    FString Error;
    if (!NiagaraService.SetParameter(SystemPath, ParamName, JsonValue, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(ParamName, FVector(X, Y, Z));
}

bool FSetNiagaraVectorParamCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SystemPath, ParamName;
    double X, Y, Z;
    return JsonObject->TryGetStringField(TEXT("system"), SystemPath) &&
           JsonObject->TryGetStringField(TEXT("param_name"), ParamName) &&
           JsonObject->TryGetNumberField(TEXT("x"), X) &&
           JsonObject->TryGetNumberField(TEXT("y"), Y) &&
           JsonObject->TryGetNumberField(TEXT("z"), Z);
}

FString FSetNiagaraVectorParamCommand::CreateSuccessResponse(const FString& ParamName, const FVector& Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("param_name"), ParamName);

    TArray<TSharedPtr<FJsonValue>> ValueArray;
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.X));
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.Y));
    ValueArray.Add(MakeShared<FJsonValueNumber>(Value.Z));
    ResponseObj->SetArrayField(TEXT("value"), ValueArray);

    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Set vector parameter '%s' to (%f, %f, %f)"), *ParamName, Value.X, Value.Y, Value.Z));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetNiagaraVectorParamCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
