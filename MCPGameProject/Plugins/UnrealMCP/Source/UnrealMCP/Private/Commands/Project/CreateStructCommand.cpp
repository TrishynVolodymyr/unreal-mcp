#include "Commands/Project/CreateStructCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateStructCommand::FCreateStructCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateStructCommand::ValidateParams(const FString& Parameters) const
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

    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray;
    if (!JsonObject->TryGetArrayField(TEXT("properties"), PropertiesArray))
    {
        return false;
    }

    return true;
}

FString FCreateStructCommand::Execute(const FString& Parameters)
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
    
    FString Path = TEXT("/Game");
    JsonObject->TryGetStringField(TEXT("path"), Path);
    
    FString Description;
    JsonObject->TryGetStringField(TEXT("description"), Description);

    // Convert properties array to the format expected by the service
    const TArray<TSharedPtr<FJsonValue>>* PropertiesArray;
    JsonObject->TryGetArrayField(TEXT("properties"), PropertiesArray);
    
    TArray<TSharedPtr<FJsonObject>> Properties;
    if (PropertiesArray)
    {
        for (const TSharedPtr<FJsonValue>& PropertyValue : *PropertiesArray)
        {
            if (TSharedPtr<FJsonObject> PropertyObj = PropertyValue->AsObject())
            {
                Properties.Add(PropertyObj);
            }
        }
    }

    // Execute the operation
    FString FullPath;
    FString Error;
    if (!ProjectService->CreateStruct(StructName, Path, Description, Properties, FullPath, Error))
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
    ResponseData->SetStringField(TEXT("full_path"), FullPath);

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}

