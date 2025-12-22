#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Service for widget layout inspection and screenshot capture
 * Handles widget hierarchy queries and visual capture operations
 */
class UNREALMCP_API FWidgetLayoutService
{
public:
    /**
     * Get the component layout information for a widget blueprint
     * @param WidgetBlueprint - Widget blueprint to inspect
     * @param OutLayoutInfo - Output JSON object containing layout information
     * @return true if layout info was retrieved successfully
     */
    static bool GetWidgetComponentLayout(UWidgetBlueprint* WidgetBlueprint, TSharedPtr<FJsonObject>& OutLayoutInfo);

    /**
     * Capture a screenshot of a widget blueprint
     * @param WidgetBlueprint - Widget blueprint to capture
     * @param Width - Screenshot width in pixels
     * @param Height - Screenshot height in pixels
     * @param Format - Image format ("png" or "jpg")
     * @param OutScreenshotData - Output JSON object containing base64 encoded image
     * @return true if screenshot was captured successfully
     */
    static bool CaptureWidgetScreenshot(UWidgetBlueprint* WidgetBlueprint, int32 Width, int32 Height,
                                       const FString& Format, TSharedPtr<FJsonObject>& OutScreenshotData);

private:
    /**
     * Build hierarchical widget information recursively
     * @param Widget - Widget to build hierarchy for
     * @return JSON object containing widget hierarchy information
     */
    static TSharedPtr<FJsonObject> BuildWidgetHierarchy(UWidget* Widget);
};
