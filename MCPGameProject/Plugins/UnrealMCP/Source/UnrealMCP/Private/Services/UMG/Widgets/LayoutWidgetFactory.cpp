// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/UMG/Widgets/LayoutWidgetFactory.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/GridPanel.h"
#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/UniformGridPanel.h"

FLayoutWidgetFactory::FLayoutWidgetFactory()
{
}

UWidget* FLayoutWidgetFactory::CreateVerticalBox(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    UVerticalBox* VerticalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *ComponentName);
    // No special properties for vertical box beyond children
    return VerticalBox;
}

UWidget* FLayoutWidgetFactory::CreateHorizontalBox(UWidgetBlueprint* WidgetBlueprint, 
                                                   const FString& ComponentName, 
                                                   const TSharedPtr<FJsonObject>& KwargsObject)
{
    UHorizontalBox* HorizontalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *ComponentName);
    // No special properties for horizontal box beyond children
    return HorizontalBox;
}

UWidget* FLayoutWidgetFactory::CreateOverlay(UWidgetBlueprint* WidgetBlueprint, 
                                            const FString& ComponentName, 
                                            const TSharedPtr<FJsonObject>& KwargsObject)
{
    UOverlay* Overlay = WidgetBlueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *ComponentName);
    // No special properties for overlay beyond children
    return Overlay;
}

UWidget* FLayoutWidgetFactory::CreateGridPanel(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FLayoutWidgetFactory::CreateCanvasPanel(UWidgetBlueprint* WidgetBlueprint, 
                                                 const FString& ComponentName, 
                                                 const TSharedPtr<FJsonObject>& KwargsObject)
{
    UCanvasPanel* CanvasPanel = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), *ComponentName);
    // No special properties for canvas panel beyond children
    return CanvasPanel;
}

UWidget* FLayoutWidgetFactory::CreateSizeBox(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FLayoutWidgetFactory::CreateScrollBox(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FLayoutWidgetFactory::CreateWrapBox(UWidgetBlueprint* WidgetBlueprint, 
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

UWidget* FLayoutWidgetFactory::CreateUniformGridPanel(UWidgetBlueprint* WidgetBlueprint, 
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

bool FLayoutWidgetFactory::GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, 
                                        const FString& FieldName, 
                                        TArray<TSharedPtr<FJsonValue>>& OutArray)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* ArrayPtr = nullptr;
    if (JsonObject->TryGetArrayField(FieldName, ArrayPtr) && ArrayPtr != nullptr)
    {
        OutArray = *ArrayPtr;
        return true;
    }
    
    return false;
}
