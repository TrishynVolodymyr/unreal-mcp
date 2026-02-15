#include "Services/UMG/WidgetValidationService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/GridPanel.h"
#include "Components/SizeBox.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/MultiLineEditableText.h"
#include "Components/RichTextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Throbber.h"
#include "Components/CircularThrobber.h"
#include "Components/SpinBox.h"
#include "Components/WrapBox.h"
#include "Components/ScaleBox.h"
#include "Components/SafeZone.h"
#include "Components/MenuAnchor.h"
#include "Components/ExpandableArea.h"
#include "Components/InputKeySelector.h"
#include "Components/InvalidationBox.h"
#include "Components/RetainerBox.h"
#include "Components/BackgroundBlur.h"
#include "Components/UniformGridPanel.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/NamedSlot.h"
#include "Components/RadialSlider.h"
#include "Components/NativeWidgetHost.h"
#include "Components/WindowTitleBarArea.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"

FWidgetValidationService::FWidgetValidationService()
{
    InitializeValidationData();
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetBlueprintCreation(const FString& Name, const FString& ParentClass, const FString& Path) const
{
    // Validate name
    FWidgetValidationResult NameResult = ValidateName(Name, TEXT("Widget Blueprint"));
    if (!NameResult.bIsValid)
    {
        return NameResult;
    }

    // Validate path
    FWidgetValidationResult PathResult = ValidatePath(Path);
    if (!PathResult.bIsValid)
    {
        return PathResult;
    }

    // Validate parent class
    if (!ParentClass.IsEmpty() && ParentClass != TEXT("UserWidget"))
    {
        // Check if parent class exists (simplified check)
        if (ParentClass.Contains(TEXT("/")))
        {
            // Full path provided, check if it's a valid path format
            if (!ParentClass.StartsWith(TEXT("/Game/")) && !ParentClass.StartsWith(TEXT("/Engine/")) && !ParentClass.StartsWith(TEXT("/Script/")))
            {
                return FWidgetValidationResult::Error(FString::Printf(TEXT("Invalid parent class path format: %s"), *ParentClass));
            }
        }
    }

    // Check if blueprint already exists
    FString FullPath = Path + TEXT("/") + Name;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        FWidgetValidationResult Result = FWidgetValidationResult::Success();
        Result.AddWarning(FString::Printf(TEXT("Widget Blueprint '%s' already exists at path '%s'"), *Name, *Path));
        return Result;
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetComponentCreation(const FString& BlueprintName, const FString& ComponentName, 
                                                                                 const FString& ComponentType, const FVector2D& Position, 
                                                                                 const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs) const
{
    // Validate blueprint name
    FWidgetValidationResult NameResult = ValidateName(BlueprintName, TEXT("Blueprint"));
    if (!NameResult.bIsValid)
    {
        return NameResult;
    }

    // Check if blueprint exists
    if (!DoesWidgetBlueprintExist(BlueprintName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Widget Blueprint '%s' does not exist"), *BlueprintName));
    }

    // Validate component name
    FWidgetValidationResult ComponentNameResult = ValidateName(ComponentName, TEXT("Component"));
    if (!ComponentNameResult.bIsValid)
    {
        return ComponentNameResult;
    }

    // Check if component already exists
    if (DoesWidgetComponentExist(BlueprintName, ComponentName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Component '%s' already exists in blueprint '%s'"), *ComponentName, *BlueprintName));
    }

    // Validate component type
    FWidgetValidationResult TypeResult = ValidateWidgetType(ComponentType);
    if (!TypeResult.bIsValid)
    {
        return TypeResult;
    }

    // Validate position
    FWidgetValidationResult PositionResult = ValidatePosition(Position);
    if (!PositionResult.bIsValid)
    {
        return PositionResult;
    }

    // Validate size
    FWidgetValidationResult SizeResult = ValidateSize(Size);
    if (!SizeResult.bIsValid)
    {
        return SizeResult;
    }

    // Validate kwargs if provided
    if (Kwargs.IsValid())
    {
        for (const auto& PropertyPair : Kwargs->Values)
        {
            const FString& PropertyName = PropertyPair.Key;
            const TSharedPtr<FJsonValue>& PropertyValue = PropertyPair.Value;

            FWidgetValidationResult PropertyResult = ValidateWidgetProperty(ComponentType, PropertyName, PropertyValue);
            if (!PropertyResult.bIsValid)
            {
                return PropertyResult;
            }
        }
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetPropertySetting(const FString& BlueprintName, const FString& ComponentName, 
                                                                               const TSharedPtr<FJsonObject>& Properties) const
{
    // Validate blueprint name
    if (!DoesWidgetBlueprintExist(BlueprintName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Widget Blueprint '%s' does not exist"), *BlueprintName));
    }

    // Validate component name
    if (!DoesWidgetComponentExist(BlueprintName, ComponentName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Component '%s' does not exist in blueprint '%s'"), *ComponentName, *BlueprintName));
    }

    // Validate properties
    if (!Properties.IsValid() || Properties->Values.Num() == 0)
    {
        return FWidgetValidationResult::Error(TEXT("No properties provided to set"));
    }

    // Get component type to validate properties
    // This would require loading the blueprint to get the actual component type
    // For now, we'll do basic validation
    FWidgetValidationResult Result = FWidgetValidationResult::Success();
    
    for (const auto& PropertyPair : Properties->Values)
    {
        const FString& PropertyName = PropertyPair.Key;
        const TSharedPtr<FJsonValue>& PropertyValue = PropertyPair.Value;

        if (PropertyName.IsEmpty())
        {
            return FWidgetValidationResult::Error(TEXT("Empty property name found"));
        }

        if (!PropertyValue.IsValid())
        {
            return FWidgetValidationResult::Error(FString::Printf(TEXT("Invalid value for property '%s'"), *PropertyName));
        }
    }

    return Result;
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetEventBinding(const FString& BlueprintName, const FString& ComponentName, 
                                                                            const FString& EventName, const FString& FunctionName) const
{
    // Validate blueprint name
    if (!DoesWidgetBlueprintExist(BlueprintName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Widget Blueprint '%s' does not exist"), *BlueprintName));
    }

    // Validate component name
    if (!DoesWidgetComponentExist(BlueprintName, ComponentName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Component '%s' does not exist in blueprint '%s'"), *ComponentName, *BlueprintName));
    }

    // Validate event name
    FWidgetValidationResult EventNameResult = ValidateName(EventName, TEXT("Event"));
    if (!EventNameResult.bIsValid)
    {
        return EventNameResult;
    }

    // Validate function name if provided
    if (!FunctionName.IsEmpty())
    {
        FWidgetValidationResult FunctionNameResult = ValidateName(FunctionName, TEXT("Function"));
        if (!FunctionNameResult.bIsValid)
        {
            return FunctionNameResult;
        }
    }

    // Check if event is valid for common widget types
    TArray<FString> CommonEvents = {
        TEXT("OnClicked"),
        TEXT("OnPressed"),
        TEXT("OnReleased"),
        TEXT("OnHovered"),
        TEXT("OnUnhovered"),
        TEXT("OnValueChanged"),
        TEXT("OnTextChanged"),
        TEXT("OnTextCommitted"),
        TEXT("OnSelectionChanged")
    };

    if (!CommonEvents.Contains(EventName))
    {
        FWidgetValidationResult Result = FWidgetValidationResult::Success();
        Result.AddWarning(FString::Printf(TEXT("Event '%s' may not be valid for all widget types"), *EventName));
        return Result;
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetHierarchy(const FString& BlueprintName, const FString& ParentComponentName, 
                                                                         const FString& ChildComponentName) const
{
    // Validate blueprint name
    if (!DoesWidgetBlueprintExist(BlueprintName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Widget Blueprint '%s' does not exist"), *BlueprintName));
    }

    // Validate parent component name
    if (!DoesWidgetComponentExist(BlueprintName, ParentComponentName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Parent component '%s' does not exist in blueprint '%s'"), *ParentComponentName, *BlueprintName));
    }

    // Validate child component name
    if (!DoesWidgetComponentExist(BlueprintName, ChildComponentName))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Child component '%s' does not exist in blueprint '%s'"), *ChildComponentName, *BlueprintName));
    }

    // Check for circular references
    if (ParentComponentName == ChildComponentName)
    {
        return FWidgetValidationResult::Error(TEXT("Cannot add a component as a child of itself"));
    }

    // Additional hierarchy validation could be added here
    // For example, checking if the parent can actually contain children

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetType(const FString& ComponentType) const
{
    if (ComponentType.IsEmpty())
    {
        return FWidgetValidationResult::Error(TEXT("Component type cannot be empty"));
    }

    if (!ValidWidgetTypes.Contains(ComponentType))
    {
        // Check if it's a User Widget Blueprint path (e.g., "WBP_HUD_ResourceBar" or "/Game/UI/HUD/WBP_HUD_ResourceBar")
        FString BlueprintPath = ComponentType;
        if (!BlueprintPath.StartsWith(TEXT("/")))
        {
            // Try to find it as an asset by name — search common UI paths
            TArray<FString> SearchPaths = {
                FString::Printf(TEXT("/Game/UI/HUD/%s.%s"), *ComponentType, *ComponentType),
                FString::Printf(TEXT("/Game/UI/%s.%s"), *ComponentType, *ComponentType),
                FString::Printf(TEXT("/Game/UI/Widgets/%s.%s"), *ComponentType, *ComponentType),
                FString::Printf(TEXT("/Game/%s.%s"), *ComponentType, *ComponentType)
            };
            
            bool bFound = false;
            for (const FString& Path : SearchPaths)
            {
                if (UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *Path))
                {
                    bFound = true;
                    break;
                }
            }
            
            if (!bFound)
            {
                return FWidgetValidationResult::Error(FString::Printf(TEXT("Invalid widget component type: %s"), *ComponentType));
            }
        }
        else
        {
            // Full path provided — verify it exists
            FString FullPath = BlueprintPath;
            if (!FullPath.Contains(TEXT(".")))
            {
                // Add asset name: "/Game/UI/HUD/WBP_Foo" -> "/Game/UI/HUD/WBP_Foo.WBP_Foo"
                FString AssetName = FPaths::GetCleanFilename(FullPath);
                FullPath = FString::Printf(TEXT("%s.%s"), *BlueprintPath, *AssetName);
            }
            if (!StaticLoadObject(UObject::StaticClass(), nullptr, *FullPath))
            {
                return FWidgetValidationResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *ComponentType));
            }
        }
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidateWidgetProperty(const FString& ComponentType, const FString& PropertyName, 
                                                                        const TSharedPtr<FJsonValue>& PropertyValue) const
{
    if (PropertyName.IsEmpty())
    {
        return FWidgetValidationResult::Error(TEXT("Property name cannot be empty"));
    }

    if (!PropertyValue.IsValid())
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Invalid value for property '%s'"), *PropertyName));
    }

    // Check if property is valid for the component type
    const TSet<FString>* ValidProperties = WidgetTypeProperties.Find(ComponentType);
    if (ValidProperties && !ValidProperties->Contains(PropertyName))
    {
        FWidgetValidationResult Result = FWidgetValidationResult::Success();
        Result.AddWarning(FString::Printf(TEXT("Property '%s' may not be valid for widget type '%s'"), *PropertyName, *ComponentType));
        return Result;
    }

    // Validate property value types
    if (PropertyName == TEXT("Text") || PropertyName == TEXT("Content"))
    {
        if (PropertyValue->Type != EJson::String)
        {
            return FWidgetValidationResult::Error(FString::Printf(TEXT("Property '%s' must be a string"), *PropertyName));
        }
    }
    else if (PropertyName.Contains(TEXT("Color")))
    {
        if (PropertyValue->Type != EJson::Array && PropertyValue->Type != EJson::Object)
        {
            return FWidgetValidationResult::Error(FString::Printf(TEXT("Property '%s' must be an array or object"), *PropertyName));
        }
    }
    else if (PropertyName.Contains(TEXT("Size")) || PropertyName.Contains(TEXT("Padding")))
    {
        if (PropertyValue->Type != EJson::Array && PropertyValue->Type != EJson::Number)
        {
            return FWidgetValidationResult::Error(FString::Printf(TEXT("Property '%s' must be a number or array"), *PropertyName));
        }
    }

    return FWidgetValidationResult::Success();
}

void FWidgetValidationService::InitializeValidationData()
{
    // Initialize valid widget types
    ValidWidgetTypes = {
        TEXT("TextBlock"),
        TEXT("Button"),
        TEXT("Image"),
        TEXT("CheckBox"),
        TEXT("Slider"),
        TEXT("ProgressBar"),
        TEXT("Border"),
        TEXT("ScrollBox"),
        TEXT("Spacer"),
        TEXT("CanvasPanel"),
        TEXT("VerticalBox"),
        TEXT("HorizontalBox"),
        TEXT("Overlay"),
        TEXT("GridPanel"),
        TEXT("SizeBox"),
        TEXT("EditableText"),
        TEXT("EditableTextBox"),
        TEXT("ComboBoxString"),
        TEXT("MultiLineEditableText"),
        TEXT("RichTextBlock"),
        TEXT("WidgetSwitcher"),
        TEXT("Throbber"),
        TEXT("CircularThrobber"),
        TEXT("SpinBox"),
        TEXT("WrapBox"),
        TEXT("ScaleBox"),
        TEXT("SafeZone"),
        TEXT("MenuAnchor"),
        TEXT("ExpandableArea"),
        TEXT("InputKeySelector"),
        TEXT("InvalidationBox"),
        TEXT("RetainerBox"),
        TEXT("BackgroundBlur"),
        TEXT("UniformGridPanel"),
        TEXT("ListView"),
        TEXT("TileView"),
        TEXT("TreeView"),
        TEXT("NamedSlot"),
        TEXT("RadialSlider"),
        TEXT("NativeWidgetHost"),
        TEXT("WindowTitleBarArea")
    };

    // Initialize common properties for widget types
    TSet<FString> CommonProperties = {
        TEXT("Visibility"),
        TEXT("IsEnabled"),
        TEXT("ToolTipText"),
        TEXT("Cursor"),
        TEXT("RenderOpacity"),
        TEXT("RenderTransform"),
        TEXT("RenderTransformPivot")
    };

    TSet<FString> TextProperties = {
        TEXT("Text"),
        TEXT("Font"),
        TEXT("ColorAndOpacity"),
        TEXT("ShadowColorAndOpacity"),
        TEXT("ShadowOffset"),
        TEXT("Justification"),
        TEXT("AutoWrapText"),
        TEXT("WrapTextAt"),
        TEXT("Margin"),
        TEXT("LineHeightPercentage")
    };

    TSet<FString> ButtonProperties = {
        TEXT("Style"),
        TEXT("ColorAndOpacity"),
        TEXT("BackgroundColor"),
        TEXT("ClickMethod"),
        TEXT("TouchMethod"),
        TEXT("PressMethod"),
        TEXT("IsFocusable")
    };

    TSet<FString> ImageProperties = {
        TEXT("Brush"),
        TEXT("ColorAndOpacity"),
        TEXT("OnMouseButtonDownEvent")
    };

    TSet<FString> BorderProperties = {
        TEXT("Background"),
        TEXT("BrushColor"),
        TEXT("DesiredSizeScale"),
        TEXT("ContentColorAndOpacity"),
        TEXT("Padding"),
        TEXT("HorizontalAlignment"),
        TEXT("VerticalAlignment"),
        TEXT("ShowEffectWhenDisabled")
    };

    // Combine common properties with specific ones
    auto CombineProperties = [&CommonProperties](const TSet<FString>& SpecificProperties) {
        TSet<FString> Combined = CommonProperties;
        Combined.Append(SpecificProperties);
        return Combined;
    };

    WidgetTypeProperties.Add(TEXT("TextBlock"), CombineProperties(TextProperties));
    WidgetTypeProperties.Add(TEXT("Button"), CombineProperties(ButtonProperties));
    WidgetTypeProperties.Add(TEXT("Image"), CombineProperties(ImageProperties));
    WidgetTypeProperties.Add(TEXT("Border"), CombineProperties(BorderProperties));

    // Initialize common events for widget types
    TSet<FString> ButtonEvents = {
        TEXT("OnClicked"),
        TEXT("OnPressed"),
        TEXT("OnReleased"),
        TEXT("OnHovered"),
        TEXT("OnUnhovered")
    };

    TSet<FString> SliderEvents = {
        TEXT("OnValueChanged"),
        TEXT("OnMouseCaptureBegin"),
        TEXT("OnMouseCaptureEnd")
    };

    TSet<FString> TextEvents = {
        TEXT("OnTextChanged"),
        TEXT("OnTextCommitted")
    };

    WidgetTypeEvents.Add(TEXT("Button"), ButtonEvents);
    WidgetTypeEvents.Add(TEXT("Slider"), SliderEvents);
    WidgetTypeEvents.Add(TEXT("EditableText"), TextEvents);
    WidgetTypeEvents.Add(TEXT("EditableTextBox"), TextEvents);
}

FWidgetValidationResult FWidgetValidationService::ValidateName(const FString& Name, const FString& NameType) const
{
    if (Name.IsEmpty())
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("%s name cannot be empty"), *NameType));
    }

    // Check for invalid characters
    FString InvalidChars = TEXT("!@#$%^&*()+={}[]|\\:;\"'<>?,./`~");
    for (int32 i = 0; i < InvalidChars.Len(); i++)
    {
        if (Name.Contains(FString::Chr(InvalidChars[i])))
        {
            return FWidgetValidationResult::Error(FString::Printf(TEXT("%s name contains invalid character: %c"), *NameType, InvalidChars[i]));
        }
    }

    // Check if name starts with a number
    if (Name.Len() > 0 && FChar::IsDigit(Name[0]))
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("%s name cannot start with a number"), *NameType));
    }

    // Check for spaces (warn but don't fail)
    if (Name.Contains(TEXT(" ")))
    {
        FWidgetValidationResult Result = FWidgetValidationResult::Success();
        Result.AddWarning(FString::Printf(TEXT("%s name contains spaces, which may cause issues"), *NameType));
        return Result;
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidatePath(const FString& Path) const
{
    if (Path.IsEmpty())
    {
        return FWidgetValidationResult::Error(TEXT("Path cannot be empty"));
    }

    if (!Path.StartsWith(TEXT("/Game/")))
    {
        return FWidgetValidationResult::Error(TEXT("Path must start with '/Game/'"));
    }

    // Check for invalid path characters
    FString InvalidPathChars = TEXT("!@#$%^&*()+={}[]|\\:;\"'<>?,`~");
    for (int32 i = 0; i < InvalidPathChars.Len(); i++)
    {
        if (Path.Contains(FString::Chr(InvalidPathChars[i])))
        {
            return FWidgetValidationResult::Error(FString::Printf(TEXT("Path contains invalid character: %c"), InvalidPathChars[i]));
        }
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidatePosition(const FVector2D& Position) const
{
    // Check for reasonable position values
    const float MaxPosition = 10000.0f;
    const float MinPosition = -10000.0f;

    if (Position.X < MinPosition || Position.X > MaxPosition)
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Position X value %f is out of reasonable range (%f to %f)"), Position.X, MinPosition, MaxPosition));
    }

    if (Position.Y < MinPosition || Position.Y > MaxPosition)
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Position Y value %f is out of reasonable range (%f to %f)"), Position.Y, MinPosition, MaxPosition));
    }

    return FWidgetValidationResult::Success();
}

FWidgetValidationResult FWidgetValidationService::ValidateSize(const FVector2D& Size) const
{
    // Check for reasonable size values
    const float MaxSize = 10000.0f;
    const float MinSize = 0.0f;

    if (Size.X < MinSize || Size.X > MaxSize)
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Size X value %f is out of reasonable range (%f to %f)"), Size.X, MinSize, MaxSize));
    }

    if (Size.Y < MinSize || Size.Y > MaxSize)
    {
        return FWidgetValidationResult::Error(FString::Printf(TEXT("Size Y value %f is out of reasonable range (%f to %f)"), Size.Y, MinSize, MaxSize));
    }

    // Warn about very small sizes
    if (Size.X < 1.0f || Size.Y < 1.0f)
    {
        FWidgetValidationResult Result = FWidgetValidationResult::Success();
        Result.AddWarning(TEXT("Widget size is very small and may not be visible"));
        return Result;
    }

    return FWidgetValidationResult::Success();
}

bool FWidgetValidationService::DoesWidgetBlueprintExist(const FString& BlueprintName) const
{
    // Use the centralized FindWidgetBlueprint utility
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindWidgetBlueprint(BlueprintName);
    UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
    return WidgetBlueprint != nullptr;
}

bool FWidgetValidationService::DoesWidgetComponentExist(const FString& BlueprintName, const FString& ComponentName) const
{
    // Use the centralized FindWidgetBlueprint utility
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindWidgetBlueprint(BlueprintName);
    UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
    
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetValidationService: Widget blueprint '%s' not found"), *BlueprintName);
        return false;
    }
    
    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetValidationService: Widget blueprint '%s' has no WidgetTree"), *BlueprintName);
        return false;
    }
    
    UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName));
    
    if (!Widget)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetValidationService: Component '%s' not found in blueprint '%s'"), 
               *ComponentName, *BlueprintName);
    }
    
    return Widget != nullptr;
}

UClass* FWidgetValidationService::GetWidgetClass(const FString& ComponentType) const
{
    // Map component type strings to their UClass
    static TMap<FString, UClass*> WidgetClassMap = {
        {TEXT("TextBlock"), UTextBlock::StaticClass()},
        {TEXT("Button"), UButton::StaticClass()},
        {TEXT("Image"), UImage::StaticClass()},
        {TEXT("CheckBox"), UCheckBox::StaticClass()},
        {TEXT("Slider"), USlider::StaticClass()},
        {TEXT("ProgressBar"), UProgressBar::StaticClass()},
        {TEXT("Border"), UBorder::StaticClass()},
        {TEXT("ScrollBox"), UScrollBox::StaticClass()},
        {TEXT("Spacer"), USpacer::StaticClass()},
        {TEXT("CanvasPanel"), UCanvasPanel::StaticClass()},
        {TEXT("VerticalBox"), UVerticalBox::StaticClass()},
        {TEXT("HorizontalBox"), UHorizontalBox::StaticClass()},
        {TEXT("Overlay"), UOverlay::StaticClass()},
        {TEXT("GridPanel"), UGridPanel::StaticClass()},
        {TEXT("SizeBox"), USizeBox::StaticClass()},
        {TEXT("EditableText"), UEditableText::StaticClass()},
        {TEXT("EditableTextBox"), UEditableTextBox::StaticClass()},
        {TEXT("ComboBoxString"), UComboBoxString::StaticClass()},
        {TEXT("MultiLineEditableText"), UMultiLineEditableText::StaticClass()},
        {TEXT("RichTextBlock"), URichTextBlock::StaticClass()},
        {TEXT("WidgetSwitcher"), UWidgetSwitcher::StaticClass()},
        {TEXT("Throbber"), UThrobber::StaticClass()},
        {TEXT("CircularThrobber"), UCircularThrobber::StaticClass()},
        {TEXT("SpinBox"), USpinBox::StaticClass()},
        {TEXT("WrapBox"), UWrapBox::StaticClass()},
        {TEXT("ScaleBox"), UScaleBox::StaticClass()},
        {TEXT("SafeZone"), USafeZone::StaticClass()},
        {TEXT("MenuAnchor"), UMenuAnchor::StaticClass()},
        {TEXT("ExpandableArea"), UExpandableArea::StaticClass()},
        {TEXT("InputKeySelector"), UInputKeySelector::StaticClass()},
        {TEXT("InvalidationBox"), UInvalidationBox::StaticClass()},
        {TEXT("RetainerBox"), URetainerBox::StaticClass()},
        {TEXT("BackgroundBlur"), UBackgroundBlur::StaticClass()},
        {TEXT("UniformGridPanel"), UUniformGridPanel::StaticClass()},
        {TEXT("ListView"), UListView::StaticClass()},
        {TEXT("TileView"), UTileView::StaticClass()},
        {TEXT("TreeView"), UTreeView::StaticClass()},
        {TEXT("NamedSlot"), UNamedSlot::StaticClass()},
        {TEXT("RadialSlider"), URadialSlider::StaticClass()},
        {TEXT("NativeWidgetHost"), UNativeWidgetHost::StaticClass()},
        {TEXT("WindowTitleBarArea"), UWindowTitleBarArea::StaticClass()}
    };

    UClass* const* FoundClass = WidgetClassMap.Find(ComponentType);
    if (FoundClass)
    {
        return *FoundClass;
    }
    
    // Try loading as a User Widget Blueprint
    TArray<FString> SearchPaths = {
        FString::Printf(TEXT("/Game/UI/HUD/%s.%s_C"), *ComponentType, *ComponentType),
        FString::Printf(TEXT("/Game/UI/%s.%s_C"), *ComponentType, *ComponentType),
        FString::Printf(TEXT("/Game/UI/Widgets/%s.%s_C"), *ComponentType, *ComponentType),
        FString::Printf(TEXT("/Game/%s.%s_C"), *ComponentType, *ComponentType)
    };
    
    // If it's already a full path
    if (ComponentType.StartsWith(TEXT("/")))
    {
        FString AssetName = FPaths::GetCleanFilename(ComponentType);
        SearchPaths.Insert(FString::Printf(TEXT("%s.%s_C"), *ComponentType, *AssetName), 0);
    }
    
    for (const FString& Path : SearchPaths)
    {
        UClass* BlueprintClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, *Path);
        if (BlueprintClass)
        {
            return BlueprintClass;
        }
    }
    
    return nullptr;
}

bool FWidgetValidationService::DoesPropertyExist(UClass* WidgetClass, const FString& PropertyName) const
{
    if (!WidgetClass)
    {
        return false;
    }

    FProperty* Property = WidgetClass->FindPropertyByName(FName(*PropertyName));
    return Property != nullptr;
}

bool FWidgetValidationService::DoesEventExist(UClass* WidgetClass, const FString& EventName) const
{
    if (!WidgetClass)
    {
        return false;
    }

    FProperty* Property = WidgetClass->FindPropertyByName(FName(*EventName));
    FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(Property);
    return DelegateProperty != nullptr;
}
