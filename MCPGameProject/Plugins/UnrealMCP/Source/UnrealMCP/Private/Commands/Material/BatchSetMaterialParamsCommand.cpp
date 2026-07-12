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
    TArray<FString> Failures;
    FString Error;

    bool bHasParameterObject = false;
    for (const TCHAR* FieldName : {TEXT("scalar_params"), TEXT("vector_params"), TEXT("texture_params")})
    {
        if (!JsonObject->HasField(FieldName))
        {
            continue;
        }
        bHasParameterObject = true;
        if (JsonObject->Values[FieldName]->Type != EJson::Object)
        {
            return CreateErrorResponse(FString::Printf(TEXT("'%s' must be a JSON object"), FieldName));
        }
    }
    if (!bHasParameterObject)
    {
        return CreateErrorResponse(TEXT("At least one of scalar_params, vector_params, or texture_params is required"));
    }

    // Process scalar parameters
    const TSharedPtr<FJsonObject>* ScalarParamsObj;
    if (JsonObject->TryGetObjectField(TEXT("scalar_params"), ScalarParamsObj))
    {
        for (const auto& Pair : (*ScalarParamsObj)->Values)
        {
            const FString Key = FString(Pair.Key.ToView());
            if (Pair.Value->Type != EJson::Number)
            {
                return CreateErrorResponse(FString::Printf(TEXT("scalar_params.%s must be a number"), *Key));
            }
            float Value = static_cast<float>(Pair.Value->AsNumber());
            if (MaterialService.SetScalarParameter(MaterialPath, Key, Value, Error))
            {
                SetScalarParams.Add(Key);
            }
            else
            {
                Failures.Add(FString::Printf(TEXT("scalar %s: %s"), *Key, *Error));
            }
        }
    }

    // Process vector parameters
    const TSharedPtr<FJsonObject>* VectorParamsObj;
    if (JsonObject->TryGetObjectField(TEXT("vector_params"), VectorParamsObj))
    {
        for (const auto& Pair : (*VectorParamsObj)->Values)
        {
            const FString Key = FString(Pair.Key.ToView());
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if (Pair.Value->TryGetArray(ColorArray) && ColorArray->Num() >= 3)
            {
                for (const TSharedPtr<FJsonValue>& Component : *ColorArray)
                {
                    if (Component->Type != EJson::Number)
                    {
                        return CreateErrorResponse(FString::Printf(TEXT("vector_params.%s components must be numbers"), *Key));
                    }
                }
                FLinearColor Color;
                Color.R = static_cast<float>((*ColorArray)[0]->AsNumber());
                Color.G = static_cast<float>((*ColorArray)[1]->AsNumber());
                Color.B = static_cast<float>((*ColorArray)[2]->AsNumber());
                Color.A = ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;

                if (MaterialService.SetVectorParameter(MaterialPath, Key, Color, Error))
                {
                    SetVectorParams.Add(Key);
                }
                else
                {
                    Failures.Add(FString::Printf(TEXT("vector %s: %s"), *Key, *Error));
                }
            }
            else
            {
                return CreateErrorResponse(FString::Printf(TEXT("vector_params.%s must be an array with at least 3 numbers"), *Key));
            }
        }
    }

    // Process texture parameters
    const TSharedPtr<FJsonObject>* TextureParamsObj;
    if (JsonObject->TryGetObjectField(TEXT("texture_params"), TextureParamsObj))
    {
        for (const auto& Pair : (*TextureParamsObj)->Values)
        {
            const FString Key = FString(Pair.Key.ToView());
            if (Pair.Value->Type != EJson::String)
            {
                return CreateErrorResponse(FString::Printf(TEXT("texture_params.%s must be an asset path string"), *Key));
            }
            FString TexturePath = Pair.Value->AsString();
            if (MaterialService.SetTextureParameter(MaterialPath, Key, TexturePath, Error))
            {
                SetTextureParams.Add(Key);
            }
            else
            {
                Failures.Add(FString::Printf(TEXT("texture %s: %s"), *Key, *Error));
            }
        }
    }

    if (!Failures.IsEmpty())
    {
        return CreateErrorResponse(FString::Join(Failures, TEXT("; ")));
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
