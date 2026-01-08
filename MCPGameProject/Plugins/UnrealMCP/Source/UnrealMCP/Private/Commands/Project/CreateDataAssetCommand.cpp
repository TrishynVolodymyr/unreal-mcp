#include "Commands/Project/CreateDataAssetCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateDataAssetCommand::FCreateDataAssetCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateDataAssetCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // name is required
    FString Name;
    if (!JsonObject->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
    {
        return false;
    }

    // asset_class is required
    FString AssetClass;
    if (!JsonObject->TryGetStringField(TEXT("asset_class"), AssetClass) || AssetClass.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FCreateDataAssetCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: name (string), asset_class (string). Optional: folder_path (string), properties (object)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString Name = JsonObject->GetStringField(TEXT("name"));
    FString AssetClass = JsonObject->GetStringField(TEXT("asset_class"));

    // folder_path is optional - defaults to /Game
    FString FolderPath;
    if (!JsonObject->TryGetStringField(TEXT("folder_path"), FolderPath) || FolderPath.IsEmpty())
    {
        FolderPath = TEXT("/Game");
    }

    // properties is optional
    const TSharedPtr<FJsonObject>* PropertiesPtr = nullptr;
    TSharedPtr<FJsonObject> Properties;
    if (JsonObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        Properties = *PropertiesPtr;
    }

    // Execute the operation
    FString AssetPath;
    FString Error;
    if (!ProjectService->CreateDataAsset(Name, AssetClass, FolderPath, Properties, AssetPath, Error))
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
    ResponseData->SetStringField(TEXT("name"), Name);
    ResponseData->SetStringField(TEXT("asset_class"), AssetClass);
    ResponseData->SetStringField(TEXT("folder_path"), FolderPath);
    ResponseData->SetStringField(TEXT("asset_path"), AssetPath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created DataAsset at %s"), *AssetPath));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
