#include "Commands/Sound/CreateSoundAttenuationCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCreateSoundAttenuationCommand::FCreateSoundAttenuationCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FCreateSoundAttenuationCommand::GetCommandName() const
{
    return TEXT("create_sound_attenuation");
}

bool FCreateSoundAttenuationCommand::ValidateParams(const FString& Parameters) const
{
    FString AssetName, FolderPath, AttenuationFunction, Error;
    float InnerRadius, FalloffDistance;
    bool bSpatialize;
    return ParseParameters(Parameters, AssetName, FolderPath, InnerRadius, FalloffDistance, AttenuationFunction, bSpatialize, Error);
}

FString FCreateSoundAttenuationCommand::Execute(const FString& Parameters)
{
    FString AssetName, FolderPath, AttenuationFunction, Error;
    float InnerRadius, FalloffDistance;
    bool bSpatialize;

    if (!ParseParameters(Parameters, AssetName, FolderPath, InnerRadius, FalloffDistance, AttenuationFunction, bSpatialize, Error))
    {
        return CreateErrorResponse(Error);
    }

    FSoundAttenuationParams Params;
    Params.AssetName = AssetName;
    Params.FolderPath = FolderPath;
    Params.InnerRadius = InnerRadius;
    Params.FalloffDistance = FalloffDistance;
    Params.AttenuationFunction = AttenuationFunction;
    Params.bSpatialize = bSpatialize;

    FString AssetPath;
    USoundAttenuation* Attenuation = SoundService.CreateSoundAttenuation(Params, AssetPath, Error);
    if (!Attenuation)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(AssetPath);
}

bool FCreateSoundAttenuationCommand::ParseParameters(const FString& JsonString, FString& OutAssetName,
    FString& OutFolderPath, float& OutInnerRadius, float& OutFalloffDistance,
    FString& OutAttenuationFunction, bool& OutSpatialize, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("asset_name"), OutAssetName) || OutAssetName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: asset_name");
        return false;
    }

    // Optional parameters with defaults
    OutFolderPath = TEXT("/Game/Audio");
    JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath);

    OutInnerRadius = 400.0f;
    JsonObject->TryGetNumberField(TEXT("inner_radius"), OutInnerRadius);

    OutFalloffDistance = 3600.0f;
    JsonObject->TryGetNumberField(TEXT("falloff_distance"), OutFalloffDistance);

    OutAttenuationFunction = TEXT("Linear");
    JsonObject->TryGetStringField(TEXT("attenuation_function"), OutAttenuationFunction);

    OutSpatialize = true;
    JsonObject->TryGetBoolField(TEXT("spatialize"), OutSpatialize);

    return true;
}

FString FCreateSoundAttenuationCommand::CreateSuccessResponse(const FString& AssetPath) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("path"), AssetPath);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created sound attenuation: %s"), *AssetPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateSoundAttenuationCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
