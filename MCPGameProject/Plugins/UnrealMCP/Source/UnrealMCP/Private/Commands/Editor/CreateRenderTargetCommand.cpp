#include "Commands/Editor/CreateRenderTargetCommand.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCreateRenderTargetCommand::FCreateRenderTargetCommand(IEditorService& InEditorService)
	: EditorService(InEditorService)
{
}

FString FCreateRenderTargetCommand::Execute(const FString& Parameters)
{
	FString Name, FolderPath, Error;
	int32 Width, Height;

	if (!ParseParameters(Parameters, Name, FolderPath, Width, Height, Error))
	{
		return CreateErrorResponse(Error);
	}

	// Build asset path
	if (FolderPath.IsEmpty())
	{
		FolderPath = TEXT("/Game");
	}
	// Ensure folder path starts with /Game
	if (!FolderPath.StartsWith(TEXT("/Game")))
	{
		FolderPath = FString::Printf(TEXT("/Game/%s"), *FolderPath);
	}
	// Remove trailing slash
	FolderPath.RemoveFromEnd(TEXT("/"));

	FString PackagePath = FString::Printf(TEXT("%s/%s"), *FolderPath, *Name);

	// Check if asset already exists
	if (FindObject<UObject>(nullptr, *PackagePath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Asset already exists at: %s"), *PackagePath));
	}

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create the render target
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(
		Package, *Name, RF_Public | RF_Standalone);

	if (!RenderTarget)
	{
		return CreateErrorResponse(TEXT("Failed to create TextureRenderTarget2D"));
	}

	// Configure
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->InitAutoFormat(Width, Height);
	RenderTarget->UpdateResourceImmediate(true);

	// Mark dirty and save
	RenderTarget->MarkPackageDirty();
	Package->MarkPackageDirty();

	// Save the asset
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, RenderTarget, *PackageFileName, SaveArgs);

	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateRenderTarget: Asset created but failed to save to disk"));
	}

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(RenderTarget);

	UE_LOG(LogTemp, Log, TEXT("Created TextureRenderTarget2D: %s (%dx%d)"), *PackagePath, Width, Height);

	return CreateSuccessResponse(PackagePath, Width, Height);
}

FString FCreateRenderTargetCommand::GetCommandName() const
{
	return TEXT("create_render_target");
}

bool FCreateRenderTargetCommand::ValidateParams(const FString& Parameters) const
{
	FString Name, FolderPath, Error;
	int32 Width, Height;
	return ParseParameters(Parameters, Name, FolderPath, Width, Height, Error);
}

bool FCreateRenderTargetCommand::ParseParameters(const FString& JsonString, FString& OutName,
	FString& OutFolderPath, int32& OutWidth, int32& OutHeight, FString& OutError) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	if (!JsonObject->TryGetStringField(TEXT("name"), OutName) || OutName.IsEmpty())
	{
		OutError = TEXT("Missing required 'name' parameter");
		return false;
	}

	JsonObject->TryGetStringField(TEXT("folder_path"), OutFolderPath);
	OutWidth = JsonObject->HasField(TEXT("width")) ? (int32)JsonObject->GetNumberField(TEXT("width")) : 256;
	OutHeight = JsonObject->HasField(TEXT("height")) ? (int32)JsonObject->GetNumberField(TEXT("height")) : 256;

	if (OutWidth <= 0 || OutHeight <= 0 || OutWidth > 4096 || OutHeight > 4096)
	{
		OutError = TEXT("Width and height must be between 1 and 4096");
		return false;
	}

	return true;
}

FString FCreateRenderTargetCommand::CreateSuccessResponse(const FString& AssetPath, int32 Width, int32 Height) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);
	ResponseObj->SetStringField(TEXT("asset_path"), AssetPath);
	ResponseObj->SetNumberField(TEXT("width"), Width);
	ResponseObj->SetNumberField(TEXT("height"), Height);
	ResponseObj->SetStringField(TEXT("type"), TEXT("TextureRenderTarget2D"));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
	return OutputString;
}

FString FCreateRenderTargetCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetBoolField(TEXT("success"), false);
	ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
	return OutputString;
}
