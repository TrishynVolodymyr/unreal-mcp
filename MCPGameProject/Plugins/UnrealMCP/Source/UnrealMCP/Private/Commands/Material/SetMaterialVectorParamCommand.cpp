#include "Commands/Material/SetMaterialVectorParamCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetMaterialVectorParamCommand::FSetMaterialVectorParamCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FSetMaterialVectorParamCommand::Execute(const FString& Parameters)
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

    // Python sends value as [R, G, B, A] array
    const TArray<TSharedPtr<FJsonValue>>* ValueArray;
    if (!JsonObject->TryGetArrayField(TEXT("value"), ValueArray))
    {
        return CreateErrorResponse(TEXT("Missing or invalid 'value' parameter (expected array [R, G, B, A])"));
    }

    if (ValueArray->Num() < 3)
    {
        return CreateErrorResponse(TEXT("'value' array must have at least 3 elements [R, G, B]"));
    }

    FLinearColor Color;
    Color.R = static_cast<float>((*ValueArray)[0]->AsNumber());
    Color.G = static_cast<float>((*ValueArray)[1]->AsNumber());
    Color.B = static_cast<float>((*ValueArray)[2]->AsNumber());
    Color.A = ValueArray->Num() >= 4 ? static_cast<float>((*ValueArray)[3]->AsNumber()) : 1.0f;

    FString Error;
    if (!MaterialService.SetVectorParameter(MaterialPath, ParameterName, Color, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(MaterialPath, ParameterName, Color);
}

bool FSetMaterialVectorParamCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MaterialPath;
    FString ParameterName;
    const TArray<TSharedPtr<FJsonValue>>* ValueArray;

    return JsonObject->TryGetStringField(TEXT("material_instance"), MaterialPath) &&
           JsonObject->TryGetStringField(TEXT("parameter_name"), ParameterName) &&
           JsonObject->TryGetArrayField(TEXT("value"), ValueArray) &&
           ValueArray->Num() >= 3;
}

FString FSetMaterialVectorParamCommand::CreateSuccessResponse(const FString& MaterialPath, const FString& ParameterName, const FLinearColor& Value) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("material_instance"), MaterialPath);
    ResponseObj->SetStringField(TEXT("param_name"), ParameterName);

    TArray<TSharedPtr<FJsonValue>> ColorArray;
    ColorArray.Add(MakeShared<FJsonValueNumber>(Value.R));
    ColorArray.Add(MakeShared<FJsonValueNumber>(Value.G));
    ColorArray.Add(MakeShared<FJsonValueNumber>(Value.B));
    ColorArray.Add(MakeShared<FJsonValueNumber>(Value.A));
    ResponseObj->SetArrayField(TEXT("value"), ColorArray);

    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Vector parameter '%s' set successfully"), *ParameterName));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMaterialVectorParamCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
