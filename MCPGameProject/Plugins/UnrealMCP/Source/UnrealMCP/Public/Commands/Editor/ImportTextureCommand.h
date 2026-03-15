#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command to import a texture file (PNG, TGA, TIF, JPEG, EXR, HDR) from disk into the Unreal project.
 * Self-contained — uses UTextureFactory directly, no service layer needed.
 */
class UNREALMCP_API FImportTextureCommand : public IUnrealMCPCommand
{
public:
	FImportTextureCommand() = default;

	//~ IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;
	//~ End IUnrealMCPCommand interface

private:
	bool ParseParameters(
		const FString& JsonString,
		FString& OutSourceFilePath,
		FString& OutAssetName,
		FString& OutFolderPath,
		FString& OutCompressionSettings,
		bool& OutSRGB,
		bool& OutPreserveAlpha,
		FString& OutError) const;

	FString CreateSuccessResponse(const FString& AssetPath, const FString& AssetName, int32 SizeX, int32 SizeY, bool bHasAlpha) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
