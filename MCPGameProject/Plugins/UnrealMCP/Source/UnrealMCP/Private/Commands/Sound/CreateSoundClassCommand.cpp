#include "Commands/Sound/CreateSoundClassCommand.h"
#include "Services/ISoundService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FCreateSoundClassCommand::FCreateSoundClassCommand(ISoundService& InSoundService)
	: SoundService(InSoundService)
{
}

FString FCreateSoundClassCommand::GetCommandName() const
{
	return TEXT("create_sound_class");
}

bool FCreateSoundClassCommand::ValidateParams(const FString& Parameters) const
{
	FString AssetName, FolderPath, ParentClassPath, Error;
	float DefaultVolume;
	return ParseParameters(Parameters, AssetName, FolderPath, ParentClassPath, DefaultVolume, Error);
}

FString FCreateSoundClassCommand::Execute(const FString& Parameters)
{
	FString AssetName, FolderPath, ParentClassPath, Error;
	float DefaultVolume;

	if (!ParseParameters(Parameters, AssetName, FolderPath, ParentClassPath, DefaultVolume, Error))
	{
		return CreateErrorResponse(Error);
	}

	FSoundClassParams Params;
	Params.AssetName = AssetName;
	Params.FolderPath = FolderPath;
	Params.ParentClassPath = ParentClassPath;
	Params.DefaultVolume = DefaultVolume;

	FString AssetPath;
	USoundClass* SoundClass = SoundService.CreateSoundClass(Params, AssetPath, Error);
	if (!SoundClass)
	{
		return CreateErrorResponse(Error);
	}

	return CreateSuccessResponse(AssetPath);
}

bool FCreateSoundClassCommand::ParseParameters(const FString& JsonString, FString& OutAssetName,
	FString& OutFolderPath, FString& OutParentClassPath, float& OutDefaultVolume, FString& OutError) const
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

	OutParentClassPath = TEXT("");
	JsonObject->TryGetStringField(TEXT("parent_class_path"), OutParentClassPath);

	OutDefaultVolume = 1.0f;
	JsonObject->TryGetNumberField(TEXT("default_volume"), OutDefaultVolume);

	return true;
}

FString FCreateSoundClassCommand::CreateSuccessResponse(const FString& AssetPath) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("path"), AssetPath);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created Sound Class: %s"), *AssetPath));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}

FString FCreateSoundClassCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	return OutputString;
}
