#include "Services/UMG/Widgets/BasicWidgetFactory.h"
#include "Editor/UMGEditor/Public/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

FBasicWidgetFactory::FBasicWidgetFactory()
{
}

bool FBasicWidgetFactory::GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, 
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

TSharedPtr<FJsonObject> FBasicWidgetFactory::GetKwargsToUse(const TSharedPtr<FJsonObject>& KwargsObject, const FString& ComponentName, const FString& ComponentType)
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

UWidget* FBasicWidgetFactory::CreateTextBlock(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    UE_LOG(LogTemp, Warning, TEXT("CreateTextBlock: Starting creation of TextBlock '%s'"), *ComponentName);
    
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateTextBlock: WidgetBlueprint is null"));
        return nullptr;
    }
    
    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateTextBlock: WidgetTree is null"));
        return nullptr;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CreateTextBlock: About to call ConstructWidget"));
    UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *ComponentName);
    
    if (!TextBlock)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateTextBlock: ConstructWidget returned null for '%s'"), *ComponentName);
        return nullptr;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CreateTextBlock: Successfully created TextBlock '%s'"), *ComponentName);
    
    // Apply text block specific properties
    FString Text;
    if (KwargsObject->TryGetStringField(TEXT("text"), Text))
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
    if (KwargsObject->TryGetNumberField(TEXT("font_size"), FontSize))
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
    if (GetJsonArray(KwargsObject, TEXT("color"), ColorArray) && ColorArray.Num() >= 3)
    {
        float R = ColorArray[0]->AsNumber();
        float G = ColorArray[1]->AsNumber();
        float B = ColorArray[2]->AsNumber();
        float A = ColorArray.Num() >= 4 ? ColorArray[3]->AsNumber() : 1.0f;
        
        UE_LOG(LogTemp, Display, TEXT("Setting color for TextBlock '%s' to [%f, %f, %f, %f]"), *ComponentName, R, G, B, A);
        FSlateColor TextColor(FLinearColor(R, G, B, A));
        TextBlock->SetColorAndOpacity(TextColor);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CreateTextBlock: Returning TextBlock '%s' successfully"), *ComponentName);
    return TextBlock;
}

UWidget* FBasicWidgetFactory::CreateButton(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FBasicWidgetFactory::CreateImage(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FBasicWidgetFactory::CreateCheckBox(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FBasicWidgetFactory::CreateSlider(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FBasicWidgetFactory::CreateProgressBar(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FBasicWidgetFactory::CreateEditableText(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FBasicWidgetFactory::CreateEditableTextBox(UWidgetBlueprint* WidgetBlueprint, 
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
