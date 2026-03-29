#include "Commands/Editor/ImportTextureCommand.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"

FString FImportTextureCommand::GetCommandName() const
{
	return TEXT("import_texture");
}

bool FImportTextureCommand::ValidateParams(const FString& Parameters) const
{
	FString SourceFilePath, AssetName, FolderPath, CompressionSettings, Error;
	bool bSRGB, bPreserveAlpha;
	return ParseParameters(Parameters, SourceFilePath, AssetName, FolderPath, CompressionSettings, bSRGB, bPreserveAlpha, Error);
}

FString FImportTextureCommand::Execute(const FString& Parameters)
{
	FString SourceFilePath, AssetName, FolderPath, CompressionSettings, Error;
	bool bSRGB, bPreserveAlpha;
	if (!ParseParameters(Parameters, SourceFilePath, AssetName, FolderPath, CompressionSettings, bSRGB, bPreserveAlpha, Error))
	{
		return CreateErrorResponse(Error);
	}

	// Validate source file exists
	if (!FPaths::FileExists(SourceFilePath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Source file does not exist: %s"), *SourceFilePath));
	}

	// Validate file extension
	FString Extension = FPaths::GetExtension(SourceFilePath).ToLower();
	TArray<FString> SupportedExtensions = { TEXT("png"), TEXT("tga"), TEXT("tif"), TEXT("tiff"), TEXT("jpg"), TEXT("jpeg"), TEXT("exr"), TEXT("hdr"), TEXT("bmp") };
	if (!SupportedExtensions.Contains(Extension))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Unsupported texture format: %s. Supported: png, tga, tif, jpg, jpeg, exr, hdr, bmp"), *Extension));
	}

	// Normalize destination path
	FString DestinationPath = FolderPath;
	if (!DestinationPath.StartsWith(TEXT("/Game")))
	{
		if (DestinationPath.StartsWith(TEXT("/")))
		{
			DestinationPath = TEXT("/Game") + DestinationPath;
		}
		else
		{
			DestinationPath = TEXT("/Game/") + DestinationPath;
		}
	}

	// Create package for the asset
	FString PackagePath = DestinationPath / AssetName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}
	Package->FullyLoad();

	// Create and configure the texture factory
	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	TextureFactory->AddToRoot();

	// Configure for automated import (suppress dialogs)
	TextureFactory->SuppressImportOverwriteDialog();
	TextureFactory->NoAlpha = !bPreserveAlpha;

	// Parse compression settings
	TextureCompressionSettings CompressionEnum = TC_Default;
	if (CompressionSettings == TEXT("Normalmap"))
	{
		CompressionEnum = TC_Normalmap;
	}
	else if (CompressionSettings == TEXT("Masks"))
	{
		CompressionEnum = TC_Masks;
	}
	else if (CompressionSettings == TEXT("Grayscale"))
	{
		CompressionEnum = TC_Grayscale;
	}
	else if (CompressionSettings == TEXT("HDR"))
	{
		CompressionEnum = TC_HDR;
	}
	else if (CompressionSettings == TEXT("Alpha"))
	{
		CompressionEnum = TC_Alpha;
	}
	// else TC_Default

	// Read file data into memory
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *SourceFilePath))
	{
		TextureFactory->RemoveFromRoot();
		return CreateErrorResponse(FString::Printf(TEXT("Failed to read source file: %s"), *SourceFilePath));
	}

	// Import using FactoryCreateBinary (no bCancelled param in UE 5.7)
	const uint8* DataPtr = FileData.GetData();

	UObject* ImportedObject = TextureFactory->FactoryCreateBinary(
		UTexture2D::StaticClass(),
		Package,
		FName(*AssetName),
		RF_Public | RF_Standalone,
		nullptr,    // Context
		*Extension, // Type (file extension)
		DataPtr,
		DataPtr + FileData.Num(),
		GWarn
	);

	TextureFactory->RemoveFromRoot();

	if (!ImportedObject)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to import texture file: %s"), *SourceFilePath));
	}

	// Apply post-import settings
	UTexture2D* ImportedTexture = Cast<UTexture2D>(ImportedObject);
	if (ImportedTexture)
	{
		ImportedTexture->CompressionSettings = CompressionEnum;
		ImportedTexture->SRGB = bSRGB;

		// For normalmap compression, always disable sRGB
		if (CompressionEnum == TC_Normalmap || CompressionEnum == TC_Masks || CompressionEnum == TC_Grayscale)
		{
			ImportedTexture->SRGB = false;
		}

		ImportedTexture->UpdateResource();
		ImportedTexture->PostEditChange();
	}

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(ImportedObject);
	Package->MarkPackageDirty();

	// Get texture info
	int32 SizeX = 0;
	int32 SizeY = 0;
	bool bHasAlpha = false;

	if (ImportedTexture)
	{
		// Read dimensions from source data instead of render resource
		// (render resource may still be initializing asynchronously, returning 32x32 placeholder)
		if (ImportedTexture->Source.IsValid())
		{
			SizeX = ImportedTexture->Source.GetSizeX();
			SizeY = ImportedTexture->Source.GetSizeY();
		}
		else
		{
			SizeX = ImportedTexture->GetSizeX();
			SizeY = ImportedTexture->GetSizeY();
		}
		bHasAlpha = ImportedTexture->HasAlphaChannel();

		UE_LOG(LogTemp, Log, TEXT("Imported texture '%s' from '%s' (%dx%d, Alpha: %s, sRGB: %s, Compression: %s)"),
			*PackagePath, *SourceFilePath, SizeX, SizeY,
			bHasAlpha ? TEXT("Yes") : TEXT("No"),
			ImportedTexture->SRGB ? TEXT("Yes") : TEXT("No"),
			*CompressionSettings);
	}

	return CreateSuccessResponse(PackagePath, AssetName, SizeX, SizeY, bHasAlpha);
}

bool FImportTextureCommand::ParseParameters(
	const FString& JsonString,
	FString& OutSourceFilePath,
	FString& OutAssetName,
	FString& OutFolderPath,
	FString& OutCompressionSettings,
	bool& OutSRGB,
	bool& OutPreserveAlpha,
	FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Failed to parse JSON parameters");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("source_file_path"), OutSourceFilePath) || OutSourceFilePath.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: source_file_path");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("asset_name"), OutAssetName) || OutAssetName.IsEmpty())
	{
		OutError = TEXT("Missing required parameter: asset_name");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath) || OutFolderPath.IsEmpty())
	{
		OutFolderPath = TEXT("/Game/Textures");
	}

	if (!JsonObject->TryGetStringField(TEXT("compression_settings"), OutCompressionSettings) || OutCompressionSettings.IsEmpty())
	{
		OutCompressionSettings = TEXT("Default");
	}

	if (!JsonObject->TryGetBoolField(TEXT("srgb"), OutSRGB))
	{
		OutSRGB = true;
	}

	if (!JsonObject->TryGetBoolField(TEXT("preserve_alpha"), OutPreserveAlpha))
	{
		OutPreserveAlpha = true;
	}

	return true;
}

FString FImportTextureCommand::CreateSuccessResponse(const FString& AssetPath, const FString& AssetName, int32 SizeX, int32 SizeY, bool bHasAlpha) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("path"), AssetPath);
	Response->SetStringField(TEXT("name"), AssetName);
	Response->SetNumberField(TEXT("size_x"), SizeX);
	Response->SetNumberField(TEXT("size_y"), SizeY);
	Response->SetBoolField(TEXT("has_alpha"), bHasAlpha);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully imported texture as: %s (%dx%d, Alpha: %s)"), *AssetPath, SizeX, SizeY, bHasAlpha ? TEXT("Yes") : TEXT("No")));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

FString FImportTextureCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}
