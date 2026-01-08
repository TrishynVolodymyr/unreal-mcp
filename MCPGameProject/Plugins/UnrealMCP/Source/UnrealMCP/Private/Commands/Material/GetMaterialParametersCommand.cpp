#include "Commands/Material/GetMaterialParametersCommand.h"
#include "Services/IMaterialService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetMaterialParametersCommand::FGetMaterialParametersCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FGetMaterialParametersCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material"), MaterialPath))
    {
        return CreateErrorResponse(TEXT("Missing 'material' parameter"));
    }

    // Get all available fields including parameters
    TArray<FString> Fields;
    Fields.Add(TEXT("parameters"));

    TSharedPtr<FJsonObject> OutMetadata;
    if (!MaterialService.GetMaterialMetadata(MaterialPath, &Fields, OutMetadata))
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to get parameters for material: %s"), *MaterialPath));
    }

    // Add success flag and material path
    OutMetadata->SetBoolField(TEXT("success"), true);
    OutMetadata->SetStringField(TEXT("material"), MaterialPath);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(OutMetadata.ToSharedRef(), Writer);

    return OutputString;
}

bool FGetMaterialParametersCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString MaterialPath;
    return JsonObject->TryGetStringField(TEXT("material"), MaterialPath);
}

FString FGetMaterialParametersCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
