#include "Commands/Sound/ImportSoundFileCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FImportSoundFileCommand::FImportSoundFileCommand(ISoundService& InSoundService)
    : SoundService(InSoundService)
{
}

FString FImportSoundFileCommand::GetCommandName() const
{
    return TEXT("import_sound_file");
}

bool FImportSoundFileCommand::ValidateParams(const FString& Parameters) const
{
    FString SourceFilePath, AssetName, FolderPath, Error;
    return ParseParameters(Parameters, SourceFilePath, AssetName, FolderPath, Error);
}

FString FImportSoundFileCommand::Execute(const FString& Parameters)
{
    FString SourceFilePath, AssetName, FolderPath, Error;
    if (!ParseParameters(Parameters, SourceFilePath, AssetName, FolderPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    // Create import params
    FSoundWaveImportParams Params;
    Params.SourceFilePath = SourceFilePath;
    Params.AssetName = AssetName;
    Params.FolderPath = FolderPath;

    // Execute import
    FString AssetPath;
    if (!SoundService.ImportSoundFile(Params, AssetPath, Error))
    {
        return CreateErrorResponse(Error);
    }

    return CreateSuccessResponse(AssetPath, AssetName);
}

bool FImportSoundFileCommand::ParseParameters(const FString& JsonString, FString& OutSourceFilePath, FString& OutAssetName, FString& OutFolderPath, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    // Required: source_file_path
    if (!JsonObject->TryGetStringField(TEXT("source_file_path"), OutSourceFilePath) || OutSourceFilePath.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: source_file_path");
        return false;
    }

    // Required: asset_name
    if (!JsonObject->TryGetStringField(TEXT("asset_name"), OutAssetName) || OutAssetName.IsEmpty())
    {
        OutError = TEXT("Missing required parameter: asset_name");
        return false;
    }

    // Optional: folder_path (default: /Game/Audio)
    if (!JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath) || OutFolderPath.IsEmpty())
    {
        OutFolderPath = TEXT("/Game/Audio");
    }

    return true;
}

FString FImportSoundFileCommand::CreateSuccessResponse(const FString& AssetPath, const FString& AssetName) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("path"), AssetPath);
    Response->SetStringField(TEXT("name"), AssetName);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully imported sound file as: %s"), *AssetPath));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}

FString FImportSoundFileCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    return OutputString;
}
