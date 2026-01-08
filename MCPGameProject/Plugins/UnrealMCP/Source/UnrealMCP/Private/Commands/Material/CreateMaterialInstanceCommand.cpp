#include "Commands/Material/CreateMaterialInstanceCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateMaterialInstanceCommand::FCreateMaterialInstanceCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FCreateMaterialInstanceCommand::Execute(const FString& Parameters)
{
    FMaterialInstanceCreationParams Params;
    FString Error;

    if (!ParseParameters(Parameters, Params, Error))
    {
        return CreateErrorResponse(Error);
    }

    FString InstancePath;
    UMaterialInterface* Instance = MaterialService.CreateMaterialInstance(Params, InstancePath, Error);

    if (!Instance)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(InstancePath, Params.bIsDynamic, Params.ParentMaterialPath);
}

FString FCreateMaterialInstanceCommand::GetCommandName() const
{
    return TEXT("create_material_instance");
}

bool FCreateMaterialInstanceCommand::ValidateParams(const FString& Parameters) const
{
    FMaterialInstanceCreationParams Params;
    FString Error;
    return ParseParameters(Parameters, Params, Error);
}

bool FCreateMaterialInstanceCommand::ParseParameters(const FString& JsonString, FMaterialInstanceCreationParams& OutParams, FString& OutError) const
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

    if (!JsonObject->TryGetStringField(TEXT("parent_material"), OutParams.ParentMaterialPath))
    {
        OutError = TEXT("Missing 'parent_material' parameter");
        return false;
    }

    // Optional parameters - accept both 'path' and 'folder_path' for compatibility with MCP
    if (!JsonObject->TryGetStringField(TEXT("path"), OutParams.Path))
    {
        JsonObject->TryGetStringField(TEXT("folder_path"), OutParams.Path);
    }
    JsonObject->TryGetBoolField(TEXT("is_dynamic"), OutParams.bIsDynamic);

    return OutParams.IsValid(OutError);
}

FString FCreateMaterialInstanceCommand::CreateSuccessResponse(const FString& InstancePath, bool bIsDynamic, const FString& ParentPath) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("instance_path"), InstancePath);
    ResponseObj->SetStringField(TEXT("instance_type"), bIsDynamic ? TEXT("MaterialInstanceDynamic") : TEXT("MaterialInstanceConstant"));
    ResponseObj->SetStringField(TEXT("parent_material"), ParentPath);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Material instance created at %s"), *InstancePath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateMaterialInstanceCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
