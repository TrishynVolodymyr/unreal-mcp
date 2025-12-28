#include "Commands/Project/SetFontFacePropertiesCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FSetFontFacePropertiesCommand::FSetFontFacePropertiesCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FSetFontFacePropertiesCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString FontPath;
    if (!JsonObject->TryGetStringField(TEXT("font_path"), FontPath) || FontPath.IsEmpty())
    {
        return false;
    }

    const TSharedPtr<FJsonObject>* PropertiesObj;
    if (!JsonObject->TryGetObjectField(TEXT("properties"), PropertiesObj))
    {
        return false;
    }

    return true;
}

FString FSetFontFacePropertiesCommand::Execute(const FString& Parameters)
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
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Parameter validation failed. Required: font_path (string), properties (object)"));
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return OutputString;
    }

    // Extract parameters
    FString FontPath = JsonObject->GetStringField(TEXT("font_path"));

    const TSharedPtr<FJsonObject>* PropertiesObj;
    JsonObject->TryGetObjectField(TEXT("properties"), PropertiesObj);
    TSharedPtr<FJsonObject> Properties = *PropertiesObj;

    // Execute the operation
    TArray<FString> SuccessProperties;
    TArray<FString> FailedProperties;
    FString Error;
    if (!ProjectService->SetFontFaceProperties(FontPath, Properties, SuccessProperties, FailedProperties, Error))
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
    ResponseData->SetStringField(TEXT("font_path"), FontPath);

    // Add success properties array
    TArray<TSharedPtr<FJsonValue>> SuccessArray;
    for (const FString& Property : SuccessProperties)
    {
        SuccessArray.Add(MakeShared<FJsonValueString>(Property));
    }
    ResponseData->SetArrayField(TEXT("success_properties"), SuccessArray);

    // Add failed properties array
    TArray<TSharedPtr<FJsonValue>> FailedArray;
    for (const FString& Property : FailedProperties)
    {
        FailedArray.Add(MakeShared<FJsonValueString>(Property));
    }
    ResponseData->SetArrayField(TEXT("failed_properties"), FailedArray);

    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Set %d properties on font face '%s' (%d failed)"),
        SuccessProperties.Num(), *FontPath, FailedProperties.Num()));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
