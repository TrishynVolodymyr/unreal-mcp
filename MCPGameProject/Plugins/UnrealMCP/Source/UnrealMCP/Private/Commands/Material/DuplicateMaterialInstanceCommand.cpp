#include "Commands/Material/DuplicateMaterialInstanceCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FDuplicateMaterialInstanceCommand::FDuplicateMaterialInstanceCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FDuplicateMaterialInstanceCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SourcePath;
    if (!JsonObject->TryGetStringField(TEXT("source_material_instance"), SourcePath))
    {
        return CreateErrorResponse(TEXT("Missing 'source_material_instance' parameter"));
    }

    FString NewName;
    if (!JsonObject->TryGetStringField(TEXT("new_name"), NewName))
    {
        return CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
    }

    FString FolderPath;
    JsonObject->TryGetStringField(TEXT("folder_path"), FolderPath);

    FString OutAssetPath;
    FString OutParentMaterial;
    FString Error;

    if (!MaterialService.DuplicateMaterialInstance(SourcePath, NewName, FolderPath, OutAssetPath, OutParentMaterial, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(NewName, OutAssetPath, OutParentMaterial);
}

bool FDuplicateMaterialInstanceCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SourcePath;
    FString NewName;

    return JsonObject->TryGetStringField(TEXT("source_material_instance"), SourcePath) &&
           JsonObject->TryGetStringField(TEXT("new_name"), NewName);
}

FString FDuplicateMaterialInstanceCommand::CreateSuccessResponse(const FString& Name, const FString& Path, const FString& Parent) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("name"), Name);
    ResponseObj->SetStringField(TEXT("path"), Path);
    ResponseObj->SetStringField(TEXT("parent"), Parent);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully duplicated material instance to '%s'"), *Path));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FDuplicateMaterialInstanceCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
