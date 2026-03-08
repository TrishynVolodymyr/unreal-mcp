#include "Commands/Sound/CreateSoundMixCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCreateSoundMixCommand::FCreateSoundMixCommand(ISoundService& InSoundService)
	: SoundService(InSoundService)
{
}

FString FCreateSoundMixCommand::GetCommandName() const
{
	return TEXT("create_sound_mix");
}

bool FCreateSoundMixCommand::ValidateParams(const FString& Parameters) const
{
	FString AssetName, FolderPath, Error;
	return ParseParameters(Parameters, AssetName, FolderPath, Error);
}

FString FCreateSoundMixCommand::Execute(const FString& Parameters)
{
	FString AssetName, FolderPath, Error;

	if (!ParseParameters(Parameters, AssetName, FolderPath, Error))
	{
		return CreateErrorResponse(Error);
	}

	FSoundMixParams Params;
	Params.AssetName = AssetName;
	Params.FolderPath = FolderPath;

	FString AssetPath;
	USoundMix* SoundMix = SoundService.CreateSoundMix(Params, AssetPath, Error);
	if (!SoundMix)
	{
		return CreateErrorResponse(Error);
	}

	return CreateSuccessResponse(AssetPath);
}

bool FCreateSoundMixCommand::ParseParameters(const FString& JsonString, FString& OutAssetName,
	FString& OutFolderPath, FString& OutError) const
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

	OutFolderPath = TEXT("/Game/Audio");
	JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath);

	return true;
}

FString FCreateSoundMixCommand::CreateSuccessResponse(const FString& AssetPath) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("path"), AssetPath);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created Sound Mix: %s"), *AssetPath));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}

FString FCreateSoundMixCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}
