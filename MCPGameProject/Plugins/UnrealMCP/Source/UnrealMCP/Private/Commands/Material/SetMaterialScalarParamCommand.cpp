#include "Commands/Material/SetMaterialScalarParamCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetMaterialScalarParamCommand::FSetMaterialScalarParamCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FSetMaterialScalarParamCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Get material_instance (Python sends this, map to material_path)
    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material_instance"), MaterialPath))
    {
        return CreateErrorResponse(TEXT("Missing 'material_instance' parameter"));
    }

    FString ParameterName;
    if (!JsonObject->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
    }

    double Value = 0.0;
    if (!JsonObject->TryGetNumberField(TEXT("value"), Value))
    {
        return CreateErrorResponse(TEXT("Missing or invalid 'value' parameter"));
    }

    FString Error;
    if (!MaterialService.SetScalarParameter(MaterialPath, ParameterName, static_cast<float>(Value), Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(MaterialPath, ParameterName, static_cast<float>(Value));
}

bool FSetMaterialScalarParamCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MaterialPath;
    FString ParameterName;
    double Value;

    return JsonObject->TryGetStringField(TEXT("material_instance"), MaterialPath) &&
           JsonObject->TryGetStringField(TEXT("parameter_name"), ParameterName) &&
           JsonObject->TryGetNumberField(TEXT("value"), Value);
}

FString FSetMaterialScalarParamCommand::CreateSuccessResponse(const FString& MaterialPath, const FString& ParameterName, float Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("material_instance"), MaterialPath);
    ResponseObj->SetStringField(TEXT("param_name"), ParameterName);
    ResponseObj->SetNumberField(TEXT("value"), Value);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Scalar parameter '%s' set to %f"), *ParameterName, Value));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMaterialScalarParamCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
