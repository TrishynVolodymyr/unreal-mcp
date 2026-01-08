#include "Commands/Material/BatchSetMaterialParamsCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FBatchSetMaterialParamsCommand::FBatchSetMaterialParamsCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FBatchSetMaterialParamsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material_instance"), MaterialPath))
    {
        return CreateErrorResponse(TEXT("Missing 'material_instance' parameter"));
    }

    TArray<FString> SetScalarParams;
    TArray<FString> SetVectorParams;
    TArray<FString> SetTextureParams;
    FString Error;

    // Process scalar parameters
    const TSharedPtr<FJsonObject>* ScalarParamsObj;
    if (JsonObject->TryGetObjectField(TEXT("scalar_params"), ScalarParamsObj))
    {
        for (const auto& Pair : (*ScalarParamsObj)->Values)
        {
            float Value = static_cast<float>(Pair.Value->AsNumber());
            if (MaterialService.SetScalarParameter(MaterialPath, Pair.Key, Value, Error))
            {
                SetScalarParams.Add(Pair.Key);
            }
        }
    }

    // Process vector parameters
    const TSharedPtr<FJsonObject>* VectorParamsObj;
    if (JsonObject->TryGetObjectField(TEXT("vector_params"), VectorParamsObj))
    {
        for (const auto& Pair : (*VectorParamsObj)->Values)
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if (Pair.Value->TryGetArray(ColorArray) && ColorArray->Num() >= 3)
            {
                FLinearColor Color;
                Color.R = static_cast<float>((*ColorArray)[0]->AsNumber());
                Color.G = static_cast<float>((*ColorArray)[1]->AsNumber());
                Color.B = static_cast<float>((*ColorArray)[2]->AsNumber());
                Color.A = ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;

                if (MaterialService.SetVectorParameter(MaterialPath, Pair.Key, Color, Error))
                {
                    SetVectorParams.Add(Pair.Key);
                }
            }
        }
    }

    // Process texture parameters
    const TSharedPtr<FJsonObject>* TextureParamsObj;
    if (JsonObject->TryGetObjectField(TEXT("texture_params"), TextureParamsObj))
    {
        for (const auto& Pair : (*TextureParamsObj)->Values)
        {
            FString TexturePath = Pair.Value->AsString();
            if (MaterialService.SetTextureParameter(MaterialPath, Pair.Key, TexturePath, Error))
            {
                SetTextureParams.Add(Pair.Key);
            }
        }
    }

    return CreateSuccessResponse(MaterialPath, SetScalarParams, SetVectorParams, SetTextureParams);
}

bool FBatchSetMaterialParamsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MaterialPath;
    return JsonObject->TryGetStringField(TEXT("material_instance"), MaterialPath);
}

FString FBatchSetMaterialParamsCommand::CreateSuccessResponse(const FString& MaterialPath, const TArray<FString>& ScalarParams, const TArray<FString>& VectorParams, const TArray<FString>& TextureParams) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("material_instance"), MaterialPath);

    TSharedPtr<FJsonObject> ResultsObj = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> ScalarArray;
    for (const FString& Param : ScalarParams)
    {
        ScalarArray.Add(MakeShared<FJsonValueString>(Param));
    }
    ResultsObj->SetArrayField(TEXT("scalar"), ScalarArray);

    TArray<TSharedPtr<FJsonValue>> VectorArray;
    for (const FString& Param : VectorParams)
    {
        VectorArray.Add(MakeShared<FJsonValueString>(Param));
    }
    ResultsObj->SetArrayField(TEXT("vector"), VectorArray);

    TArray<TSharedPtr<FJsonValue>> TextureArray;
    for (const FString& Param : TextureParams)
    {
        TextureArray.Add(MakeShared<FJsonValueString>(Param));
    }
    ResultsObj->SetArrayField(TEXT("texture"), TextureArray);

    ResponseObj->SetObjectField(TEXT("results"), ResultsObj);
    ResponseObj->SetStringField(TEXT("message"), TEXT("Batch parameter update completed"));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FBatchSetMaterialParamsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
