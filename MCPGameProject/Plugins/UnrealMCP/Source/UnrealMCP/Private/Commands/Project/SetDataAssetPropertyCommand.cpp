#include "Commands/Project/SetDataAssetPropertyCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FSetDataAssetPropertyCommand::FSetDataAssetPropertyCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FSetDataAssetPropertyCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // asset_path is required
    FString AssetPath;
    if (!JsonObject->TryGetStringField(TEXT("asset_path"), AssetPath) || AssetPath.IsEmpty())
    {
        return false;
    }

    // property_name is required
    FString PropertyName;
    if (!JsonObject->TryGetStringField(TEXT("property_name"), PropertyName) || PropertyName.IsEmpty())
    {
        return false;
    }

    // property_value is required (can be any type)
    if (!JsonObject->HasField(TEXT("property_value")))
    {
        return false;
    }

    return true;
}

FString FSetDataAssetPropertyCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: asset_path (string), property_name (string), property_value (any)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
    FString PropertyName = JsonObject->GetStringField(TEXT("property_name"));
    TSharedPtr<FJsonValue> PropertyValue = JsonObject->TryGetField(TEXT("property_value"));

    // Execute the operation
    FString Error;
    if (!ProjectService->SetDataAssetProperty(AssetPath, PropertyName, PropertyValue, Error))
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
    ResponseData->SetStringField(TEXT("asset_path"), AssetPath);
    ResponseData->SetStringField(TEXT("property_name"), PropertyName);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully set property '%s' on %s"), *PropertyName, *AssetPath));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
