#include "Commands/Project/UpdateStructCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FUpdateStructCommand::FUpdateStructCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

FString FUpdateStructCommand::GetCommandName() const
{
    return TEXT("update_struct");
}

bool FUpdateStructCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString StructName;
    if (!JsonObject->TryGetStringField(TEXT("struct_name"), StructName) || StructName.IsEmpty())
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray = nullptr;
    if (!JsonObject->TryGetArrayField(TEXT("properties"), PropertiesArray) || !PropertiesArray || PropertiesArray->Num() == 0)
    {
        return false;
    }

    return true;
}

FString FUpdateStructCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid JSON parameters"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Validate parameters
    if (!ValidateParams(Parameters))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString StructName = JsonObject->GetStringField(TEXT("struct_name"));
    
    FString Path = TEXT("/Game/Blueprints");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString Description;
    JsonObject->TryGetStringField(TEXT("description"), Description);

    // Convert properties array
    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray = nullptr;
    JsonObject->TryGetArrayField(TEXT("properties"), PropertiesArray);
    
    TArray<TSharedPtr<FJsonObject>> Properties;
    if (PropertiesArray)
    {
        for (const TSharedPtr<FJsonValue>& PropertyValue : *PropertiesArray)
        {
            if (PropertyValue->Type == EJson::Object)
            {
                Properties.Add(PropertyValue->AsObject());
            }
        }
    }

    // Execute the operation using ProjectService
    FString Error;
    if (!ProjectService->UpdateStruct(StructName, Path, Description, Properties, Error))
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Error);
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("struct_name"), StructName);
    ResponseData->SetStringField(TEXT("path"), Path);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}

