#include "Commands/Project/MoveAssetCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FMoveAssetCommand::FMoveAssetCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FMoveAssetCommand::ValidateParams(const FString& Parameters) const
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

    // destination_folder is required
    FString DestinationFolder;
    if (!JsonObject->TryGetStringField(TEXT("destination_folder"), DestinationFolder) || DestinationFolder.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FMoveAssetCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: asset_path (string), destination_folder (string)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
    FString DestinationFolder = JsonObject->GetStringField(TEXT("destination_folder"));

    // Execute the operation
    FString NewAssetPath;
    FString Error;
    if (!ProjectService->MoveAsset(AssetPath, DestinationFolder, NewAssetPath, Error))
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
    ResponseData->SetStringField(TEXT("old_path"), AssetPath);
    ResponseData->SetStringField(TEXT("destination_folder"), DestinationFolder);
    ResponseData->SetStringField(TEXT("new_asset_path"), NewAssetPath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully moved asset to %s"), *NewAssetPath));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
