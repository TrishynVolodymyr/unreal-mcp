#include "Commands/Project/CreateFontCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FCreateFontCommand::FCreateFontCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FCreateFontCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // font_name is always required
    FString FontName;
    if (!JsonObject->TryGetStringField(TEXT("font_name"), FontName) || FontName.IsEmpty())
    {
        return false;
    }

    // source_type is required
    FString SourceType;
    if (!JsonObject->TryGetStringField(TEXT("source_type"), SourceType) || SourceType.IsEmpty())
    {
        return false;
    }

    // Validate source_type value
    if (SourceType != TEXT("ttf") && SourceType != TEXT("sdf_texture") && SourceType != TEXT("offline"))
    {
        return false;
    }

    // Type-specific validation
    if (SourceType == TEXT("ttf"))
    {
        FString TTFFilePath;
        if (!JsonObject->TryGetStringField(TEXT("ttf_file_path"), TTFFilePath) || TTFFilePath.IsEmpty())
        {
            return false;
        }
    }
    else if (SourceType == TEXT("offline"))
    {
        FString TexturePath, MetricsFilePath;
        if (!JsonObject->TryGetStringField(TEXT("atlas_texture"), TexturePath) || TexturePath.IsEmpty())
        {
            return false;
        }
        if (!JsonObject->TryGetStringField(TEXT("metrics_file"), MetricsFilePath) || MetricsFilePath.IsEmpty())
        {
            return false;
        }
    }
    // sdf_texture has no additional required params (sdf_texture is optional)

    return true;
}

FString FCreateFontCommand::Execute(const FString& Parameters)
{
    // Parse JSON parameters
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    // Validate parameters
    if (!ValidateParams(Parameters))
    {
        return CreateErrorResponse(TEXT("Parameter validation failed. Required: font_name (string), source_type ('ttf'|'sdf_texture'|'offline'). For TTF: ttf_file_path. For offline: atlas_texture, metrics_file."));
    }

    // Route to the appropriate handler based on source_type
    FString SourceType = JsonObject->GetStringField(TEXT("source_type"));

    if (SourceType == TEXT("ttf"))
    {
        return ExecuteTTFImport(JsonObject);
    }
    else if (SourceType == TEXT("sdf_texture"))
    {
        return ExecuteSDFTexture(JsonObject);
    }
    else if (SourceType == TEXT("offline"))
    {
        return ExecuteOffline(JsonObject);
    }

    return CreateErrorResponse(TEXT("Unknown source_type"));
}

FString FCreateFontCommand::ExecuteTTFImport(const TSharedPtr<FJsonObject>& JsonObject)
{
    FString FontName = JsonObject->GetStringField(TEXT("font_name"));
    FString TTFFilePath = JsonObject->GetStringField(TEXT("ttf_file_path"));
    FString Path = TEXT("/Game/Fonts");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    // Get optional font metrics
    TSharedPtr<FJsonObject> FontMetrics;
    const TSharedPtr<FJsonObject>* MetricsObj;
    if (JsonObject->TryGetObjectField(TEXT("font_metrics"), MetricsObj))
    {
        FontMetrics = *MetricsObj;
    }

    FString OutAssetPath;
    FString Error;
    if (!ProjectService->ImportTTFFont(FontName, Path, TTFFilePath, FontMetrics, OutAssetPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("font_name"), FontName);
    ResponseData->SetStringField(TEXT("source_type"), TEXT("ttf"));
    ResponseData->SetStringField(TEXT("ttf_file_path"), TTFFilePath);
    ResponseData->SetStringField(TEXT("asset_path"), OutAssetPath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully imported TTF font '%s' from '%s'"), *FontName, *TTFFilePath));

    return CreateSuccessResponse(ResponseData);
}

FString FCreateFontCommand::ExecuteSDFTexture(const TSharedPtr<FJsonObject>& JsonObject)
{
    FString FontName = JsonObject->GetStringField(TEXT("font_name"));
    FString Path = TEXT("/Game/Fonts");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString SDFTexturePath;
    JsonObject->TryGetStringField(TEXT("sdf_texture"), SDFTexturePath);

    bool bUseSDF = true;
    JsonObject->TryGetBoolField(TEXT("use_sdf"), bUseSDF);

    int32 DistanceFieldSpread = 32;
    double TempSpread = 0;
    if (JsonObject->TryGetNumberField(TEXT("distance_field_spread"), TempSpread))
    {
        DistanceFieldSpread = static_cast<int32>(TempSpread);
    }

    TSharedPtr<FJsonObject> FontMetrics;
    const TSharedPtr<FJsonObject>* MetricsObj;
    if (JsonObject->TryGetObjectField(TEXT("font_metrics"), MetricsObj))
    {
        FontMetrics = *MetricsObj;
    }

    FString OutAssetPath;
    FString Error;
    if (!ProjectService->CreateFontFace(FontName, Path, SDFTexturePath, bUseSDF, DistanceFieldSpread, FontMetrics, OutAssetPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("font_name"), FontName);
    ResponseData->SetStringField(TEXT("source_type"), TEXT("sdf_texture"));
    ResponseData->SetStringField(TEXT("asset_path"), OutAssetPath);
    ResponseData->SetBoolField(TEXT("use_sdf"), bUseSDF);
    ResponseData->SetNumberField(TEXT("distance_field_spread"), DistanceFieldSpread);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created SDF font face '%s' at '%s'"), *FontName, *OutAssetPath));

    return CreateSuccessResponse(ResponseData);
}

FString FCreateFontCommand::ExecuteOffline(const TSharedPtr<FJsonObject>& JsonObject)
{
    FString FontName = JsonObject->GetStringField(TEXT("font_name"));
    FString Path = TEXT("/Game/Fonts");
    JsonObject->TryGetStringField(TEXT("path"), Path);

    FString AtlasTexturePath = JsonObject->GetStringField(TEXT("atlas_texture"));
    FString MetricsFilePath = JsonObject->GetStringField(TEXT("metrics_file"));

    FString OutAssetPath;
    FString Error;
    if (!ProjectService->CreateOfflineFont(FontName, Path, AtlasTexturePath, MetricsFilePath, OutAssetPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("font_name"), FontName);
    ResponseData->SetStringField(TEXT("source_type"), TEXT("offline"));
    ResponseData->SetStringField(TEXT("atlas_texture"), AtlasTexturePath);
    ResponseData->SetStringField(TEXT("metrics_file"), MetricsFilePath);
    ResponseData->SetStringField(TEXT("asset_path"), OutAssetPath);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created offline font '%s' at '%s'"), *FontName, *OutAssetPath));

    return CreateSuccessResponse(ResponseData);
}

FString FCreateFontCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
    return OutputString;
}

FString FCreateFontCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& ResponseData) const
{
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
