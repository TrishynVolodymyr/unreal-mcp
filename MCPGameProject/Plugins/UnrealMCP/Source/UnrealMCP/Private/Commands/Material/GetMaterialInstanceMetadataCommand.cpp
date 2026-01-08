#include "Commands/Material/GetMaterialInstanceMetadataCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetMaterialInstanceMetadataCommand::FGetMaterialInstanceMetadataCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FGetMaterialInstanceMetadataCommand::Execute(const FString& Parameters)
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

    TSharedPtr<FJsonObject> OutMetadata;
    if (!MaterialService.GetMaterialMetadata(MaterialPath, nullptr, OutMetadata))
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to get metadata for material instance: %s"), *MaterialPath));
    }

    // The service returns the metadata, just add success flag
    OutMetadata->SetBoolField(TEXT("success"), true);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(OutMetadata.ToSharedRef(), Writer);

    return OutputString;
}

bool FGetMaterialInstanceMetadataCommand::ValidateParams(const FString& Parameters) const
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

FString FGetMaterialInstanceMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
