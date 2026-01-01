#include "Commands/Material/SetMaterialParameterCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetMaterialParameterCommand::FSetMaterialParameterCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FSetMaterialParameterCommand::Execute(const FString& Parameters)
{
    FParameterSetRequest Request;
    FString Error;

    if (!ParseParameters(Parameters, Request, Error))
    {
        return CreateErrorResponse(Error);
    }

    bool bSuccess = false;

    if (Request.ParameterType.Equals(TEXT("scalar"), ESearchCase::IgnoreCase))
    {
        float Value = 0.0f;
        if (Request.Value->TryGetNumber(Value))
        {
            bSuccess = MaterialService.SetScalarParameter(Request.MaterialPath, Request.ParameterName, Value, Error);
        }
        else
        {
            Error = TEXT("Invalid scalar value");
        }
    }
    else if (Request.ParameterType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
        if (Request.Value->TryGetArray(ArrayValue) && ArrayValue->Num() >= 3)
        {
            FLinearColor Color;
            Color.R = (*ArrayValue)[0]->AsNumber();
            Color.G = (*ArrayValue)[1]->AsNumber();
            Color.B = (*ArrayValue)[2]->AsNumber();
            Color.A = ArrayValue->Num() >= 4 ? (*ArrayValue)[3]->AsNumber() : 1.0f;

            bSuccess = MaterialService.SetVectorParameter(Request.MaterialPath, Request.ParameterName, Color, Error);
        }
        else
        {
            Error = TEXT("Invalid vector value. Expected array of [R, G, B] or [R, G, B, A]");
        }
    }
    else if (Request.ParameterType.Equals(TEXT("texture"), ESearchCase::IgnoreCase))
    {
        FString TexturePath;
        if (Request.Value->TryGetString(TexturePath))
        {
            bSuccess = MaterialService.SetTextureParameter(Request.MaterialPath, Request.ParameterName, TexturePath, Error);
        }
        else
        {
            Error = TEXT("Invalid texture value. Expected string path.");
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

    return CreateSuccessResponse(Request.ParameterName, Request.ParameterType);
}

FString FSetMaterialParameterCommand::GetCommandName() const
{
    return TEXT("set_material_parameter");
}

bool FSetMaterialParameterCommand::ValidateParams(const FString& Parameters) const
{
    FParameterSetRequest Request;
    FString Error;
    return ParseParameters(Parameters, Request, Error);
}

bool FSetMaterialParameterCommand::ParseParameters(const FString& JsonString, FParameterSetRequest& OutRequest, FString& OutError) const
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

    OutRequest.Value = JsonObject->TryGetField(TEXT("value"));
    if (!OutRequest.Value.IsValid())
    {
        OutError = TEXT("Missing 'value' parameter");
        return false;
    }

    return true;
}

FString FSetMaterialParameterCommand::CreateSuccessResponse(const FString& ParameterName, const FString& ParameterType) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("parameter_name"), ParameterName);
    ResponseObj->SetStringField(TEXT("parameter_type"), ParameterType);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Parameter '%s' set successfully"), *ParameterName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMaterialParameterCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
