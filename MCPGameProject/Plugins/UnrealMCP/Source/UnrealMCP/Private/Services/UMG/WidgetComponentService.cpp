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
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
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
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(KwargsObject.ToSharedRef(), Writer);
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
    }
    
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
    UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *ComponentName);
    
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("TextBlock"));
    
    // Apply text block specific properties
    FString Text;
    if (KwargsToUse->TryGetStringField(TEXT("text"), Text))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting text for TextBlock '%s' to '%s'"), *ComponentName, *Text);
        TextBlock->SetText(FText::FromString(Text));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No 'text' field provided for TextBlock '%s'"), *ComponentName);
    }
    
    // Apply font size if provided
    int32 FontSize = 12;
    if (KwargsToUse->TryGetNumberField(TEXT("font_size"), FontSize))
    {
        // Apply scaling factor to convert from desired visual size to internal font size
        // Unreal applies a 4/3 (1.3333) multiplier, so we multiply by 4/3 to get the required size
        const float UE_FONT_SCALE_FACTOR = 4.0f / 3.0f; // 1.3333...
        int32 ScaledFontSize = FMath::RoundToInt(FontSize * UE_FONT_SCALE_FACTOR);
        
        UE_LOG(LogTemp, Display, TEXT("Setting font size for TextBlock '%s' to %d (scaled from %d)"), 
            *ComponentName, ScaledFontSize, FontSize);
        
        // Create a completely new font info instead of modifying existing
        // Directly create it with the requested size and preserve other properties
        FSlateFontInfo CurrentFont = TextBlock->GetFont();
        FSlateFontInfo NewFontInfo(
            CurrentFont.FontObject,
            ScaledFontSize,  // Use the scaled size to get the desired visual size
            CurrentFont.TypefaceFontName
        );
        
        // Preserve any other necessary font properties
        NewFontInfo.FontMaterial = CurrentFont.FontMaterial;
        NewFontInfo.OutlineSettings = CurrentFont.OutlineSettings;
        
        // Apply the font
        TextBlock->SetFont(NewFontInfo);
        
        // Force update
        TextBlock->SynchronizeProperties();
        WidgetBlueprint->MarkPackageDirty();
        
        UE_LOG(LogTemp, Display, TEXT("Applied new font with size %d to TextBlock '%s'"), 
            ScaledFontSize, *ComponentName);
    }
    
    // Apply text color if provided
    TArray<TSharedPtr<FJsonValue>> ColorArray;
    if (GetJsonArray(KwargsToUse, TEXT("color"), ColorArray) && ColorArray.Num() >= 3)
    {
        float R = ColorArray[0]->AsNumber();
        float G = ColorArray[1]->AsNumber();
        float B = ColorArray[2]->AsNumber();
        float A = ColorArray.Num() >= 4 ? ColorArray[3]->AsNumber() : 1.0f;
        
        UE_LOG(LogTemp, Display, TEXT("Setting color for TextBlock '%s' to [%f, %f, %f, %f]"), *ComponentName, R, G, B, A);
        FSlateColor TextColor(FLinearColor(R, G, B, A));
        TextBlock->SetColorAndOpacity(TextColor);
    }
    
    return TextBlock;
}

UWidget* FWidgetComponentService::CreateButton(UWidgetBlueprint* WidgetBlueprint, 
                                           const FString& ComponentName, 
                                           const TSharedPtr<FJsonObject>& KwargsObject)
{
    UButton* Button = WidgetBlueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *ComponentName);
    
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Button"));
    
    // Apply background color if provided
    TArray<TSharedPtr<FJsonValue>> BgColorArray;
    if (GetJsonArray(KwargsToUse, TEXT("background_color"), BgColorArray) && BgColorArray.Num() >= 3)
    {
        float R = BgColorArray[0]->AsNumber();
        float G = BgColorArray[1]->AsNumber();
        float B = BgColorArray[2]->AsNumber();
        float A = BgColorArray.Num() >= 4 ? BgColorArray[3]->AsNumber() : 1.0f;
        
        UE_LOG(LogTemp, Display, TEXT("Setting background color for Button '%s' to [%f, %f, %f, %f]"), *ComponentName, R, G, B, A);
        
        // Actually apply the button background color
        FSlateColor BackgroundColor(FLinearColor(R, G, B, A));
        Button->WidgetStyle.Normal.TintColor = BackgroundColor;
        Button->WidgetStyle.Hovered.TintColor = BackgroundColor;
        Button->WidgetStyle.Pressed.TintColor = BackgroundColor;
        
        UE_LOG(LogTemp, Display, TEXT("Applied background color [%f, %f, %f, %f] to Button '%s'"), 
            R, G, B, A, *ComponentName);
    }
    
    // Apply use brush transparency if provided
    bool bUseBrushTransparency = false;
    if (KwargsToUse->TryGetBoolField(TEXT("use_brush_transparency"), bUseBrushTransparency))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting brush draw type for Button '%s' to support transparency"), *ComponentName);
        
        // Update button style to use transparency
        FButtonStyle ButtonStyle = Button->GetStyle();
        ButtonStyle.Normal.DrawAs = bUseBrushTransparency ? ESlateBrushDrawType::Image : ESlateBrushDrawType::Box;
        ButtonStyle.Hovered.DrawAs = bUseBrushTransparency ? ESlateBrushDrawType::Image : ESlateBrushDrawType::Box;
        ButtonStyle.Pressed.DrawAs = bUseBrushTransparency ? ESlateBrushDrawType::Image : ESlateBrushDrawType::Box;
        ButtonStyle.Disabled.DrawAs = bUseBrushTransparency ? ESlateBrushDrawType::Image : ESlateBrushDrawType::Box;
        Button->SetStyle(ButtonStyle);
    }
    
    // Note: We no longer add text inside the button
    // Text should be added separately using a TextBlock and then arranged as a child of the button
    
    return Button;
}

UWidget* FWidgetComponentService::CreateImage(UWidgetBlueprint* WidgetBlueprint, 
                                          const FString& ComponentName, 
                                          const TSharedPtr<FJsonObject>& KwargsObject)
{
    UImage* Image = WidgetBlueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *ComponentName);
    
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Image"));
    
    // Apply image specific properties
    FString ImagePath;
    if (KwargsToUse->TryGetStringField(TEXT("image_path"), ImagePath) || 
        KwargsToUse->TryGetStringField(TEXT("brush_asset_path"), ImagePath))
    {
        if (!ImagePath.IsEmpty())
        {
            UE_LOG(LogTemp, Display, TEXT("Setting image path for Image '%s' to '%s'"), *ComponentName, *ImagePath);
            // In a full implementation, you would load the asset and set it as the image brush
            // For now we'll just log that we received the parameter
        }
    }
    
    // Apply brush color if provided
    TArray<TSharedPtr<FJsonValue>> BrushColorArray;
    if (GetJsonArray(KwargsToUse, TEXT("brush_color"), BrushColorArray) && BrushColorArray.Num() >= 3)
    {
        float R = BrushColorArray[0]->AsNumber();
        float G = BrushColorArray[1]->AsNumber();
        float B = BrushColorArray[2]->AsNumber();
        float A = BrushColorArray.Num() >= 4 ? BrushColorArray[3]->AsNumber() : 1.0f;
        
        UE_LOG(LogTemp, Display, TEXT("Setting brush color for Image '%s' to [%f, %f, %f, %f]"), *ComponentName, R, G, B, A);
        
        // Apply the brush color
        Image->SetColorAndOpacity(FLinearColor(R, G, B, A));
        
        UE_LOG(LogTemp, Display, TEXT("Applied brush color [%f, %f, %f, %f] to Image '%s'"), 
            R, G, B, A, *ComponentName);
    }
    
    // Apply use brush transparency if provided (for proper alpha handling)
    bool bUseBrushTransparency = false;
    if (KwargsToUse->TryGetBoolField(TEXT("use_brush_transparency"), bUseBrushTransparency))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting image brush draw type for Image '%s' to support transparency"), *ComponentName);
        
        FSlateBrush Brush = Image->GetBrush();
        Brush.DrawAs = bUseBrushTransparency ? ESlateBrushDrawType::Image : ESlateBrushDrawType::Box;
        Image->SetBrush(Brush);
    }
    
    return Image;
}

UWidget* FWidgetComponentService::CreateCheckBox(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("CheckBox"));
    
    // Check if text is provided
    FString Text;
    bool HasText = KwargsToUse->TryGetStringField(TEXT("text"), Text) && !Text.IsEmpty();
    
    if (!HasText)
    {
        // Simple case: no text, just create a checkbox
        UCheckBox* CheckBox = WidgetBlueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), *ComponentName);
        
        // Apply checkbox specific properties
        bool IsChecked = false;
        if (KwargsToUse->TryGetBoolField(TEXT("is_checked"), IsChecked))
        {
            CheckBox->SetIsChecked(IsChecked);
        }
        
        return CheckBox;
    }
    else
    {
        // Create a horizontal box to hold both the checkbox and the text
        UHorizontalBox* HBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), 
            *(ComponentName + TEXT("_Container"))
        );
        
        // Create the checkbox
        UCheckBox* CheckBox = WidgetBlueprint->WidgetTree->ConstructWidget<UCheckBox>(
            UCheckBox::StaticClass(), 
            *ComponentName
        );
        
        // Apply checkbox specific properties
        bool IsChecked = false;
        if (KwargsToUse->TryGetBoolField(TEXT("is_checked"), IsChecked))
        {
            CheckBox->SetIsChecked(IsChecked);
        }
        
        // Create the text block for the label
        UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), 
            *(ComponentName + TEXT("_Label"))
        );
        TextBlock->SetText(FText::FromString(Text));
        
        // Add padding between checkbox and label
        int32 Padding = 5;
        KwargsToUse->TryGetNumberField(TEXT("padding"), Padding);
        
        // Add checkbox to horizontal box
        HBox->AddChild(CheckBox);
        
        // Add text block to horizontal box
        UHorizontalBoxSlot* TextSlot = Cast<UHorizontalBoxSlot>(HBox->AddChild(TextBlock));
        if (TextSlot)
        {
            TextSlot->SetPadding(FMargin(Padding, 0, 0, 0));
            TextSlot->SetVerticalAlignment(VAlign_Center);
        }
        
        UE_LOG(LogTemp, Display, TEXT("Created CheckBox with text: %s"), *Text);
        
        return HBox;
    }
}

UWidget* FWidgetComponentService::CreateSlider(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    USlider* Slider = WidgetBlueprint->WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), *ComponentName);
    
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Slider"));
    
    // Apply slider specific properties
    float MinValue = 0.0f;
    if (KwargsToUse->TryGetNumberField(TEXT("min_value"), MinValue))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting min value for Slider '%s' to %f"), *ComponentName, MinValue);
        Slider->SetMinValue(MinValue);
    }
    
    float MaxValue = 1.0f;
    if (KwargsToUse->TryGetNumberField(TEXT("max_value"), MaxValue))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting max value for Slider '%s' to %f"), *ComponentName, MaxValue);
        Slider->SetMaxValue(MaxValue);
    }
    
    float Value = 0.5f;
    if (KwargsToUse->TryGetNumberField(TEXT("value"), Value))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting value for Slider '%s' to %f"), *ComponentName, Value);
        Slider->SetValue(Value);
    }
    
    FString Orientation;
    if (KwargsToUse->TryGetStringField(TEXT("orientation"), Orientation))
    {
        bool IsHorizontal = Orientation.Equals(TEXT("Horizontal"), ESearchCase::IgnoreCase);
        UE_LOG(LogTemp, Display, TEXT("Setting orientation for Slider '%s' to %s"), 
            *ComponentName, IsHorizontal ? TEXT("Horizontal") : TEXT("Vertical"));
        Slider->SetOrientation(IsHorizontal ? Orient_Horizontal : Orient_Vertical);
    }
    
    // Apply bar color if provided
    TArray<TSharedPtr<FJsonValue>> BarColorArray;
    if (GetJsonArray(KwargsToUse, TEXT("bar_color"), BarColorArray) && BarColorArray.Num() >= 3)
    {
        float R = BarColorArray[0]->AsNumber();
        float G = BarColorArray[1]->AsNumber();
        float B = BarColorArray[2]->AsNumber();
        float A = BarColorArray.Num() >= 4 ? BarColorArray[3]->AsNumber() : 1.0f;
        
        FLinearColor BarColor(R, G, B, A);
        Slider->SetSliderBarColor(BarColor);
        
        UE_LOG(LogTemp, Display, TEXT("Applied bar color [%f, %f, %f, %f] to Slider '%s'"), 
            R, G, B, A, *ComponentName);
    }
    
    return Slider;
}

UWidget* FWidgetComponentService::CreateProgressBar(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    UProgressBar* ProgressBar = WidgetBlueprint->WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *ComponentName);
    
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("ProgressBar"));
    
    // Apply progress bar specific properties
    float Percent = 0.5f;
    if (KwargsToUse->TryGetNumberField(TEXT("percent"), Percent))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting percent for ProgressBar '%s' to %f"), *ComponentName, Percent);
        ProgressBar->SetPercent(Percent);
    }
    
    // Apply fill color if provided
    TArray<TSharedPtr<FJsonValue>> FillColorArray;
    if (GetJsonArray(KwargsToUse, TEXT("fill_color"), FillColorArray) && FillColorArray.Num() >= 3)
    {
        float R = FillColorArray[0]->AsNumber();
        float G = FillColorArray[1]->AsNumber();
        float B = FillColorArray[2]->AsNumber();
        float A = FillColorArray.Num() >= 4 ? FillColorArray[3]->AsNumber() : 1.0f;
        
        FLinearColor FillColor(R, G, B, A);
        ProgressBar->SetFillColorAndOpacity(FillColor);
        
        UE_LOG(LogTemp, Display, TEXT("Applied fill color [%f, %f, %f, %f] to ProgressBar '%s'"), 
            R, G, B, A, *ComponentName);
    }
    
    return ProgressBar;
}

UWidget* FWidgetComponentService::CreateBorder(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *ComponentName);
    
    // Get the proper kwargs object (direct or nested)
    TSharedPtr<FJsonObject> KwargsToUse = GetKwargsToUse(KwargsObject, ComponentName, TEXT("Border"));
    
    // Apply border specific properties
    TArray<TSharedPtr<FJsonValue>> BrushColorArray;
    if (GetJsonArray(KwargsToUse, TEXT("background_color"), BrushColorArray) || 
        GetJsonArray(KwargsToUse, TEXT("brush_color"), BrushColorArray))
    {
        if (BrushColorArray.Num() >= 3)
        {
            float R = BrushColorArray[0]->AsNumber();
            float G = BrushColorArray[1]->AsNumber();
            float B = BrushColorArray[2]->AsNumber();
            float A = BrushColorArray.Num() >= 4 ? BrushColorArray[3]->AsNumber() : 1.0f;
            
            UE_LOG(LogTemp, Display, TEXT("Setting brush color for Border '%s' to [%f, %f, %f, %f]"), *ComponentName, R, G, B, A);
            
            // In UE 5.5, UBorder doesn't have SetBrushFromSlateBrush
            // Just set the brush color directly
            FLinearColor BrushColor(R, G, B, A);
            Border->SetBrushColor(BrushColor);
            
            UE_LOG(LogTemp, Display, TEXT("Applied brush color [%f, %f, %f, %f] to Border '%s'"), 
                R, G, B, A, *ComponentName);
        }
    }

    // Apply opacity if provided
    float Opacity = 1.0f;
    if (KwargsToUse->TryGetNumberField(TEXT("opacity"), Opacity))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting opacity for Border '%s' to %f"), *ComponentName, Opacity);
        Border->SetRenderOpacity(Opacity);
    }
    
    // Apply use brush transparency if provided
    bool bUseBrushTransparency = false;
    if (KwargsToUse->TryGetBoolField(TEXT("use_brush_transparency"), bUseBrushTransparency))
    {
        UE_LOG(LogTemp, Display, TEXT("Setting use brush transparency for Border '%s' to %d"), *ComponentName, bUseBrushTransparency);
        
        // In UE 5.5, we need to handle transparency differently
        // First, get the current brush color to preserve it
        FLinearColor BorderColor = Border->GetBrushColor();
        
        // For UE 5.5, we just set the brush color directly
        // The transparency will be controlled by the alpha value in the brush color
        Border->SetBrushColor(BorderColor);
        
        // In UE 5.5, transparency settings might be handled differently
        // Preserve the current opacity
        Border->SetRenderOpacity(Border->GetRenderOpacity());
        
        // Additional note: In UE 5.5, background transparency may depend on the material settings
        // Ensure your UI materials support proper alpha blending
        UE_LOG(LogTemp, Display, TEXT("Applied transparency settings to Border '%s'"), *ComponentName);
    }
    
    // Apply padding if provided
    TArray<TSharedPtr<FJsonValue>> PaddingArray;
    if (GetJsonArray(KwargsToUse, TEXT("padding"), PaddingArray))
    {
        if (PaddingArray.Num() >= 4)
        {
            // Padding format is Left, Top, Right, Bottom
            float Left = PaddingArray[0]->AsNumber();
            float Top = PaddingArray[1]->AsNumber();
            float Right = PaddingArray[2]->AsNumber();
            float Bottom = PaddingArray[3]->AsNumber();
            
            UE_LOG(LogTemp, Display, TEXT("Setting padding for Border '%s' to [%f, %f, %f, %f]"), *ComponentName, Left, Top, Right, Bottom);
            
            Border->SetPadding(FMargin(Left, Top, Right, Bottom));
        }
        else if (PaddingArray.Num() >= 1)
        {
            // Single value for all sides
            float Padding = PaddingArray[0]->AsNumber();
            
            UE_LOG(LogTemp, Display, TEXT("Setting uniform padding for Border '%s' to %f"), *ComponentName, Padding);
            
            Border->SetPadding(FMargin(Padding));
        }
    }
    
    return Border;
}

UWidget* FWidgetComponentService::CreateScrollBox(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UScrollBox* ScrollBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), *ComponentName);
    
    // Apply scroll box specific properties
    FString Orientation;
    if (KwargsObject->TryGetStringField(TEXT("orientation"), Orientation))
    {
        ScrollBox->SetOrientation(Orientation.Equals(TEXT("Horizontal"), ESearchCase::IgnoreCase) 
            ? Orient_Horizontal : Orient_Vertical);
    }
    
    FString ScrollBarVisibility;
    if (KwargsObject->TryGetStringField(TEXT("scroll_bar_visibility"), ScrollBarVisibility))
    {
        // Set scroll bar visibility based on string value
        // In a full implementation, you would map string values to ESlateVisibility enum
    }
    
    return ScrollBox;
}

UWidget* FWidgetComponentService::CreateSpacer(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    USpacer* Spacer = WidgetBlueprint->WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), *ComponentName);
    // No special properties for spacer beyond size
    return Spacer;
}

UWidget* FWidgetComponentService::CreateWidgetSwitcher(UWidgetBlueprint* WidgetBlueprint, 
                                                    const FString& ComponentName, 
                                                    const TSharedPtr<FJsonObject>& KwargsObject)
{
    UWidgetSwitcher* Switcher = WidgetBlueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), *ComponentName);
    
    // Apply widget switcher specific properties
    int32 ActiveIndex = 0;
    KwargsObject->TryGetNumberField(TEXT("active_widget_index"), ActiveIndex);
    Switcher->SetActiveWidgetIndex(ActiveIndex);
    
    return Switcher;
}

UWidget* FWidgetComponentService::CreateThrobber(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    UThrobber* Throbber = WidgetBlueprint->WidgetTree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), *ComponentName);
    
    // Apply throbber specific properties
    int32 NumPieces = 3; // Default value
    bool bFoundNumPieces = KwargsObject->TryGetNumberField(TEXT("number_of_pieces"), NumPieces);
    Throbber->SetNumberOfPieces(NumPieces);

    bool Animate = true; // Default value
    bool bFoundAnimate = KwargsObject->TryGetBoolField(TEXT("animate"), Animate);
    Throbber->SetAnimateHorizontally(Animate);
    Throbber->SetAnimateVertically(Animate);

    return Throbber;
}

UWidget* FWidgetComponentService::CreateExpandableArea(UWidgetBlueprint* WidgetBlueprint, 
                                                    const FString& ComponentName, 
                                                    const TSharedPtr<FJsonObject>& KwargsObject)
{
    // Create the ExpandableArea without trying to set header content
    UExpandableArea* ExpandableArea = WidgetBlueprint->WidgetTree->ConstructWidget<UExpandableArea>(UExpandableArea::StaticClass(), *ComponentName);
    
    // Create a text block for the header text separately
    FString HeaderText;
    UTextBlock* HeaderTextBlock = nullptr;
    if (KwargsObject->TryGetStringField(TEXT("header_text"), HeaderText))
    {
        // We'll create a separate text block with the header text
        HeaderTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), 
            *(ComponentName + TEXT("_HeaderText"))
        );
        HeaderTextBlock->SetText(FText::FromString(HeaderText));
    }
    
    // Set expansion state
    bool IsExpanded = false;
    KwargsObject->TryGetBoolField(TEXT("is_expanded"), IsExpanded);
    ExpandableArea->SetIsExpanded(IsExpanded);
    
    // Use both widgets - let the user arrange them in the Blueprint editor
    // We won't try to set one as the header of the other
    
    // Just log that the user will need to set up the header content manually
    UE_LOG(LogTemp, Warning, TEXT("Created ExpandableArea '%s'. In UE 5.5, you'll need to manually set the header content in the Widget Blueprint."), *ComponentName);
    
    return ExpandableArea;
}

UWidget* FWidgetComponentService::CreateRichTextBlock(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    URichTextBlock* RichTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), *ComponentName);
    
    // Apply rich text block specific properties
    FString Text;
    if (KwargsObject->TryGetStringField(TEXT("text"), Text))
    {
        RichTextBlock->SetText(FText::FromString(Text));
    }
    
    bool AutoWrapText = true;
    KwargsObject->TryGetBoolField(TEXT("auto_wrap_text"), AutoWrapText);
    RichTextBlock->SetAutoWrapText(AutoWrapText);
    
    return RichTextBlock;
}

UWidget* FWidgetComponentService::CreateMultiLineEditableText(UWidgetBlueprint* WidgetBlueprint, 
                                                           const FString& ComponentName, 
                                                           const TSharedPtr<FJsonObject>& KwargsObject)
{
    UMultiLineEditableText* TextBox = WidgetBlueprint->WidgetTree->ConstructWidget<UMultiLineEditableText>(UMultiLineEditableText::StaticClass(), *ComponentName);
    
    // Apply text box specific properties
    FString Text;
    if (KwargsObject->TryGetStringField(TEXT("text"), Text))
    {
        TextBox->SetText(FText::FromString(Text));
    }
    
    FString HintText;
    if (KwargsObject->TryGetStringField(TEXT("hint_text"), HintText))
    {
        TextBox->SetHintText(FText::FromString(HintText));
    }
    
    return TextBox;
}

UWidget* FWidgetComponentService::CreateVerticalBox(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    UVerticalBox* VerticalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *ComponentName);
    // No special properties for vertical box beyond children
    return VerticalBox;
}

UWidget* FWidgetComponentService::CreateHorizontalBox(UWidgetBlueprint* WidgetBlueprint, 
                                                   const FString& ComponentName, 
                                                   const TSharedPtr<FJsonObject>& KwargsObject)
{
    UHorizontalBox* HorizontalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *ComponentName);
    // No special properties for horizontal box beyond children
    return HorizontalBox;
}

UWidget* FWidgetComponentService::CreateOverlay(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    UOverlay* Overlay = WidgetBlueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *ComponentName);
    // No special properties for overlay beyond children
    return Overlay;
}

UWidget* FWidgetComponentService::CreateGridPanel(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UGridPanel* GridPanel = WidgetBlueprint->WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), *ComponentName);
    
    // Get column and row fill properties if provided
    int32 ColumnCount = 2;
    KwargsObject->TryGetNumberField(TEXT("column_count"), ColumnCount);
    
    int32 RowCount = 2;
    KwargsObject->TryGetNumberField(TEXT("row_count"), RowCount);
    
    // In a more complete implementation, you might set up initial columns/rows
    // but this requires more complex setup that's usually done when children are added
    
    return GridPanel;
}

UWidget* FWidgetComponentService::CreateSizeBox(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    USizeBox* SizeBox = WidgetBlueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *ComponentName);
    
    // Apply size box specific properties
    float MinWidth = 0.0f;
    if (KwargsObject->TryGetNumberField(TEXT("min_width"), MinWidth) && MinWidth > 0.0f)
    {
        SizeBox->SetMinDesiredWidth(MinWidth);
    }
    
    float MinHeight = 0.0f;
    if (KwargsObject->TryGetNumberField(TEXT("min_height"), MinHeight) && MinHeight > 0.0f)
    {
        SizeBox->SetMinDesiredHeight(MinHeight);
    }
    
    float MaxWidth = 0.0f;
    if (KwargsObject->TryGetNumberField(TEXT("max_width"), MaxWidth) && MaxWidth > 0.0f)
    {
        SizeBox->SetMaxDesiredWidth(MaxWidth);
    }
    
    float MaxHeight = 0.0f;
    if (KwargsObject->TryGetNumberField(TEXT("max_height"), MaxHeight) && MaxHeight > 0.0f)
    {
        SizeBox->SetMaxDesiredHeight(MaxHeight);
    }
    
    return SizeBox;
}

UWidget* FWidgetComponentService::CreateCanvasPanel(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    UCanvasPanel* CanvasPanel = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), *ComponentName);
    // No special properties for canvas panel beyond children
    return CanvasPanel;
}

UWidget* FWidgetComponentService::CreateComboBox(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    UComboBoxString* ComboBox = WidgetBlueprint->WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), *ComponentName);
    
    // NOTE: Setting options via kwargs only affects the runtime instance.
    // The 'Default Options' array in the editor details panel is not populated by this method,
    // likely because UComboBoxString::DefaultOptions is a private/protected member or intended for editor-only modification.

    // Set runtime options if provided
    TArray<TSharedPtr<FJsonValue>> Options;
    if (GetJsonArray(KwargsObject, TEXT("options"), Options))
    {
        // ComboBox->ClearOptions(); // Optional: Clear existing runtime options first
        for (const TSharedPtr<FJsonValue>& OptionJson : Options)
        {
            FString OptionText = OptionJson->AsString();
            ComboBox->AddOption(OptionText); 
        }
    }
    
    // Set runtime selected option if provided
    FString SelectedOptionString;
    if (KwargsObject->TryGetStringField(TEXT("selected_option"), SelectedOptionString) && !SelectedOptionString.IsEmpty())
    {
        ComboBox->SetSelectedOption(SelectedOptionString);
    }
    
    return ComboBox;
}

UWidget* FWidgetComponentService::CreateEditableText(UWidgetBlueprint* WidgetBlueprint, 
                                                  const FString& ComponentName, 
                                                  const TSharedPtr<FJsonObject>& KwargsObject)
{
    UEditableText* TextEdit = WidgetBlueprint->WidgetTree->ConstructWidget<UEditableText>(UEditableText::StaticClass(), *ComponentName);
    
    // Apply editable text specific properties
    FString Text;
    if (KwargsObject->TryGetStringField(TEXT("text"), Text))
    {
        TextEdit->SetText(FText::FromString(Text));
    }
    
    FString HintText;
    if (KwargsObject->TryGetStringField(TEXT("hint_text"), HintText))
    {
        TextEdit->SetHintText(FText::FromString(HintText));
    }
    
    bool IsPassword = false;
    if (KwargsObject->TryGetBoolField(TEXT("is_password"), IsPassword))
    {
        TextEdit->SetIsPassword(IsPassword);
    }
    
    bool IsReadOnly = false;
    if (KwargsObject->TryGetBoolField(TEXT("is_read_only"), IsReadOnly))
    {
        TextEdit->SetIsReadOnly(IsReadOnly);
    }
    
    return TextEdit;
}

UWidget* FWidgetComponentService::CreateEditableTextBox(UWidgetBlueprint* WidgetBlueprint, 
                                                     const FString& ComponentName, 
                                                     const TSharedPtr<FJsonObject>& KwargsObject)
{
    UEditableTextBox* TextBox = WidgetBlueprint->WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), *ComponentName);
    
    // Apply editable text box specific properties
    FString Text;
    if (KwargsObject->TryGetStringField(TEXT("text"), Text))
    {
        TextBox->SetText(FText::FromString(Text));
    }
    
    FString HintText;
    if (KwargsObject->TryGetStringField(TEXT("hint_text"), HintText))
    {
        TextBox->SetHintText(FText::FromString(HintText));
    }
    
    bool IsPassword = false;
    if (KwargsObject->TryGetBoolField(TEXT("is_password"), IsPassword))
    {
        TextBox->SetIsPassword(IsPassword);
    }
    
    bool IsReadOnly = false;
    if (KwargsObject->TryGetBoolField(TEXT("is_read_only"), IsReadOnly))
    {
        TextBox->SetIsReadOnly(IsReadOnly);
    }
    
    return TextBox;
}

UWidget* FWidgetComponentService::CreateCircularThrobber(UWidgetBlueprint* WidgetBlueprint, 
                                                      const FString& ComponentName, 
                                                      const TSharedPtr<FJsonObject>& KwargsObject)
{
    UCircularThrobber* Throbber = WidgetBlueprint->WidgetTree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), *ComponentName);
    
    // Apply circular throbber specific properties
    int32 NumPieces = 8;
    KwargsObject->TryGetNumberField(TEXT("number_of_pieces"), NumPieces);
    Throbber->SetNumberOfPieces(NumPieces);
    
    float Period = 0.75f;
    KwargsObject->TryGetNumberField(TEXT("period"), Period);
    Throbber->SetPeriod(Period);
    
    float Radius = 16.0f;
    KwargsObject->TryGetNumberField(TEXT("radius"), Radius);
    Throbber->SetRadius(Radius);
    
    return Throbber;
}

UWidget* FWidgetComponentService::CreateSpinBox(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    USpinBox* SpinBox = WidgetBlueprint->WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), *ComponentName);
    
    // Apply spin box specific properties
    float MinValue = 0.0f;
    KwargsObject->TryGetNumberField(TEXT("min_value"), MinValue);
    SpinBox->SetMinValue(MinValue);
    
    float MaxValue = 100.0f;
    KwargsObject->TryGetNumberField(TEXT("max_value"), MaxValue);
    SpinBox->SetMaxValue(MaxValue);
    
    float Value = 0.0f;
    KwargsObject->TryGetNumberField(TEXT("value"), Value);
    SpinBox->SetValue(Value);
    
    float StepSize = 1.0f;
    KwargsObject->TryGetNumberField(TEXT("step_size"), StepSize);
    SpinBox->SetMinSliderValue(StepSize);
    
    return SpinBox;
}

UWidget* FWidgetComponentService::CreateWrapBox(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    UWrapBox* WrapBox = WidgetBlueprint->WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), *ComponentName);
    
    // Apply wrap box specific properties
    float WrapWidth = 500.0f;
    KwargsObject->TryGetNumberField(TEXT("wrap_width"), WrapWidth);
    // Note: In UE5.5, SetWrapWidth is not available - wrap width needs to be configured in the Widget Editor
    
    // Apply wrap horizontal/vertical alignment
    FString HorizontalAlignment;
    if (KwargsObject->TryGetStringField(TEXT("horizontal_alignment"), HorizontalAlignment))
    {
        if (HorizontalAlignment.Equals(TEXT("Left"), ESearchCase::IgnoreCase))
            WrapBox->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
        else if (HorizontalAlignment.Equals(TEXT("Center"), ESearchCase::IgnoreCase))
            WrapBox->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
        else if (HorizontalAlignment.Equals(TEXT("Right"), ESearchCase::IgnoreCase))
            WrapBox->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
    }
    
    return WrapBox;
}

UWidget* FWidgetComponentService::CreateScaleBox(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    UScaleBox* ScaleBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), *ComponentName);
    
    // Apply scale box specific properties
    FString StretchDirection;
    if (KwargsObject->TryGetStringField(TEXT("stretch_direction"), StretchDirection))
    {
        if (StretchDirection.Equals(TEXT("Both"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretchDirection(EStretchDirection::Both);
        else if (StretchDirection.Equals(TEXT("DownOnly"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretchDirection(EStretchDirection::DownOnly);
        else if (StretchDirection.Equals(TEXT("UpOnly"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretchDirection(EStretchDirection::UpOnly);
    }
    
    FString Stretch;
    if (KwargsObject->TryGetStringField(TEXT("stretch"), Stretch))
    {
        if (Stretch.Equals(TEXT("None"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretch(EStretch::None);
        else if (Stretch.Equals(TEXT("Fill"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretch(EStretch::Fill);
        else if (Stretch.Equals(TEXT("ScaleToFit"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretch(EStretch::ScaleToFit);
        else if (Stretch.Equals(TEXT("ScaleToFitX"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretch(EStretch::ScaleToFitX);
        else if (Stretch.Equals(TEXT("ScaleToFitY"), ESearchCase::IgnoreCase))
            ScaleBox->SetStretch(EStretch::ScaleToFitY);
    }
    
    float UserSpecifiedScale = 1.0f;
    if (KwargsObject->TryGetNumberField(TEXT("scale"), UserSpecifiedScale))
    {
        ScaleBox->SetUserSpecifiedScale(UserSpecifiedScale);
    }
    
    return ScaleBox;
}

UWidget* FWidgetComponentService::CreateNamedSlot(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UNamedSlot* NamedSlot = WidgetBlueprint->WidgetTree->ConstructWidget<UNamedSlot>(UNamedSlot::StaticClass(), *ComponentName);
    // No special properties for named slot beyond standard widget properties
    return NamedSlot;
}

UWidget* FWidgetComponentService::CreateRadialSlider(UWidgetBlueprint* WidgetBlueprint, 
                                                  const FString& ComponentName, 
                                                  const TSharedPtr<FJsonObject>& KwargsObject)
{
    URadialSlider* RadialSlider = WidgetBlueprint->WidgetTree->ConstructWidget<URadialSlider>(URadialSlider::StaticClass(), *ComponentName);
    
    // Apply radial slider specific properties
    float Value = 0.0f;
    KwargsObject->TryGetNumberField(TEXT("value"), Value);
    RadialSlider->SetValue(Value);
    
    // Add support for start/end angle
    float StartAngle = 0.0f;
    if (KwargsObject->TryGetNumberField(TEXT("slider_handle_start_angle"), StartAngle)) {
        RadialSlider->SetSliderHandleStartAngle(StartAngle);
    }
    float EndAngle = 360.0f;
    if (KwargsObject->TryGetNumberField(TEXT("slider_handle_end_angle"), EndAngle)) {
        RadialSlider->SetSliderHandleEndAngle(EndAngle);
    }
    
    float MinValue = 0.0f;
    KwargsObject->TryGetNumberField(TEXT("min_value"), MinValue);
    // RadialSlider->MinValue = MinValue; // Directly setting the member variable - Caused compilation error
    UE_LOG(LogTemp, Warning, TEXT("URadialSlider: MinValue cannot be set directly via code in this UE version. Please set it in the Widget Blueprint editor."));
    
    float MaxValue = 1.0f;
    KwargsObject->TryGetNumberField(TEXT("max_value"), MaxValue);
    // RadialSlider->MaxValue = MaxValue; // Directly setting the member variable - Caused compilation error
    UE_LOG(LogTemp, Warning, TEXT("URadialSlider: MaxValue cannot be set directly via code in this UE version. Please set it in the Widget Blueprint editor."));
    
    // Set slider colors if provided
    TArray<TSharedPtr<FJsonValue>> SliderBarColorArray;
    if (GetJsonArray(KwargsObject, TEXT("slider_bar_color"), SliderBarColorArray) && SliderBarColorArray.Num() >= 3)
    {
        float R = SliderBarColorArray[0]->AsNumber();
        float G = SliderBarColorArray[1]->AsNumber();
        float B = SliderBarColorArray[2]->AsNumber();
        float A = SliderBarColorArray.Num() >= 4 ? SliderBarColorArray[3]->AsNumber() : 1.0f;
        
        FLinearColor SliderBarColor(R, G, B, A);
        // RadialSlider doesn't have a SetSliderBarColor method in UE 5.5
        // We'd need to set widget style properties to change colors
    }
    
    return RadialSlider;
}

UWidget* FWidgetComponentService::CreateListView(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    UListView* ListView = WidgetBlueprint->WidgetTree->ConstructWidget<UListView>(UListView::StaticClass(), *ComponentName);
    
    // ListView requires more setup in Blueprint for data binding to be useful
    // We'll create the base widget here, and the user can configure it in the Blueprint editor
    
    // Set basic properties if available
    FString SelectionMode;
    if (KwargsObject->TryGetStringField(TEXT("selection_mode"), SelectionMode))
    {
        if (SelectionMode.Equals(TEXT("Single"), ESearchCase::IgnoreCase))
            ListView->SetSelectionMode(ESelectionMode::Single);
        else if (SelectionMode.Equals(TEXT("Multi"), ESearchCase::IgnoreCase))
            ListView->SetSelectionMode(ESelectionMode::Multi);
        else if (SelectionMode.Equals(TEXT("None"), ESearchCase::IgnoreCase))
            ListView->SetSelectionMode(ESelectionMode::None);
    }
    
    return ListView;
}

UWidget* FWidgetComponentService::CreateTileView(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    UTileView* TileView = WidgetBlueprint->WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), *ComponentName);
    
    // TileView requires more setup in Blueprint for data binding to be useful
    // We'll create the base widget here, and the user can configure it in the Blueprint editor
    
    // Set basic properties if available
    float EntryWidth = 128.0f;
    KwargsObject->TryGetNumberField(TEXT("entry_width"), EntryWidth);
    TileView->SetEntryWidth(EntryWidth);
    
    float EntryHeight = 128.0f;
    KwargsObject->TryGetNumberField(TEXT("entry_height"), EntryHeight);
    TileView->SetEntryHeight(EntryHeight);
    
    return TileView;
}

UWidget* FWidgetComponentService::CreateTreeView(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    UTreeView* TreeView = WidgetBlueprint->WidgetTree->ConstructWidget<UTreeView>(UTreeView::StaticClass(), *ComponentName);
    
    // TreeView requires more setup in Blueprint for data binding to be useful
    // We'll create the base widget here, and the user can configure it in the Blueprint editor
    
    return TreeView;
}

UWidget* FWidgetComponentService::CreateSafeZone(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    USafeZone* SafeZone = WidgetBlueprint->WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), *ComponentName);
    
    // Apply safe zone specific properties
    bool IsTitleSafe = true;
    KwargsObject->TryGetBoolField(TEXT("is_title_safe"), IsTitleSafe);
    // UE 5.5 doesn't have SetIsTitleSafe method for USafeZone
    // SafeZone->SetIsTitleSafe(IsTitleSafe);
    
    // Set padding scale if provided
    float PaddingScale = 1.0f;
    KwargsObject->TryGetNumberField(TEXT("padding_scale"), PaddingScale);
    // UE 5.5 doesn't have SetPadding method for USafeZone
    // SafeZone->SetPadding(FMargin(PaddingScale));
    
    return SafeZone;
}

UWidget* FWidgetComponentService::CreateMenuAnchor(UWidgetBlueprint* WidgetBlueprint, 
                                                const FString& ComponentName, 
                                                const TSharedPtr<FJsonObject>& KwargsObject)
{
    UMenuAnchor* MenuAnchor = WidgetBlueprint->WidgetTree->ConstructWidget<UMenuAnchor>(UMenuAnchor::StaticClass(), *ComponentName);
    
    // MenuAnchor requires setup in Blueprint to be useful
    // For the menu content, we'll need to bind a function in Blueprint
    
    // Set basic properties if available
    FString Placement;
    if (KwargsObject->TryGetStringField(TEXT("placement"), Placement))
    {
        if (Placement.Equals(TEXT("ComboBox"), ESearchCase::IgnoreCase))
            MenuAnchor->SetPlacement(MenuPlacement_ComboBox);
        else if (Placement.Equals(TEXT("BelowAnchor"), ESearchCase::IgnoreCase))
            MenuAnchor->SetPlacement(MenuPlacement_BelowAnchor);
        else if (Placement.Equals(TEXT("CenteredBelowAnchor"), ESearchCase::IgnoreCase))
            MenuAnchor->SetPlacement(MenuPlacement_CenteredBelowAnchor);
        else if (Placement.Equals(TEXT("AboveAnchor"), ESearchCase::IgnoreCase))
            MenuAnchor->SetPlacement(MenuPlacement_AboveAnchor);
        else if (Placement.Equals(TEXT("CenteredAboveAnchor"), ESearchCase::IgnoreCase))
            MenuAnchor->SetPlacement(MenuPlacement_CenteredAboveAnchor);
    }
    
    return MenuAnchor;
}

UWidget* FWidgetComponentService::CreateNativeWidgetHost(UWidgetBlueprint* WidgetBlueprint, 
                                                      const FString& ComponentName, 
                                                      const TSharedPtr<FJsonObject>& KwargsObject)
{
    UNativeWidgetHost* NativeWidgetHost = WidgetBlueprint->WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), *ComponentName);
    // NativeWidgetHost requires platform-specific setup to be useful
    UE_LOG(LogTemp, Warning, TEXT("Created NativeWidgetHost '%s'. Additional platform-specific setup may be required in Blueprint."), *ComponentName);
    
    return NativeWidgetHost;
}

UWidget* FWidgetComponentService::CreateBackgroundBlur(UWidgetBlueprint* WidgetBlueprint, 
                                                    const FString& ComponentName, 
                                                    const TSharedPtr<FJsonObject>& KwargsObject)
{
    UBackgroundBlur* BackgroundBlur = WidgetBlueprint->WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), *ComponentName);
    
    // Apply background blur specific properties
    float BlurStrength = 5.0f;
    KwargsObject->TryGetNumberField(TEXT("blur_strength"), BlurStrength);
    BackgroundBlur->SetBlurStrength(BlurStrength);
    
    bool ApplyAlphaToBlur = true;
    KwargsObject->TryGetBoolField(TEXT("apply_alpha_to_blur"), ApplyAlphaToBlur);
    BackgroundBlur->SetApplyAlphaToBlur(ApplyAlphaToBlur);
    
    // Set blur background color if provided
    TArray<TSharedPtr<FJsonValue>> ColorArray;
    if (GetJsonArray(KwargsObject, TEXT("background_color"), ColorArray) && ColorArray.Num() >= 3)
    {
        float R = ColorArray[0]->AsNumber();
        float G = ColorArray[1]->AsNumber();
        float B = ColorArray[2]->AsNumber();
        float A = ColorArray.Num() >= 4 ? ColorArray[3]->AsNumber() : 1.0f;
        
        // UE 5.5 doesn't have SetBackgroundColor method for UBackgroundBlur
        // BackgroundBlur->SetBackgroundColor(FLinearColor(R, G, B, A));
    }
    
    return BackgroundBlur;
}

UWidget* FWidgetComponentService::CreateUniformGridPanel(UWidgetBlueprint* WidgetBlueprint, 
                                                      const FString& ComponentName, 
                                                      const TSharedPtr<FJsonObject>& KwargsObject)
{
    UUniformGridPanel* UniformGrid = WidgetBlueprint->WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), *ComponentName);
    
    // Apply uniform grid panel specific properties
    int32 SlotPadding = 0;
    KwargsObject->TryGetNumberField(TEXT("slot_padding"), SlotPadding);
    UniformGrid->SetSlotPadding(FVector2D(SlotPadding, SlotPadding));
    
    int32 MinDesiredSlotWidth = 0;
    KwargsObject->TryGetNumberField(TEXT("min_desired_slot_width"), MinDesiredSlotWidth);
    UniformGrid->SetMinDesiredSlotWidth(MinDesiredSlotWidth);
    
    int32 MinDesiredSlotHeight = 0;
    KwargsObject->TryGetNumberField(TEXT("min_desired_slot_height"), MinDesiredSlotHeight);
    UniformGrid->SetMinDesiredSlotHeight(MinDesiredSlotHeight);
    
    return UniformGrid;
} 