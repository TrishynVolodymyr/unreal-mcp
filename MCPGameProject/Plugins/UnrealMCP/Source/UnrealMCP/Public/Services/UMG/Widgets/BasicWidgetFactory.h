#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class UWidgetBlueprint;
class UWidget;

/**
 * Factory class for creating basic UMG widget components
 * Handles simple UI elements like TextBlock, Button, Image, etc.
 */
class UNREALMCP_API FBasicWidgetFactory
{
public:
    FBasicWidgetFactory();

    // Basic interactive components
    UWidget* CreateTextBlock(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateButton(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateImage(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateCheckBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateSlider(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateProgressBar(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateEditableText(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);
    UWidget* CreateEditableTextBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject);

private:
    // Helper to get JSON arrays
    bool GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<TSharedPtr<FJsonValue>>& OutArray);
    
    // Helper to extract kwargs object (handles nested kwargs)
    TSharedPtr<FJsonObject> GetKwargsToUse(const TSharedPtr<FJsonObject>& KwargsObject, const FString& ComponentName, const FString& ComponentType);
};
