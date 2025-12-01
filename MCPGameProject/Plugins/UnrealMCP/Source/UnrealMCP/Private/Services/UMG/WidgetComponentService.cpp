#include "Services/UMG/WidgetComponentService.h"
#include "Editor/UMGEditor/Public/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Throbber.h"
#include "Components/ExpandableArea.h"
#include "Components/RichTextBlock.h"
#include "Components/MultiLineEditableText.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/GridPanel.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "Components/CircularThrobber.h"
#include "Components/SpinBox.h"
#include "Components/WrapBox.h"
#include "Components/ScaleBox.h"
#include "Components/NamedSlot.h"
#include "Components/RadialSlider.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/SafeZone.h"
#include "Components/MenuAnchor.h"
#include "Components/NativeWidgetHost.h"
#include "Components/BackgroundBlur.h"
#include "Components/UniformGridPanel.h"

FWidgetComponentService::FWidgetComponentService()
{
}

UWidget* FWidgetComponentService::CreateWidgetComponent(
    UWidgetBlueprint* WidgetBlueprint, 
    const FString& ComponentName, 
    const FString& ComponentType,
    const FVector2D& Position, 
    const FVector2D& Size,
    const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Log the received KwargsObject
    FString JsonString = TEXT("null");
    if (KwargsObject.IsValid())
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(KwargsObject.ToSharedRef(), Writer);
    }
    UE_LOG(LogTemp, Log, TEXT("FWidgetComponentService::CreateWidgetComponent Received Kwargs for %s (%s): %s"), *ComponentName, *ComponentType, *JsonString);
    
    // Create the appropriate widget based on component type
    UWidget* CreatedWidget = nullptr;

    // TextBlock
    if (ComponentType.Equals(TEXT("TextBlock"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateTextBlock(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Button
    else if (ComponentType.Equals(TEXT("Button"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateButton(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Image
    else if (ComponentType.Equals(TEXT("Image"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateImage(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // CheckBox
    else if (ComponentType.Equals(TEXT("CheckBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateCheckBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Slider
    else if (ComponentType.Equals(TEXT("Slider"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateSlider(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // ProgressBar
    else if (ComponentType.Equals(TEXT("ProgressBar"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateProgressBar(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Border
    else if (ComponentType.Equals(TEXT("Border"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateBorder(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // ScrollBox
    else if (ComponentType.Equals(TEXT("ScrollBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateScrollBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Spacer
    else if (ComponentType.Equals(TEXT("Spacer"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateSpacer(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // WidgetSwitcher
    else if (ComponentType.Equals(TEXT("WidgetSwitcher"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateWidgetSwitcher(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Throbber
    else if (ComponentType.Equals(TEXT("Throbber"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateThrobber(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // ExpandableArea
    else if (ComponentType.Equals(TEXT("ExpandableArea"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateExpandableArea(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // RichTextBlock
    else if (ComponentType.Equals(TEXT("RichTextBlock"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateRichTextBlock(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // MultiLineEditableText
    else if (ComponentType.Equals(TEXT("MultiLineEditableText"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateMultiLineEditableText(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // VerticalBox
    else if (ComponentType.Equals(TEXT("VerticalBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateVerticalBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // HorizontalBox
    else if (ComponentType.Equals(TEXT("HorizontalBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateHorizontalBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Overlay
    else if (ComponentType.Equals(TEXT("Overlay"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateOverlay(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // GridPanel
    else if (ComponentType.Equals(TEXT("GridPanel"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateGridPanel(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // SizeBox
    else if (ComponentType.Equals(TEXT("SizeBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateSizeBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // CanvasPanel
    else if (ComponentType.Equals(TEXT("CanvasPanel"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateCanvasPanel(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // ComboBox
    else if (ComponentType.Equals(TEXT("ComboBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateComboBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // EditableText
    else if (ComponentType.Equals(TEXT("EditableText"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateEditableText(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // EditableTextBox
    else if (ComponentType.Equals(TEXT("EditableTextBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateEditableTextBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // CircularThrobber
    else if (ComponentType.Equals(TEXT("CircularThrobber"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateCircularThrobber(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // SpinBox
    else if (ComponentType.Equals(TEXT("SpinBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateSpinBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // WrapBox
    else if (ComponentType.Equals(TEXT("WrapBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateWrapBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // ScaleBox
    else if (ComponentType.Equals(TEXT("ScaleBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateScaleBox(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // NamedSlot
    else if (ComponentType.Equals(TEXT("NamedSlot"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateNamedSlot(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // RadialSlider
    else if (ComponentType.Equals(TEXT("RadialSlider"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateRadialSlider(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // ListView
    else if (ComponentType.Equals(TEXT("ListView"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateListView(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // TileView
    else if (ComponentType.Equals(TEXT("TileView"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateTileView(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // TreeView
    else if (ComponentType.Equals(TEXT("TreeView"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateTreeView(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // SafeZone
    else if (ComponentType.Equals(TEXT("SafeZone"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateSafeZone(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // MenuAnchor
    else if (ComponentType.Equals(TEXT("MenuAnchor"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateMenuAnchor(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // NativeWidgetHost
    else if (ComponentType.Equals(TEXT("NativeWidgetHost"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateNativeWidgetHost(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // BackgroundBlur
    else if (ComponentType.Equals(TEXT("BackgroundBlur"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateBackgroundBlur(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // StackBox (not a standard UE widget, use VerticalBox instead)
    else if (ComponentType.Equals(TEXT("StackBox"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateVerticalBox(WidgetBlueprint, ComponentName, KwargsObject);
        UE_LOG(LogTemp, Warning, TEXT("StackBox is not available in this UE version. Using VerticalBox instead for '%s'."), *ComponentName);
    }
    // UniformGridPanel
    else if (ComponentType.Equals(TEXT("UniformGridPanel"), ESearchCase::IgnoreCase))
    {
        CreatedWidget = CreateUniformGridPanel(WidgetBlueprint, ComponentName, KwargsObject);
    }
    // Default case for unsupported types
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Unsupported component type: %s"), *ComponentType);
        return nullptr;
    }
    
    // If widget creation failed, return nullptr
    if (!CreatedWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create widget component: %s"), *ComponentName);
        return nullptr;
    }
    
    // Add the widget to the widget tree
    if (!AddWidgetToTree(WidgetBlueprint, CreatedWidget, Position, Size))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add widget to tree: %s"), *ComponentName);
        return nullptr;
    }
    
    // Save the blueprint
    SaveWidgetBlueprint(WidgetBlueprint);
    
    UE_LOG(LogTemp, Log, TEXT("Successfully created and added widget component: %s"), *ComponentName);
    return CreatedWidget;
}

bool FWidgetComponentService::GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, 
                                         const FString& FieldName, 
                                         TArray<TSharedPtr<FJsonValue>>& OutArray)
{
    if (!JsonObject->HasField(FieldName))
    {
        return false;
    }

    // In UE 5.5, TryGetArrayField API has changed
    const TArray<TSharedPtr<FJsonValue>>* ArrayPtr;
    bool bSuccess = JsonObject->TryGetArrayField(FStringView(*FieldName), ArrayPtr);
    if (bSuccess)
    {
        OutArray = *ArrayPtr;
        return true;
    }
    return false;
}

TSharedPtr<FJsonObject> FWidgetComponentService::GetKwargsToUse(const TSharedPtr<FJsonObject>& KwargsObject, const FString& ComponentName, const FString& ComponentType)
{
    // Debug: Print the KwargsObject structure
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(KwargsObject.ToSharedRef(), Writer);
    UE_LOG(LogTemp, Display, TEXT("KwargsObject for %s '%s': %s"), *ComponentType, *ComponentName, *JsonString);
    
    // Determine if we're using nested kwargs
    if (KwargsObject->HasField(TEXT("kwargs")))
    {
        TSharedPtr<FJsonObject> NestedKwargs = KwargsObject->GetObjectField(TEXT("kwargs"));
        UE_LOG(LogTemp, Display, TEXT("Using nested kwargs for %s '%s'"), *ComponentType, *ComponentName);
        return NestedKwargs;
    }
    
    return KwargsObject;
}

UWidget* FWidgetComponentService::CreateTextBlock(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("TextBlock"));
    return BasicWidgetFactory.CreateTextBlock(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateButton(UWidgetBlueprint* WidgetBlueprint, 
                                           const FString& ComponentName, 
                                           const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Button"));
    return BasicWidgetFactory.CreateButton(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateImage(UWidgetBlueprint* WidgetBlueprint, 
                                          const FString& ComponentName, 
                                          const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Image"));
    return BasicWidgetFactory.CreateImage(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateCheckBox(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("CheckBox"));
    return BasicWidgetFactory.CreateCheckBox(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateSlider(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Slider"));
    return BasicWidgetFactory.CreateSlider(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateProgressBar(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("ProgressBar"));
    return BasicWidgetFactory.CreateProgressBar(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateBorder(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Border"));
    return AdvancedWidgetFactory.CreateBorder(WidgetBlueprint, ComponentName, KwargsToUse);
}

UWidget* FWidgetComponentService::CreateScrollBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateScrollBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateSpacer(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateSpacer(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateWidgetSwitcher(UWidgetBlueprint* WidgetBlueprint, 
                                                    const FString& ComponentName, 
                                                    const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateWidgetSwitcher(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateThrobber(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateThrobber(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateExpandableArea(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateExpandableArea(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateRichTextBlock(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateRichTextBlock(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateMultiLineEditableText(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateMultiLineEditableText(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateVerticalBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateVerticalBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateHorizontalBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateHorizontalBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateOverlay(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateOverlay(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateGridPanel(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateGridPanel(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateSizeBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateSizeBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateCanvasPanel(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateCanvasPanel(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateComboBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateComboBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateEditableText(UWidgetBlueprint* WidgetBlueprint, 
                                                  const FString& ComponentName, 
                                                  const TSharedPtr<FJsonObject>& KwargsObject)
{
    return BasicWidgetFactory.CreateEditableText(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateEditableTextBox(UWidgetBlueprint* WidgetBlueprint, 
                                                     const FString& ComponentName, 
                                                     const TSharedPtr<FJsonObject>& KwargsObject)
{
    return BasicWidgetFactory.CreateEditableTextBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateCircularThrobber(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateCircularThrobber(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateSpinBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateSpinBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateWrapBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateWrapBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateScaleBox(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateScaleBox(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateNamedSlot(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateNamedSlot(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateRadialSlider(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateRadialSlider(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateListView(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateListView(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateTileView(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateTileView(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateTreeView(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateTreeView(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateSafeZone(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateSafeZone(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateMenuAnchor(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateMenuAnchor(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateNativeWidgetHost(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateNativeWidgetHost(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateBackgroundBlur(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return AdvancedWidgetFactory.CreateBackgroundBlur(WidgetBlueprint, ComponentName, KwargsObject);
}

UWidget* FWidgetComponentService::CreateUniformGridPanel(UWidgetBlueprint* WidgetBlueprint, const FString& ComponentName, const TSharedPtr<FJsonObject>& KwargsObject)
{
    return LayoutWidgetFactory.CreateUniformGridPanel(WidgetBlueprint, ComponentName, KwargsObject);
}

bool FWidgetComponentService::AddWidgetToTree(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
{
    if (!WidgetBlueprint || !Widget)
    {
        return false;
    }

    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("Widget blueprint has no WidgetTree"));
        return false;
    }

    // Get the root widget (should be a CanvasPanel)
    UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget;
    if (!RootWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Widget blueprint has no root widget"));
        return false;
    }

    // Try to cast to CanvasPanel (most common case)
    if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(RootWidget))
    {
        // Add the widget as a child of the canvas panel
        UCanvasPanelSlot* Slot = CanvasPanel->AddChildToCanvas(Widget);
        if (Slot)
        {
            // Set position and size
            Slot->SetPosition(Position);
            Slot->SetSize(Size);
            Slot->SetAlignment(FVector2D(0.0f, 0.0f)); // Top-left alignment
            
            UE_LOG(LogTemp, Log, TEXT("Added widget '%s' to canvas panel at position [%f, %f] with size [%f, %f]"), 
                *Widget->GetName(), Position.X, Position.Y, Size.X, Size.Y);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to add widget to canvas panel"));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Root widget is not a CanvasPanel, cannot set position/size. Root widget type: %s"), 
            *RootWidget->GetClass()->GetName());
        
        // For non-canvas panels, we can still try to add the widget but without position/size control
        // This would require different logic based on the panel type
        return false;
    }
}

void FWidgetComponentService::SaveWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
{
    if (!WidgetBlueprint)
    {
        return;
    }

    // Mark the blueprint as dirty
    WidgetBlueprint->MarkPackageDirty();
    
    // Compile the blueprint
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    
    // Save the asset
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);
    
    UE_LOG(LogTemp, Log, TEXT("Saved widget blueprint: %s"), *WidgetBlueprint->GetName());
}
