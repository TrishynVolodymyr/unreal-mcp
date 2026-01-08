#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"

/**
 * Command for capturing a screenshot of the active editor viewport.
 * Saves the image to a specified path and returns the file location.
 */
class UNREALMCP_API FCaptureViewportScreenshotCommand : public IUnrealMCPCommand
{
public:
	FCaptureViewportScreenshotCommand() = default;
	virtual ~FCaptureViewportScreenshotCommand() = default;

	// IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override { return TEXT("capture_viewport_screenshot"); }
	virtual bool ValidateParams(const FString& Parameters) const override;
};
