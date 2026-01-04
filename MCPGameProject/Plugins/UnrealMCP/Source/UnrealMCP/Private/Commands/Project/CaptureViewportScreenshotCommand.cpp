#include "Commands/Project/CaptureViewportScreenshotCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "EditorViewportClient.h"
#include "Slate/SceneViewport.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

bool FCaptureViewportScreenshotCommand::ValidateParams(const FString& Parameters) const
{
	// No required parameters - output_path is optional
	return true;
}

FString FCaptureViewportScreenshotCommand::Execute(const FString& Parameters)
{
	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	FJsonSerializer::Deserialize(Reader, JsonObject);

	// Get optional output path
	FString OutputPath;
	if (JsonObject.IsValid())
	{
		JsonObject->TryGetStringField(TEXT("output_path"), OutputPath);
	}

	// If no path specified, use default in Saved/Screenshots
	if (OutputPath.IsEmpty())
	{
		FString ProjectDir = FPaths::ProjectDir();
		FString ScreenshotsDir = FPaths::Combine(ProjectDir, TEXT("Saved"), TEXT("Screenshots"), TEXT("MCP"));

		// Ensure directory exists
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*ScreenshotsDir))
		{
			PlatformFile.CreateDirectoryTree(*ScreenshotsDir);
		}

		// Generate timestamped filename
		FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		OutputPath = FPaths::Combine(ScreenshotsDir, FString::Printf(TEXT("Viewport_%s.png"), *Timestamp));
	}

	// Make sure path is absolute
	OutputPath = FPaths::ConvertRelativePathToFull(OutputPath);

	// Get the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

	// Get the first level editor viewport
	TSharedPtr<IAssetViewport> ActiveViewport = LevelEditorModule.GetFirstActiveViewport();

	if (!ActiveViewport.IsValid())
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active viewport found"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Get the scene viewport (which is an FViewport)
	FViewport* Viewport = ActiveViewport->GetSharedActiveViewport().Get();
	if (!Viewport)
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not access scene viewport"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Get viewport size
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid viewport size"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Use UE's proper screenshot function - this handles all the render target magic correctly
	TArray<FColor> Bitmap;
	if (!GetViewportScreenShot(Viewport, Bitmap))
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to capture viewport screenshot"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Compress to PNG
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid())
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create image wrapper"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Convert FColor array to raw bytes with forced opaque alpha
	TArray<uint8> RawData;
	RawData.SetNum(Bitmap.Num() * 4);
	for (int32 i = 0; i < Bitmap.Num(); i++)
	{
		RawData[i * 4 + 0] = Bitmap[i].B;
		RawData[i * 4 + 1] = Bitmap[i].G;
		RawData[i * 4 + 2] = Bitmap[i].R;
		RawData[i * 4 + 3] = 255;  // Force fully opaque
	}

	if (!ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), ViewportSize.X, ViewportSize.Y, ERGBFormat::BGRA, 8))
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to set raw image data"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Get compressed PNG data
	TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(100);
	if (CompressedData.Num() == 0)
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to compress image to PNG"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Save to file
	if (!FFileHelper::SaveArrayToFile(CompressedData, *OutputPath))
	{
		TSharedPtr<FJsonObject> ErrorResponse = FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to save screenshot to: %s"), *OutputPath));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Create success response
	TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
	ResponseData->SetBoolField(TEXT("success"), true);
	ResponseData->SetStringField(TEXT("file_path"), OutputPath);
	ResponseData->SetNumberField(TEXT("width"), ViewportSize.X);
	ResponseData->SetNumberField(TEXT("height"), ViewportSize.Y);
	ResponseData->SetStringField(TEXT("message"), FString::Printf(TEXT("Screenshot saved to: %s"), *OutputPath));

	// Convert response to JSON string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
	return OutputString;
}
