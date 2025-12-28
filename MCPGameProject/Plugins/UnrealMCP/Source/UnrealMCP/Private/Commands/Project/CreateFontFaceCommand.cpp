#include "Commands/Project/CreateFontFaceCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateFontFaceCommand::FCreateFontFaceCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateFontFaceCommand::ValidateParams(const FString& Parameters) const
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

    return true;
}

FString FCreateFontFaceCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: font_name (string)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString FontName = JsonObject->GetStringField(TEXT("font_name"));

    FString Path = TEXT("/Game/Fonts");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString SourceTexturePath;
    JsonObject->TryGetStringField(TEXT("source_texture"), SourceTexturePath);

    bool bUseSDF = true;  // Default to SDF
    JsonObject->TryGetBoolField(TEXT("use_sdf"), bUseSDF);

    int32 DistanceFieldSpread = 32;  // Default spread
    double TempSpread = 0;
    if (JsonObject->TryGetNumberField(TEXT("distance_field_spread"), TempSpread))
    {
        DistanceFieldSpread = static_cast<int32>(TempSpread);
    }

    // Get font metrics if provided
    TSharedPtr<FJsonObject> FontMetrics;
    const TSharedPtr<FJsonObject>* MetricsObj;
    if (JsonObject->TryGetObjectField(TEXT("font_metrics"), MetricsObj))
    {
        FontMetrics = *MetricsObj;
    }

    // Execute the operation
    FString OutAssetPath;
    FString Error;
    if (!ProjectService->CreateFontFace(FontName, Path, SourceTexturePath, bUseSDF, DistanceFieldSpread, FontMetrics, OutAssetPath, Error))
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
    ResponseData->SetStringField(TEXT("path"), Path);
    ResponseData->SetStringField(TEXT("asset_path"), OutAssetPath);
    ResponseData->SetBoolField(TEXT("use_sdf"), bUseSDF);
    ResponseData->SetNumberField(TEXT("distance_field_spread"), DistanceFieldSpread);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created font face '%s' at '%s'"), *FontName, *OutAssetPath));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
