#include "Commands/Material/CreateMaterialCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateMaterialCommand::FCreateMaterialCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FCreateMaterialCommand::Execute(const FString& Parameters)
{
    FMaterialCreationParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString MaterialPath;
    UMaterial* Material = MaterialService.CreateMaterial(Params, MaterialPath, Error);

    if (!Material)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(MaterialPath);
}

FString FCreateMaterialCommand::GetCommandName() const
{
    return TEXT("create_material");
}

bool FCreateMaterialCommand::ValidateParams(const FString& Parameters) const
{
    FMaterialCreationParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FCreateMaterialCommand::ParseParameters(const FString& JsonString, FMaterialCreationParams& OutParams, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("name"), OutParams.Name))
    {
        OutError = TEXT("Missing 'name' parameter");
        return false;
    }

    // Optional parameters with defaults
    JsonObject->TryGetStringField(TEXT("path"), OutParams.Path);
    JsonObject->TryGetStringField(TEXT("blend_mode"), OutParams.BlendMode);
    JsonObject->TryGetStringField(TEXT("shading_model"), OutParams.ShadingModel);

    return OutParams.IsValid(OutError);
}

FString FCreateMaterialCommand::CreateSuccessResponse(const FString& MaterialPath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Material created at %s"), *MaterialPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateMaterialCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
