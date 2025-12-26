#include "Commands/Project/DuplicateAssetCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FDuplicateAssetCommand::FDuplicateAssetCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FDuplicateAssetCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // source_path is required
    FString SourcePath;
    if (!JsonObject->TryGetStringField(TEXT("source_path"), SourcePath) || SourcePath.IsEmpty())
    {
        return false;
    }

    // new_name is required
    FString NewName;
    if (!JsonObject->TryGetStringField(TEXT("new_name"), NewName) || NewName.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FDuplicateAssetCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: source_path (string), new_name (string). Optional: destination_path (string)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString SourcePath = JsonObject->GetStringField(TEXT("source_path"));
    FString NewName = JsonObject->GetStringField(TEXT("new_name"));

    // destination_path is optional - if not provided, use the same directory as source
    FString DestinationPath;
    if (!JsonObject->TryGetStringField(TEXT("destination_path"), DestinationPath) || DestinationPath.IsEmpty())
    {
        // Extract directory from source path
        int32 LastSlashIndex;
        if (SourcePath.FindLastChar('/', LastSlashIndex))
        {
            DestinationPath = SourcePath.Left(LastSlashIndex);
        }
        else
        {
            DestinationPath = TEXT("/Game");
        }
    }

    // Execute the operation
    FString NewAssetPath;
    FString Error;
    if (!ProjectService->DuplicateAsset(SourcePath, DestinationPath, NewName, NewAssetPath, Error))
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
    ResponseData->SetStringField(TEXT("source_path"), SourcePath);
    ResponseData->SetStringField(TEXT("destination_path"), DestinationPath);
    ResponseData->SetStringField(TEXT("new_name"), NewName);
    ResponseData->SetStringField(TEXT("new_asset_path"), NewAssetPath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully duplicated asset to %s"), *NewAssetPath));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
