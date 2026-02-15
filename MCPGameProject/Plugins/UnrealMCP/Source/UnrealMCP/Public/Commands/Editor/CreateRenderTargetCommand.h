#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IEditorService.h"

/**
 * Command for creating TextureRenderTarget2D assets.
 * Creates a persistent render target asset in the Content Browser.
 */
class UNREALMCP_API FCreateRenderTargetCommand : public IUnrealMCPCommand
{
public:
	explicit FCreateRenderTargetCommand(IEditorService& InEditorService);

	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	IEditorService& EditorService;

	bool ParseParameters(const FString& JsonString, FString& OutName, FString& OutFolderPath,
		int32& OutWidth, int32& OutHeight, FString& OutError) const;
	FString CreateSuccessResponse(const FString& AssetPath, int32 Width, int32 Height) const;
	FString CreateErrorResponse(const FString& ErrorMessage) const;
};
