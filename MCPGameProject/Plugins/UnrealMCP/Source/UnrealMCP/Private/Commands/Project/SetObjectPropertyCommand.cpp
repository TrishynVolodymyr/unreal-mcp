#include "Commands/Project/SetObjectPropertyCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FSetObjectPropertyCommand::FSetObjectPropertyCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FSetObjectPropertyCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }
    FString AssetPath;
    if (!JsonObject->TryGetStringField(TEXT("asset_path"), AssetPath) || AssetPath.IsEmpty())
    {
        return false;
    }
    FString PropertyName;
    if (!JsonObject->TryGetStringField(TEXT("property_name"), PropertyName) || PropertyName.IsEmpty())
    {
        return false;
    }
    // property_value is required but may legitimately be any string; presence only.
    return JsonObject->HasField(TEXT("property_value"));
}

FString FSetObjectPropertyCommand::Execute(const FString& Parameters)
{
    auto MakeError = [](const FString& Msg) -> FString
    {
        TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(Msg);
        FString Out;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
        FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
        return Out;
    };

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return MakeError(TEXT("Invalid JSON parameters"));
    }
    if (!ValidateParams(Parameters))
    {
        return MakeError(TEXT("Parameter validation failed. Required: asset_path (string), property_name (string), property_value (string in UE property-text form)"));
    }

    const FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
    const FString PropertyName = JsonObject->GetStringField(TEXT("property_name"));
    // Accept value as string; numbers/bools are also fine via JSON string coercion.
    FString ValueString;
    if (!JsonObject->TryGetStringField(TEXT("property_value"), ValueString))
    {
        // Coerce non-string JSON (number/bool) to string.
        const TSharedPtr<FJsonValue> Raw = JsonObject->TryGetField(TEXT("property_value"));
        if (Raw.IsValid()) { Raw->TryGetString(ValueString); }
    }

    FString Error;
    FString AppliedValue;
    if (!ProjectService->SetObjectProperty(AssetPath, PropertyName, ValueString, Error, &AppliedValue))
    {
        return MakeError(Error);
    }

    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("asset_path"), AssetPath);
    ResponseData->SetStringField(TEXT("property_name"), PropertyName);
    // The value RE-EXPORTED from the asset after import + PostEditChangeProperty — what
    // actually persists. Callers must compare this against intent instead of trusting
    // `success` (known-issues: a write that parses can still be sanitized/reverted).
    ResponseData->SetStringField(TEXT("applied_value"), AppliedValue);
    ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Set '%s' on %s"), *PropertyName, *AssetPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
