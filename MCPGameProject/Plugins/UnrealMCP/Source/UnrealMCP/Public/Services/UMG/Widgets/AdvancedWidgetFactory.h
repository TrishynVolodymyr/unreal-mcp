#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Factory class for creating advanced/specialized UMG widget components
 * Handles complex widgets like ListView, TreeView, ComboBox, etc.
 */
class UNREALMCP_API FAdvancedWidgetFactory
{
public:
    FAdvancedWidgetFactory();

    // Advanced/specialized components
    UWidget* CreateListView(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateTileView(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateTreeView(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateComboBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateMenuAnchor(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateWidgetSwitcher(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateExpandableArea(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateRichTextBlock(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateMultiLineEditableText(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateSpinBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateRadialSlider(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateThrobber(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateCircularThrobber(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateSafeZone(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateBackgroundBlur(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateNativeWidgetHost(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateNamedSlot(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateScaleBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateBorder(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateSpacer(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);

private:
    // Helper to get JSON arrays
    bool GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<TSharedPtr<FJsonValue>>& OutArray);
};
