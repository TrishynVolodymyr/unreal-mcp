#include "Commands/Sound/CreateSoundCueCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCreateSoundCueCommand::FCreateSoundCueCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FCreateSoundCueCommand::GetCommandName() const
{
    return TEXT("create_sound_cue");
}

bool FCreateSoundCueCommand::ValidateParams(const FString& Parameters) const
{
    FString AssetName, FolderPath, InitialSoundWave, Error;
    return ParseParameters(Parameters, AssetName, FolderPath, InitialSoundWave, Error);
}

FString FCreateSoundCueCommand::Execute(const FString& Parameters)
{
    FString AssetName, FolderPath, InitialSoundWave, Error;
    if (!ParseParameters(Parameters, AssetName, FolderPath, InitialSoundWave, Error))
    {
        return CreateErrorResponse(Error);
    }

    FSoundCueCreationParams Params;
    Params.AssetName = AssetName;
    Params.FolderPath = FolderPath;
    Params.InitialSoundWavePath = InitialSoundWave;

    FString AssetPath;
    USoundCue* SoundCue = SoundService.CreateSoundCue(Params, AssetPath, Error);
    if (!SoundCue)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(AssetPath);
}

bool FCreateSoundCueCommand::ParseParameters(const FString& JsonString, FString& OutAssetName, FString& OutFolderPath,
    FString& OutInitialSoundWave, FString& OutError) const
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

    // Optional parameters
    if (!JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath))
    {
        OutFolderPath = TEXT("/Game/Audio");
    }

    JsonObject->TryGetStringField(TEXT("initial_sound_wave"), OutInitialSoundWave);

    return true;
}

FString FCreateSoundCueCommand::CreateSuccessResponse(const FString& AssetPath) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("path"), AssetPath);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created Sound Cue: %s"), *AssetPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateSoundCueCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
