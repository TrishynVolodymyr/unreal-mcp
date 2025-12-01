#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Factory class for creating layout container widgets
 * Handles containers like VerticalBox, HorizontalBox, Canvas, Grid, etc.
 */
class UNREALMCP_API FLayoutWidgetFactory
{
public:
    FLayoutWidgetFactory();

    // Layout containers
    UWidget* CreateVerticalBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateHorizontalBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateOverlay(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateGridPanel(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateCanvasPanel(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateSizeBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateScrollBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateWrapBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateUniformGridPanel(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);

private:
    // Helper to get JSON arrays
    bool GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<TSharedPtr<FJsonValue>>& OutArray);
};
