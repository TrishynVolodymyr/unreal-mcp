#include "Commands/Material/GetMaterialExpressionMetadataCommand.h"
#include "Services/MaterialExpressionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetMaterialExpressionMetadataCommand::FGetMaterialExpressionMetadataCommand()
{
}

FString FGetMaterialExpressionMetadataCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString MaterialPath;
    if (!JsonObject->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    // Parse optional fields array
    TArray<FString> Fields;
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
    if (JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *FieldsArray)
        {
            Fields.Add(Value->AsString());
        }
    }

    TSharedPtr<FJsonObject> Metadata;
    bool bSuccess = FMaterialExpressionService::Get().GetGraphMetadata(
        MaterialPath,
        Fields.Num() > 0 ? &Fields : nullptr,
        Metadata);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Metadata.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetMaterialExpressionMetadataCommand::GetCommandName() const
{
    return TEXT("get_material_expression_metadata");
}

bool FGetMaterialExpressionMetadataCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonObject->HasField(TEXT("material_path"));
}

FString FGetMaterialExpressionMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
