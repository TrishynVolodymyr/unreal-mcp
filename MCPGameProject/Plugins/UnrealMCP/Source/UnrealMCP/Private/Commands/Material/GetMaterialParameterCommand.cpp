#include "Commands/Material/GetMaterialParameterCommand.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetMaterialParameterCommand::FGetMaterialParameterCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FGetMaterialParameterCommand::Execute(const FString& Parameters)
{
    FParameterGetRequest Request;
    FString Error;

    if (!ParseParameters(Parameters, Request, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonValue> ResultValue;
    bool bSuccess = false;

    if (Request.ParameterType.Equals(TEXT("scalar"), ESearchCase::IgnoreCase))
    {
        float Value = 0.0f;
        bSuccess = MaterialService.GetScalarParameter(Request.MaterialPath, Request.ParameterName, Value, Error);
        if (bSuccess)
        {
            ResultValue = MakeShared<FJsonValueNumber>(Value);
        }
    }
    else if (Request.ParameterType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
    {
        FLinearColor Value;
        bSuccess = MaterialService.GetVectorParameter(Request.MaterialPath, Request.ParameterName, Value, Error);
        if (bSuccess)
        {
            TArray<TSharedPtr<FJsonValue>> ColorArray;
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.R));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.G));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.B));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.A));
            ResultValue = MakeShared<FJsonValueArray>(ColorArray);
        }
    }
    else if (Request.ParameterType.Equals(TEXT("texture"), ESearchCase::IgnoreCase))
    {
        FString TexturePath;
        bSuccess = MaterialService.GetTextureParameter(Request.MaterialPath, Request.ParameterName, TexturePath, Error);
        if (bSuccess)
        {
            ResultValue = MakeShared<FJsonValueString>(TexturePath);
        }
    }
    else
    {
        Error = FString::Printf(TEXT("Unknown parameter type: %s. Use 'scalar', 'vector', or 'texture'."), *Request.ParameterType);
    }

    if (!bSuccess)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(Request.ParameterName, Request.ParameterType, ResultValue);
}

FString FGetMaterialParameterCommand::GetCommandName() const
{
    return TEXT("get_material_parameter");
}

bool FGetMaterialParameterCommand::ValidateParams(const FString& Parameters) const
{
    FParameterGetRequest Request;
    FString Error;
    return ParseParameters(Parameters, Request, Error);
}

bool FGetMaterialParameterCommand::ParseParameters(const FString& JsonString, FParameterGetRequest& OutRequest, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("material_path"), OutRequest.MaterialPath))
    {
        OutError = TEXT("Missing 'material_path' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("parameter_name"), OutRequest.ParameterName))
    {
        OutError = TEXT("Missing 'parameter_name' parameter");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("parameter_type"), OutRequest.ParameterType))
    {
        OutError = TEXT("Missing 'parameter_type' parameter");
        return false;
    }

    return true;
}

FString FGetMaterialParameterCommand::CreateSuccessResponse(const FString& ParameterName, const FString& ParameterType, const TSharedPtr<FJsonValue>& Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("parameter_name"), ParameterName);
    ResponseObj->SetStringField(TEXT("parameter_type"), ParameterType);
    ResponseObj->SetField(TEXT("value"), Value);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetMaterialParameterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
