#include "Commands/Project/CreateOfflineFontCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateOfflineFontCommand::FCreateOfflineFontCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateOfflineFontCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString FontName;
    if (!JsonObject->TryGetStringField(TEXT("font_name"), FontName) || FontName.IsEmpty())
    {
        return false;
    }

    FString TexturePath;
    if (!JsonObject->TryGetStringField(TEXT("texture_path"), TexturePath) || TexturePath.IsEmpty())
    {
        return false;
    }

    FString MetricsFilePath;
    if (!JsonObject->TryGetStringField(TEXT("metrics_file_path"), MetricsFilePath) || MetricsFilePath.IsEmpty())
    {
        return false;
    }

    return true;
}

FString FCreateOfflineFontCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: font_name (string), texture_path (string), metrics_file_path (string)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString FontName = JsonObject->GetStringField(TEXT("font_name"));
    FString TexturePath = JsonObject->GetStringField(TEXT("texture_path"));
    FString MetricsFilePath = JsonObject->GetStringField(TEXT("metrics_file_path"));
    FString Path = JsonObject->HasField(TEXT("path")) ? JsonObject->GetStringField(TEXT("path")) : TEXT("/Game/Fonts");

    // Execute the operation
    FString OutAssetPath;
    FString Error;
    if (!ProjectService->CreateOfflineFont(FontName, Path, TexturePath, MetricsFilePath, OutAssetPath, Error))
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
    ResponseData->SetStringField(TEXT("font_name"), FontName);
    ResponseData->SetStringField(TEXT("font_path"), OutAssetPath);
    ResponseData->SetStringField(TEXT("texture_path"), TexturePath);
    ResponseData->SetStringField(TEXT("metrics_file_path"), MetricsFilePath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created offline font '%s' at '%s'"), *FontName, *OutAssetPath));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
