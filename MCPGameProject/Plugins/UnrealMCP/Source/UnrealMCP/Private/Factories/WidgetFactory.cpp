#include "Factories/WidgetFactory.h"

// Basic UMG Components
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Components/Spacer.h"

// Layout Components
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/GridPanel.h"
#include "Components/UniformGridPanel.h"
#include "Components/WrapBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"

// Input Components
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableText.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/InputKeySelector.h"

// Advanced Components
#include "Components/RichTextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Throbber.h"
#include "Components/CircularThrobber.h"
#include "Components/ExpandableArea.h"
#include "Components/MenuAnchor.h"
#include "Components/SafeZone.h"
#include "Components/InvalidationBox.h"
#include "Components/RetainerBox.h"
#include "Components/BackgroundBlur.h"
#include "Components/NamedSlot.h"
#include "Components/RadialSlider.h"
#include "Components/NativeWidgetHost.h"
#include "Components/WindowTitleBarArea.h"

// List Components
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"

// Blueprint/WidgetTree for widget creation
#include "Blueprint/WidgetTree.h"

FWidgetFactory& FWidgetFactory::Get()
{
    static FWidgetFactory Instance;
    return Instance;
}

void FWidgetFactory::RegisterWidgetType(const FString& TypeName, UClass* WidgetClass)
{
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("FWidgetFactory::RegisterWidgetType: Attempted to register null WidgetClass for type '%s'"), *TypeName);
        return;
    }

    if (!WidgetClass->IsChildOf(UWidget::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("FWidgetFactory::RegisterWidgetType: Class '%s' is not a valid UWidget"), *WidgetClass->GetName());
        return;
    }

    FScopeLock Lock(&WidgetMapLock);
    WidgetTypeMap.Add(TypeName, WidgetClass);
    
    UE_LOG(LogTemp, Log, TEXT("FWidgetFactory: Registered widget type '%s' -> '%s'"), *TypeName, *WidgetClass->GetName());
}

UClass* FWidgetFactory::GetWidgetClass(const FString& TypeName) const
{
    FScopeLock Lock(&WidgetMapLock);
    
    // Ensure default types are initialized
    if (!bDefaultTypesInitialized)
    {
        // Cast away const to initialize - this is safe as we're using a lock
        const_cast<FWidgetFactory*>(this)->InitializeDefaultWidgetTypes();
    }

    if (UClass* const* FoundClass = WidgetTypeMap.Find(TypeName))
    {
        return *FoundClass;
    }

    UE_LOG(LogTemp, Warning, TEXT("FWidgetFactory::GetWidgetClass: Widget type '%s' not found"), *TypeName);
    return nullptr;
}

TArray<FString> FWidgetFactory::GetAvailableWidgetTypes() const
{
    FScopeLock Lock(&WidgetMapLock);
    
    // Ensure default types are initialized
    if (!bDefaultTypesInitialized)
    {
        // Cast away const to initialize - this is safe as we're using a lock
        const_cast<FWidgetFactory*>(this)->InitializeDefaultWidgetTypes();
    }

    TArray<FString> AvailableTypes;
    WidgetTypeMap.GetKeys(AvailableTypes);
    
    // Sort alphabetically for consistent output
    AvailableTypes.Sort();
    
    return AvailableTypes;
}

UWidget* FWidgetFactory::CreateWidget(const FString& TypeName, const FString& WidgetName)
{
    UClass* WidgetClass = GetWidgetClass(TypeName);
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FWidgetFactory::CreateWidget: Cannot create widget of unknown type '%s'"), *TypeName);
        return nullptr;
    }

    // Note: Widget creation typically requires a WidgetTree context
    // This method provides the class lookup, but actual widget instantiation
    // should be done through UWidgetTree::ConstructWidget in the appropriate context
    UE_LOG(LogTemp, Log, TEXT("FWidgetFactory::CreateWidget: Found widget class '%s' for type '%s'"), 
           *WidgetClass->GetName(), *TypeName);
    
    return nullptr; // Return nullptr as widgets need WidgetTree context for proper creation
}

void FWidgetFactory::InitializeDefaultWidgetTypes()
{
    FScopeLock Lock(&WidgetMapLock);
    
    if (bDefaultTypesInitialized)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("FWidgetFactory: Initializing default widget types"));

    // Basic Display Components
    RegisterWidgetType(TEXT("TextBlock"), UTextBlock::StaticClass());
    RegisterWidgetType(TEXT("RichTextBlock"), URichTextBlock::StaticClass());
    RegisterWidgetType(TEXT("Image"), UImage::StaticClass());
    RegisterWidgetType(TEXT("Spacer"), USpacer::StaticClass());

    // Input Components
    RegisterWidgetType(TEXT("Button"), UButton::StaticClass());
    RegisterWidgetType(TEXT("CheckBox"), UCheckBox::StaticClass());
    RegisterWidgetType(TEXT("Slider"), USlider::StaticClass());
    RegisterWidgetType(TEXT("RadialSlider"), URadialSlider::StaticClass());
    RegisterWidgetType(TEXT("SpinBox"), USpinBox::StaticClass());
    RegisterWidgetType(TEXT("ComboBoxString"), UComboBoxString::StaticClass());
    RegisterWidgetType(TEXT("ComboBox"), UComboBoxString::StaticClass());  // Alias for convenience
    RegisterWidgetType(TEXT("EditableText"), UEditableText::StaticClass());
    RegisterWidgetType(TEXT("EditableTextBox"), UEditableTextBox::StaticClass());
    RegisterWidgetType(TEXT("MultiLineEditableText"), UMultiLineEditableText::StaticClass());
    RegisterWidgetType(TEXT("InputKeySelector"), UInputKeySelector::StaticClass());

    // Layout Panels
    RegisterWidgetType(TEXT("CanvasPanel"), UCanvasPanel::StaticClass());
    RegisterWidgetType(TEXT("VerticalBox"), UVerticalBox::StaticClass());
    RegisterWidgetType(TEXT("HorizontalBox"), UHorizontalBox::StaticClass());
    RegisterWidgetType(TEXT("Overlay"), UOverlay::StaticClass());
    RegisterWidgetType(TEXT("GridPanel"), UGridPanel::StaticClass());
    RegisterWidgetType(TEXT("UniformGridPanel"), UUniformGridPanel::StaticClass());
    RegisterWidgetType(TEXT("WrapBox"), UWrapBox::StaticClass());
    RegisterWidgetType(TEXT("ScrollBox"), UScrollBox::StaticClass());

    // Container Components
    RegisterWidgetType(TEXT("Border"), UBorder::StaticClass());
    RegisterWidgetType(TEXT("SizeBox"), USizeBox::StaticClass());
    RegisterWidgetType(TEXT("ScaleBox"), UScaleBox::StaticClass());
    RegisterWidgetType(TEXT("SafeZone"), USafeZone::StaticClass());
    RegisterWidgetType(TEXT("InvalidationBox"), UInvalidationBox::StaticClass());
    RegisterWidgetType(TEXT("RetainerBox"), URetainerBox::StaticClass());

    // Progress and Status Components
    RegisterWidgetType(TEXT("ProgressBar"), UProgressBar::StaticClass());
    RegisterWidgetType(TEXT("Throbber"), UThrobber::StaticClass());
    RegisterWidgetType(TEXT("CircularThrobber"), UCircularThrobber::StaticClass());

    // Advanced Components
    RegisterWidgetType(TEXT("WidgetSwitcher"), UWidgetSwitcher::StaticClass());
    RegisterWidgetType(TEXT("ExpandableArea"), UExpandableArea::StaticClass());
    RegisterWidgetType(TEXT("MenuAnchor"), UMenuAnchor::StaticClass());
    RegisterWidgetType(TEXT("BackgroundBlur"), UBackgroundBlur::StaticClass());
    RegisterWidgetType(TEXT("NamedSlot"), UNamedSlot::StaticClass());
    RegisterWidgetType(TEXT("NativeWidgetHost"), UNativeWidgetHost::StaticClass());
    RegisterWidgetType(TEXT("WindowTitleBarArea"), UWindowTitleBarArea::StaticClass());

    // List Components
    RegisterWidgetType(TEXT("ListView"), UListView::StaticClass());
    RegisterWidgetType(TEXT("TileView"), UTileView::StaticClass());
    RegisterWidgetType(TEXT("TreeView"), UTreeView::StaticClass());

    bDefaultTypesInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("FWidgetFactory: Initialized %d default widget types"), WidgetTypeMap.Num());
}