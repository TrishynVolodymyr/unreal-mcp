#include "Commands/Material/SetMaterialTextureParamCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FSetMaterialTextureParamCommand::FSetMaterialTextureParamCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FSetMaterialTextureParamCommand::Execute(const FString& Parameters)
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

    FString TexturePath;
    if (!JsonObject->TryGetStringField(TEXT("texture_path"), TexturePath))
    {
        return CreateErrorResponse(TEXT("Missing 'texture_path' parameter"));
    }

    FString Error;
    if (!MaterialService.SetTextureParameter(MaterialPath, ParameterName, TexturePath, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(MaterialPath, ParameterName, TexturePath);
}

bool FSetMaterialTextureParamCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MaterialPath;
    FString ParameterName;
    FString TexturePath;

    return JsonObject->TryGetStringField(TEXT("material_instance"), MaterialPath) &&
           JsonObject->TryGetStringField(TEXT("parameter_name"), ParameterName) &&
           JsonObject->TryGetStringField(TEXT("texture_path"), TexturePath);
}

FString FSetMaterialTextureParamCommand::CreateSuccessResponse(const FString& MaterialPath, const FString& ParameterName, const FString& TexturePath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("material_instance"), MaterialPath);
    ResponseObj->SetStringField(TEXT("param_name"), ParameterName);
    ResponseObj->SetStringField(TEXT("texture"), TexturePath);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Texture parameter '%s' set to '%s'"), *ParameterName, *TexturePath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FSetMaterialTextureParamCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
