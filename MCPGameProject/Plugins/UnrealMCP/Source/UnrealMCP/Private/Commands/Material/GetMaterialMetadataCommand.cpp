#include "Commands/Material/GetMaterialMetadataCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGetMaterialMetadataCommand::FGetMaterialMetadataCommand(IMaterialService& InMaterialService)
    : MaterialService(InMaterialService)
{
}

FString FGetMaterialMetadataCommand::Execute(const FString& Parameters)
{
    FString MaterialPath;
    TArray<FString> Fields;
    FString Error;

    if (!ParseParameters(Parameters, MaterialPath, Fields, Error))
    {
        return CreateErrorResponse(Error);
    }

    TSharedPtr<FJsonObject> Metadata;
    TArray<FString>* FieldsPtr = Fields.Num() > 0 ? &Fields : nullptr;

    if (!MaterialService.GetMaterialMetadata(MaterialPath, FieldsPtr, Metadata))
    {
        return CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    // Serialize the metadata JSON object
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Metadata.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetMaterialMetadataCommand::GetCommandName() const
{
    return TEXT("get_material_metadata");
}

bool FGetMaterialMetadataCommand::ValidateParams(const FString& Parameters) const
{
    FString MaterialPath;
    TArray<FString> Fields;
    FString Error;
    return ParseParameters(Parameters, MaterialPath, Fields, Error);
}

bool FGetMaterialMetadataCommand::ParseParameters(const FString& JsonString, FString& OutMaterialPath, TArray<FString>& OutFields, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("material_path"), OutMaterialPath))
    {
        OutError = TEXT("Missing 'material_path' parameter");
        return false;
    }

    // Optional fields array
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
    if (JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *FieldsArray)
        {
            FString Field;
            if (Value->TryGetString(Field))
            {
                OutFields.Add(Field);
            }
        }
    }

    return true;
}

FString FGetMaterialMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);

    return OutputString;
}
