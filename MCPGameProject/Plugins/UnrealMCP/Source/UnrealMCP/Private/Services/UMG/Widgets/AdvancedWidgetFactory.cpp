#include "Services/UMG/Widgets/AdvancedWidgetFactory.h"
#include "Editor/UMGEditor/Public/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Throbber.h"
#include "Components/ExpandableArea.h"
#include "Components/RichTextBlock.h"
#include "Components/MultiLineEditableText.h"
#include "Components/CircularThrobber.h"
#include "Components/SpinBox.h"
#include "Components/RadialSlider.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/SafeZone.h"
#include "Components/MenuAnchor.h"
#include "Components/NativeWidgetHost.h"
#include "Components/BackgroundBlur.h"
#include "Components/ScaleBox.h"
#include "Components/NamedSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"

FAdvancedWidgetFactory::FAdvancedWidgetFactory()
{
}

bool FAdvancedWidgetFactory::GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, 
                                         const FString& FieldName, 
                                         TArray<TSharedPtr<FJsonValue>>& OutArray)
{
    if (!JsonObject->HasField(FieldName))
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* ArrayPtr;
    bool bSuccess = JsonObject->TryGetArrayField(FStringView(*FieldName), ArrayPtr);
    if (bSuccess)
    {
        OutArray = *ArrayPtr;
        return true;
    }
    return false;
}

UWidget* FAdvancedWidgetFactory::CreateBorder(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *ComponentName);
    
    // Apply border specific properties
    TArray<TSharedPtr<FJsonValue>> BrushColorArray;
    if (GetJsonArray(KwargsObject, TEXT("background_color"), BrushColorArray) || 
        GetJsonArray(KwargsObject, TEXT("brush_color"), BrushColorArray))
    {
        if (BrushColorArray.Num() >= 3)
        {
            float R = BrushColorArray[0]->AsNumber();
            float G = BrushColorArray[1]->AsNumber();
            float B = BrushColorArray[2]->AsNumber();
            float A = BrushColorArray.Num() >= 4 ? BrushColorArray[3]->AsNumber() : 1.0f;
            
            FLinearColor BrushColor(R, G, B, A);
            Border->SetBrushColor(BrushColor);
        }
    }

    float Opacity = 1.0f;
    if (KwargsObject->TryGetNumberField(TEXT("opacity"), Opacity))
    {
        Border->SetRenderOpacity(Opacity);
    }
    
    bool bUseBrushTransparency = false;
    if (KwargsObject->TryGetBoolField(TEXT("use_brush_transparency"), bUseBrushTransparency))
    {
        FLinearColor BorderColor = Border->GetBrushColor();
        Border->SetBrushColor(BorderColor);
        Border->SetRenderOpacity(Border->GetRenderOpacity());
    }
    
    TArray<TSharedPtr<FJsonValue>> PaddingArray;
    if (GetJsonArray(KwargsObject, TEXT("padding"), PaddingArray))
    {
        if (PaddingArray.Num() >= 4)
        {
            float Left = PaddingArray[0]->AsNumber();
            float Top = PaddingArray[1]->AsNumber();
            float Right = PaddingArray[2]->AsNumber();
            float Bottom = PaddingArray[3]->AsNumber();
            Border->SetPadding(FMargin(Left, Top, Right, Bottom));
        }
        else if (PaddingArray.Num() >= 1)
        {
            float Padding = PaddingArray[0]->AsNumber();
            Border->SetPadding(FMargin(Padding));
        }
    }
    
    return Border;
}

UWidget* FAdvancedWidgetFactory::CreateSpacer(UWidgetBlueprint* WidgetBlueprint, 
                                             const FString& ComponentName, 
                                             const TSharedPtr<FJsonObject>& KwargsObject)
{
    USpacer* Spacer = WidgetBlueprint->WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), *ComponentName);
    return Spacer;
}

UWidget* FAdvancedWidgetFactory::CreateWidgetSwitcher(UWidgetBlueprint* WidgetBlueprint, 
                                                     const FString& ComponentName, 
                                                     const TSharedPtr<FJsonObject>& KwargsObject)
{
    UWidgetSwitcher* Switcher = WidgetBlueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), *ComponentName);
    
    int32 ActiveIndex = 0;
    KwargsObject->TryGetNumberField(TEXT("active_widget_index"), ActiveIndex);
    Switcher->SetActiveWidgetIndex(ActiveIndex);
    
    return Switcher;
}

UWidget* FAdvancedWidgetFactory::CreateThrobber(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UThrobber* Throbber = WidgetBlueprint->WidgetTree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), *ComponentName);
    
    int32 NumPieces = 3;
    KwargsObject->TryGetNumberField(TEXT("number_of_pieces"), NumPieces);
    Throbber->SetNumberOfPieces(NumPieces);

    bool Animate = true;
    KwargsObject->TryGetBoolField(TEXT("animate"), Animate);
    Throbber->SetAnimateHorizontally(Animate);
    Throbber->SetAnimateVertically(Animate);

    return Throbber;
}

UWidget* FAdvancedWidgetFactory::CreateExpandableArea(UWidgetBlueprint* WidgetBlueprint, 
                                                     const FString& ComponentName, 
                                                     const TSharedPtr<FJsonObject>& KwargsObject)
{
    UExpandableArea* ExpandableArea = WidgetBlueprint->WidgetTree->ConstructWidget<UExpandableArea>(UExpandableArea::StaticClass(), *ComponentName);
    
    FString HeaderText;
    if (KwargsObject->TryGetStringField(TEXT("header_text"), HeaderText))
    {
        UTextBlock* HeaderTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), 
            *(ComponentName + TEXT("_HeaderText"))
        );
        HeaderTextBlock->SetText(FText::FromString(HeaderText));
    }
    
    bool IsExpanded = false;
    KwargsObject->TryGetBoolField(TEXT("is_expanded"), IsExpanded);
    ExpandableArea->SetIsExpanded(IsExpanded);
    
    return ExpandableArea;
}

UWidget* FAdvancedWidgetFactory::CreateRichTextBlock(UWidgetBlueprint* WidgetBlueprint, 
                                                    const FString& ComponentName, 
                                                    const TSharedPtr<FJsonObject>& KwargsObject)
{
    URichTextBlock* RichTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateMultiLineEditableText(UWidgetBlueprint* WidgetBlueprint, 
                                                            const FString& ComponentName, 
                                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    UMultiLineEditableText* TextBox = WidgetBlueprint->WidgetTree->ConstructWidget<UMultiLineEditableText>(UMultiLineEditableText::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateCircularThrobber(UWidgetBlueprint* WidgetBlueprint, 
                                                       const FString& ComponentName, 
                                                       const TSharedPtr<FJsonObject>& KwargsObject)
{
    UCircularThrobber* Throbber = WidgetBlueprint->WidgetTree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateSpinBox(UWidgetBlueprint* WidgetBlueprint, 
                                              const FString& ComponentName, 
                                              const TSharedPtr<FJsonObject>& KwargsObject)
{
    USpinBox* SpinBox = WidgetBlueprint->WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateRadialSlider(UWidgetBlueprint* WidgetBlueprint, 
                                                   const FString& ComponentName, 
                                                   const TSharedPtr<FJsonObject>& KwargsObject)
{
    URadialSlider* RadialSlider = WidgetBlueprint->WidgetTree->ConstructWidget<URadialSlider>(URadialSlider::StaticClass(), *ComponentName);
    
    float Value = 0.0f;
    KwargsObject->TryGetNumberField(TEXT("value"), Value);
    RadialSlider->SetValue(Value);
    
    float StartAngle = 0.0f;
    if (KwargsObject->TryGetNumberField(TEXT("slider_handle_start_angle"), StartAngle))
    {
        RadialSlider->SetSliderHandleStartAngle(StartAngle);
    }
    
    float EndAngle = 360.0f;
    if (KwargsObject->TryGetNumberField(TEXT("slider_handle_end_angle"), EndAngle))
    {
        RadialSlider->SetSliderHandleEndAngle(EndAngle);
    }
    
    return RadialSlider;
}

UWidget* FAdvancedWidgetFactory::CreateListView(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UListView* ListView = WidgetBlueprint->WidgetTree->ConstructWidget<UListView>(UListView::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateTileView(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UTileView* TileView = WidgetBlueprint->WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), *ComponentName);
    
    float EntryWidth = 128.0f;
    KwargsObject->TryGetNumberField(TEXT("entry_width"), EntryWidth);
    TileView->SetEntryWidth(EntryWidth);
    
    float EntryHeight = 128.0f;
    KwargsObject->TryGetNumberField(TEXT("entry_height"), EntryHeight);
    TileView->SetEntryHeight(EntryHeight);
    
    return TileView;
}

UWidget* FAdvancedWidgetFactory::CreateTreeView(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UTreeView* TreeView = WidgetBlueprint->WidgetTree->ConstructWidget<UTreeView>(UTreeView::StaticClass(), *ComponentName);
    return TreeView;
}

UWidget* FAdvancedWidgetFactory::CreateSafeZone(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    USafeZone* SafeZone = WidgetBlueprint->WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), *ComponentName);
    return SafeZone;
}

UWidget* FAdvancedWidgetFactory::CreateMenuAnchor(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    UMenuAnchor* MenuAnchor = WidgetBlueprint->WidgetTree->ConstructWidget<UMenuAnchor>(UMenuAnchor::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateNativeWidgetHost(UWidgetBlueprint* WidgetBlueprint, 
                                                       const FString& ComponentName, 
                                                       const TSharedPtr<FJsonObject>& KwargsObject)
{
    UNativeWidgetHost* NativeWidgetHost = WidgetBlueprint->WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), *ComponentName);
    return NativeWidgetHost;
}

UWidget* FAdvancedWidgetFactory::CreateBackgroundBlur(UWidgetBlueprint* WidgetBlueprint, 
                                                     const FString& ComponentName, 
                                                     const TSharedPtr<FJsonObject>& KwargsObject)
{
    UBackgroundBlur* BackgroundBlur = WidgetBlueprint->WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), *ComponentName);
    
    float BlurStrength = 5.0f;
    KwargsObject->TryGetNumberField(TEXT("blur_strength"), BlurStrength);
    BackgroundBlur->SetBlurStrength(BlurStrength);
    
    bool ApplyAlphaToBlur = true;
    KwargsObject->TryGetBoolField(TEXT("apply_alpha_to_blur"), ApplyAlphaToBlur);
    BackgroundBlur->SetApplyAlphaToBlur(ApplyAlphaToBlur);
    
    return BackgroundBlur;
}

UWidget* FAdvancedWidgetFactory::CreateScaleBox(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UScaleBox* ScaleBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), *ComponentName);
    
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

UWidget* FAdvancedWidgetFactory::CreateNamedSlot(UWidgetBlueprint* WidgetBlueprint, 
                                                const FString& ComponentName, 
                                                const TSharedPtr<FJsonObject>& KwargsObject)
{
    UNamedSlot* NamedSlot = WidgetBlueprint->WidgetTree->ConstructWidget<UNamedSlot>(UNamedSlot::StaticClass(), *ComponentName);
    return NamedSlot;
}

UWidget* FAdvancedWidgetFactory::CreateComboBox(UWidgetBlueprint* WidgetBlueprint, 
                                               const FString& ComponentName, 
                                               const TSharedPtr<FJsonObject>& KwargsObject)
{
    UComboBoxString* ComboBox = WidgetBlueprint->WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), *ComponentName);
    
    TArray<TSharedPtr<FJsonValue>> Options;
    if (GetJsonArray(KwargsObject, TEXT("options"), Options))
    {
        for (const TSharedPtr<FJsonValue>& OptionJson : Options)
        {
            FString OptionText = OptionJson->AsString();
            ComboBox->AddOption(OptionText); 
        }
    }
    
    FString SelectedOptionString;
    if (KwargsObject->TryGetStringField(TEXT("selected_option"), SelectedOptionString) && !SelectedOptionString.IsEmpty())
    {
        ComboBox->SetSelectedOption(SelectedOptionString);
    }
    
    return ComboBox;
}
