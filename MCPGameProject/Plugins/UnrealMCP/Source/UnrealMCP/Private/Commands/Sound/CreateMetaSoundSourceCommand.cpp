#include "Commands/Sound/CreateMetaSoundSourceCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCreateMetaSoundSourceCommand::FCreateMetaSoundSourceCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FCreateMetaSoundSourceCommand::GetCommandName() const
{
    return TEXT("create_metasound_source");
}

bool FCreateMetaSoundSourceCommand::ValidateParams(const FString& Parameters) const
{
    FString AssetName, FolderPath, OutputFormat, Error;
    bool bIsOneShot;
    return ParseParameters(Parameters, AssetName, FolderPath, OutputFormat, bIsOneShot, Error);
}

FString FCreateMetaSoundSourceCommand::Execute(const FString& Parameters)
{
    FString AssetName, FolderPath, OutputFormat, Error;
    bool bIsOneShot;
    if (!ParseParameters(Parameters, AssetName, FolderPath, OutputFormat, bIsOneShot, Error))
    {
        return CreateErrorResponse(Error);
    }

    FMetaSoundSourceParams Params;
    Params.AssetName = AssetName;
    Params.FolderPath = FolderPath;
    Params.OutputFormat = OutputFormat;
    Params.bIsOneShot = bIsOneShot;

    FString AssetPath;
    UMetaSoundSource* MetaSound = SoundService.CreateMetaSoundSource(Params, AssetPath, Error);
    if (!MetaSound)
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(AssetPath, AssetName, OutputFormat, bIsOneShot);
}

bool FCreateMetaSoundSourceCommand::ParseParameters(const FString& JsonString, FString& OutAssetName,
    FString& OutFolderPath, FString& OutOutputFormat, bool& OutIsOneShot, FString& OutError) const
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
        OutFolderPath = TEXT("/Game/Audio/MetaSounds");
    }

    if (!JsonObject->TryGetStringField(TEXT("output_format"), OutOutputFormat))
    {
        OutOutputFormat = TEXT("Stereo");
    }

    if (!JsonObject->TryGetBoolField(TEXT("is_one_shot"), OutIsOneShot))
    {
        OutIsOneShot = true;
    }

    return true;
}

FString FCreateMetaSoundSourceCommand::CreateSuccessResponse(const FString& AssetPath,
    const FString& AssetName, const FString& OutputFormat, bool bIsOneShot) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("path"), AssetPath);
    Response->SetStringField(TEXT("name"), AssetName);
    Response->SetStringField(TEXT("output_format"), OutputFormat);
    Response->SetBoolField(TEXT("is_one_shot"), bIsOneShot);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created MetaSound Source: %s"), *AssetPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FCreateMetaSoundSourceCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
