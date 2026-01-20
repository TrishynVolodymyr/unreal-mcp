#include "Services/UMG/UMGService.h"
#include "Services/UMG/WidgetComponentService.h"
#include "Services/UMG/WidgetValidationService.h"
#include "Services/UMG/WidgetLayoutService.h"
#include "Services/UMG/WidgetInputHandlerService.h"
#include "Services/UMG/WidgetBindingService.h"
#include "Services/PropertyService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ContentWidget.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_VariableGet.h"
#include "K2Node_CallFunction.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "K2Node_ComponentBoundEvent.h"
#include "UObject/TextProperty.h"
#include "UObject/EnumProperty.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

FUMGService& FUMGService::Get()
{
    static FUMGService Instance;
    return Instance;
}

FUMGService::FUMGService()
{
    WidgetComponentService = MakeUnique<FWidgetComponentService>();
    ValidationService = MakeUnique<FWidgetValidationService>();
}

FUMGService::~FUMGService()
{
    // Destructor implementation - unique_ptr will handle cleanup automatically
}

UWidgetBlueprint* FUMGService::CreateWidgetBlueprint(const FString& Name, const FString& ParentClass, const FString& Path)
{
    // Validate parameters
    if (ValidationService)
    {
        FWidgetValidationResult ValidationResult = ValidationService->ValidateWidgetBlueprintCreation(Name, ParentClass, Path);
        if (!ValidationResult.bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint creation validation failed: %s"), *ValidationResult.ErrorMessage);
            return nullptr;
        }
        
        // Log warnings if any
        for (const FString& Warning : ValidationResult.Warnings)
        {
            UE_LOG(LogTemp, Warning, TEXT("UMGService: %s"), *Warning);
        }
    }

    // Check if blueprint already exists and is functional
    if (DoesWidgetBlueprintExist(Name, Path))
    {
        FString FullPath = Path + TEXT("/") + Name;
        UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(FullPath);
        UWidgetBlueprint* ExistingWidgetBP = Cast<UWidgetBlueprint>(ExistingAsset);
        if (ExistingWidgetBP)
        {
            UE_LOG(LogTemp, Display, TEXT("UMGService: Using existing functional widget blueprint: %s"), *FullPath);
            return ExistingWidgetBP;
        }
    }
    
    // If asset exists but is not functional, delete it first
    FString FullPath = Path + TEXT("/") + Name;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Deleting non-functional widget blueprint: %s"), *FullPath);
        UEditorAssetLibrary::DeleteAsset(FullPath);
    }

    // Find parent class
    UClass* ParentClassPtr = FindParentClass(ParentClass);
    if (!ParentClassPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Could not find parent class: %s, using default UserWidget"), *ParentClass);
        ParentClassPtr = UUserWidget::StaticClass();
    }

    return CreateWidgetBlueprintInternal(Name, ParentClassPtr, Path);
}

bool FUMGService::DoesWidgetBlueprintExist(const FString& Name, const FString& Path)
{
    FString FullPath = Path + TEXT("/") + Name;
    
    // First check if asset exists in the asset system
    if (!UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        return false;
    }
    
    // Then check if it's actually a functional widget blueprint
    UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(FullPath);
    UWidgetBlueprint* ExistingWidgetBP = Cast<UWidgetBlueprint>(ExistingAsset);
    
    if (!ExistingWidgetBP)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Asset exists but is not a UWidgetBlueprint: %s"), *FullPath);
        return false;
    }
    
    // Check if it has a proper WidgetTree
    if (!ExistingWidgetBP->WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Widget Blueprint exists but has no WidgetTree: %s"), *FullPath);
        return false;
    }
    
    return true;
}

UWidget* FUMGService::AddWidgetComponent(const FString& BlueprintName, const FString& ComponentName,
                                        const FString& ComponentType, const FVector2D& Position,
                                        const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs,
                                        FString* OutError)
{
    // Validate parameters
    if (ValidationService)
    {
        FWidgetValidationResult ValidationResult = ValidationService->ValidateWidgetComponentCreation(BlueprintName, ComponentName, ComponentType, Position, Size, Kwargs);
        if (!ValidationResult.bIsValid)
        {
            FString ErrorMsg = FString::Printf(TEXT("Widget component creation validation failed: %s"), *ValidationResult.ErrorMessage);
            UE_LOG(LogTemp, Error, TEXT("UMGService: %s"), *ErrorMsg);
            if (OutError) *OutError = ErrorMsg;
            return nullptr;
        }

        // Log warnings if any
        for (const FString& Warning : ValidationResult.Warnings)
        {
            UE_LOG(LogTemp, Warning, TEXT("UMGService: %s"), *Warning);
        }
    }

    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        FString ErrorMsg = FString::Printf(TEXT("Failed to find widget blueprint: %s"), *BlueprintName);
        UE_LOG(LogTemp, Error, TEXT("UMGService: %s"), *ErrorMsg);
        if (OutError) *OutError = ErrorMsg;
        return nullptr;
    }

    if (!WidgetComponentService)
    {
        FString ErrorMsg = TEXT("Internal error: WidgetComponentService is null");
        UE_LOG(LogTemp, Error, TEXT("UMGService: %s"), *ErrorMsg);
        if (OutError) *OutError = ErrorMsg;
        return nullptr;
    }

    FString ServiceError;
    UWidget* Result = WidgetComponentService->CreateWidgetComponent(WidgetBlueprint, ComponentName, ComponentType, Position, Size, Kwargs, ServiceError);
    if (!Result && OutError)
    {
        *OutError = ServiceError.IsEmpty() ? FString::Printf(TEXT("Failed to create widget component: %s of type %s"), *ComponentName, *ComponentType) : ServiceError;
    }
    return Result;
}

bool FUMGService::SetWidgetProperties(const FString& BlueprintName, const FString& ComponentName, 
                                     const TSharedPtr<FJsonObject>& Properties, TArray<FString>& OutSuccessProperties, 
                                     TArray<FString>& OutFailedProperties)
{
    // Validate parameters
    if (ValidationService)
    {
        FWidgetValidationResult ValidationResult = ValidationService->ValidateWidgetPropertySetting(BlueprintName, ComponentName, Properties);
        if (!ValidationResult.bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Widget property setting validation failed: %s"), *ValidationResult.ErrorMessage);
            return false;
        }
        
        // Log warnings if any
        for (const FString& Warning : ValidationResult.Warnings)
        {
            UE_LOG(LogTemp, Warning, TEXT("UMGService: %s"), *Warning);
        }
    }

    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return false;
    }

    UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName));
    if (!Widget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget component: %s"), *ComponentName);
        return false;
    }

    OutSuccessProperties.Empty();
    OutFailedProperties.Empty();

    // Handle special widget-specific properties first
    TSharedPtr<FJsonObject> RemainingProperties = MakeShareable(new FJsonObject);
    for (const auto& PropPair : Properties->Values)
    {
        RemainingProperties->SetField(PropPair.Key, PropPair.Value);
    }

    // Special handling for TextBlock font properties
    if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        // Handle font_size property (convenience for setting just the font size)
        if (Properties->HasField(TEXT("font_size")))
        {
            double FontSize = 0.0;
            if (Properties->TryGetNumberField(TEXT("font_size"), FontSize))
            {
                TextBlock->Modify();
                FSlateFontInfo CurrentFont = TextBlock->GetFont();
                CurrentFont.Size = static_cast<float>(FontSize);
                TextBlock->SetFont(CurrentFont);
                TextBlock->SynchronizeProperties();
                WidgetBlueprint->MarkPackageDirty();
                OutSuccessProperties.Add(TEXT("font_size"));
                RemainingProperties->RemoveField(TEXT("font_size"));
                UE_LOG(LogTemp, Log, TEXT("UMGService: Set font_size to %f on TextBlock '%s'"), FontSize, *ComponentName);
            }
            else
            {
                OutFailedProperties.Add(TEXT("font_size"));
                RemainingProperties->RemoveField(TEXT("font_size"));
                UE_LOG(LogTemp, Warning, TEXT("UMGService: Invalid font_size value for TextBlock '%s'"), *ComponentName);
            }
        }

        // Handle Font property as JSON object with FontObject, Size, LetterSpacing, etc.
        if (Properties->HasField(TEXT("Font")))
        {
            UE_LOG(LogTemp, Warning, TEXT("UMGService: *** FONT PROPERTY DETECTED on TextBlock '%s' ***"), *ComponentName);

            const TSharedPtr<FJsonObject>* FontJsonObject;
            if (Properties->TryGetObjectField(TEXT("Font"), FontJsonObject) && FontJsonObject->IsValid())
            {
                FSlateFontInfo CurrentFont = TextBlock->GetFont();
                bool bFontModified = false;

                // Log all keys in the Font object for debugging
                TArray<FString> FontKeys;
                (*FontJsonObject)->Values.GetKeys(FontKeys);
                UE_LOG(LogTemp, Warning, TEXT("UMGService: Font object has %d keys: %s"), FontKeys.Num(), *FString::Join(FontKeys, TEXT(", ")));

                // Handle FontObject - load and set the font asset (UFont or UFontFace)
                FString FontObjectPath;
                if ((*FontJsonObject)->TryGetStringField(TEXT("FontObject"), FontObjectPath))
                {
                    UE_LOG(LogTemp, Warning, TEXT("UMGService: FontObject path from JSON: '%s'"), *FontObjectPath);
                    // Normalize the path - ensure it has the correct format
                    if (!FontObjectPath.IsEmpty())
                    {
                        // Add .FontObject suffix if not already a full reference
                        FString FullFontPath = FontObjectPath;
                        if (!FullFontPath.Contains(TEXT(".")))
                        {
                            // Assume it's a simple path like "/Game/Fonts/MyFont", add asset name
                            FString AssetName = FPaths::GetBaseFilename(FontObjectPath);
                            FullFontPath = FontObjectPath + TEXT(".") + AssetName;
                        }

                        UObject* FontAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *FullFontPath);
                        if (FontAsset)
                        {
                            // Check if it's a UFont (composite font)
                            if (UFont* CompositeFont = Cast<UFont>(FontAsset))
                            {
                                CurrentFont.FontObject = CompositeFont;
                                CurrentFont.TypefaceFontName = NAME_None; // Use default typeface
                                bFontModified = true;
                                UE_LOG(LogTemp, Log, TEXT("UMGService: Set FontObject (UFont) to '%s' on TextBlock '%s'"), *FullFontPath, *ComponentName);
                            }
                            // Check if it's a UFontFace (font face asset)
                            else if (UFontFace* FontFace = Cast<UFontFace>(FontAsset))
                            {
                                CurrentFont.FontObject = FontFace;
                                CurrentFont.TypefaceFontName = NAME_None;
                                bFontModified = true;
                                UE_LOG(LogTemp, Log, TEXT("UMGService: Set FontObject (UFontFace) to '%s' on TextBlock '%s'"), *FullFontPath, *ComponentName);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("UMGService: Loaded asset '%s' is not a UFont or UFontFace (actual type: %s)"),
                                       *FullFontPath, *FontAsset->GetClass()->GetName());
                                OutFailedProperties.Add(FString::Printf(TEXT("FontObject_WrongType_%s"), *FontAsset->GetClass()->GetName()));
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("UMGService: Failed to load font asset: '%s'"), *FullFontPath);
                            OutFailedProperties.Add(TEXT("FontObject_NotFound"));
                        }
                    }
                }

                // Handle TypefaceFontName - the typeface name within a composite font
                FString TypefaceName;
                if ((*FontJsonObject)->TryGetStringField(TEXT("TypefaceFontName"), TypefaceName))
                {
                    CurrentFont.TypefaceFontName = FName(*TypefaceName);
                    bFontModified = true;
                    UE_LOG(LogTemp, Log, TEXT("UMGService: Set TypefaceFontName to '%s' on TextBlock '%s'"), *TypefaceName, *ComponentName);
                }

                // Handle Size within Font object
                double Size = 0.0;
                if ((*FontJsonObject)->TryGetNumberField(TEXT("Size"), Size))
                {
                    CurrentFont.Size = static_cast<float>(Size);
                    bFontModified = true;
                }

                // Handle LetterSpacing within Font object
                int32 LetterSpacing = 0;
                if ((*FontJsonObject)->TryGetNumberField(TEXT("LetterSpacing"), LetterSpacing))
                {
                    CurrentFont.LetterSpacing = LetterSpacing;
                    bFontModified = true;
                }

                // Handle SkewAmount within Font object
                double SkewAmount = 0.0;
                if ((*FontJsonObject)->TryGetNumberField(TEXT("SkewAmount"), SkewAmount))
                {
                    CurrentFont.SkewAmount = static_cast<float>(SkewAmount);
                    bFontModified = true;
                }

                if (bFontModified)
                {
                    TextBlock->Modify();
                    TextBlock->SetFont(CurrentFont);
                    TextBlock->SynchronizeProperties();
                    WidgetBlueprint->MarkPackageDirty();
                    OutSuccessProperties.Add(TEXT("Font"));
                    UE_LOG(LogTemp, Log, TEXT("UMGService: Set Font properties on TextBlock '%s'"), *ComponentName);
                }
                else
                {
                    OutFailedProperties.Add(TEXT("Font"));
                    UE_LOG(LogTemp, Warning, TEXT("UMGService: No valid Font sub-properties found for TextBlock '%s'"), *ComponentName);
                }
                RemainingProperties->RemoveField(TEXT("Font"));
            }
        }
    }

    // Handle slot properties (Slot.* prefix)
    TArray<FString> SlotPropertiesToRemove;
    for (const auto& PropPair : RemainingProperties->Values)
    {
        FString PropName = PropPair.Key;
        if (PropName.StartsWith(TEXT("Slot."), ESearchCase::IgnoreCase))
        {
            // Extract the property name after "Slot."
            FString SlotPropName = PropName.RightChop(5); // Remove "Slot."
            FString SlotError;

            if (SetSlotProperty(Widget, SlotPropName, PropPair.Value, SlotError))
            {
                OutSuccessProperties.Add(PropName);
                UE_LOG(LogTemp, Log, TEXT("UMGService: Successfully set slot property '%s' on '%s'"), *PropName, *ComponentName);
            }
            else
            {
                OutFailedProperties.Add(PropName);
                UE_LOG(LogTemp, Warning, TEXT("UMGService: Failed to set slot property '%s' on '%s': %s"), *PropName, *ComponentName, *SlotError);
            }
            SlotPropertiesToRemove.Add(PropName);
        }
    }

    // Remove processed slot properties from remaining
    for (const FString& PropToRemove : SlotPropertiesToRemove)
    {
        RemainingProperties->RemoveField(PropToRemove);
    }

    // Use PropertyService for remaining universal property setting
    TArray<FString> SuccessProps;
    TMap<FString, FString> FailedProps;

    FPropertyService::Get().SetObjectProperties(Widget, RemainingProperties, SuccessProps, FailedProps);

    // Append PropertyService results to output arrays (don't overwrite special-handled properties)
    OutSuccessProperties.Append(SuccessProps);
    for (const auto& FailedProp : FailedProps)
    {
        OutFailedProperties.Add(FailedProp.Key);
        // Log detailed error message
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Failed to set property '%s': %s"), 
               *FailedProp.Key, *FailedProp.Value);
    }

    // Save the blueprint if any properties were set
    if (OutSuccessProperties.Num() > 0)
    {
        WidgetBlueprint->MarkPackageDirty();
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);
    }

    return OutSuccessProperties.Num() > 0;
}

bool FUMGService::BindWidgetEvent(const FString& BlueprintName, const FString& ComponentName, 
                                 const FString& EventName, const FString& FunctionName, 
                                 FString& OutActualFunctionName)
{
    // Validate parameters
    if (ValidationService)
    {
        FWidgetValidationResult ValidationResult = ValidationService->ValidateWidgetEventBinding(BlueprintName, ComponentName, EventName, FunctionName);
        if (!ValidationResult.bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Widget event binding validation failed: %s"), *ValidationResult.ErrorMessage);
            return false;
        }
        
        // Log warnings if any
        for (const FString& Warning : ValidationResult.Warnings)
        {
            UE_LOG(LogTemp, Warning, TEXT("UMGService: %s"), *Warning);
        }
    }

    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return false;
    }

    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint has no WidgetTree: %s"), *BlueprintName);
        return false;
    }

    UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName));
    if (!Widget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget component: %s"), *ComponentName);
        return false;
    }

    // Ensure the widget is exposed as a variable - required for event binding
    if (!Widget->bIsVariable)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Widget '%s' is not exposed as variable. Exposing it now."), *ComponentName);
        Widget->bIsVariable = true;
        WidgetBlueprint->MarkPackageDirty();
    }

    OutActualFunctionName = FunctionName.IsEmpty() ? (ComponentName + TEXT("_") + EventName) : FunctionName;

    return CreateEventBinding(WidgetBlueprint, Widget, ComponentName, EventName, OutActualFunctionName);
}

bool FUMGService::SetTextBlockBinding(const FString& BlueprintName, const FString& TextBlockName, 
                                     const FString& BindingName, const FString& VariableType)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return false;
    }

    UTextBlock* TextBlock = Cast<UTextBlock>(WidgetBlueprint->WidgetTree->FindWidget(FName(*TextBlockName)));
    if (!TextBlock)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find text block widget: %s"), *TextBlockName);
        return false;
    }

    // Create variable if it doesn't exist
    bool bVariableExists = false;
    for (const FBPVariableDescription& Variable : WidgetBlueprint->NewVariables)
    {
        if (Variable.VarName == FName(*BindingName))
        {
            bVariableExists = true;
            break;
        }
    }

    if (!bVariableExists)
    {
        // Determine pin type based on variable type
        FEdGraphPinType PinType;
        if (VariableType == TEXT("Text"))
        {
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Text, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
        }
        else if (VariableType == TEXT("String"))
        {
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_String, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
        }
        else if (VariableType == TEXT("Int") || VariableType == TEXT("Integer"))
        {
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Int, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
        }
        else if (VariableType == TEXT("Float"))
        {
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
        }
        else if (VariableType == TEXT("Boolean") || VariableType == TEXT("Bool"))
        {
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Boolean, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
        }
        else
        {
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Text, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
        }

        FBlueprintEditorUtils::AddMemberVariable(WidgetBlueprint, FName(*BindingName), PinType);
    }

    return CreateTextBlockBindingFunction(WidgetBlueprint, TextBlockName, BindingName, VariableType);
}

bool FUMGService::DoesWidgetComponentExist(const FString& BlueprintName, const FString& ComponentName)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        return false;
    }

    if (!WidgetBlueprint->WidgetTree)
    {
        return false;
    }

    // Special case: For common root canvas names, check root widget first
    // This ensures predictable behavior when users expect to find the root canvas
    bool bIsCommonRootCanvasName = (
        ComponentName.Equals(TEXT("CanvasPanel_0"), ESearchCase::IgnoreCase) ||
        ComponentName.Equals(TEXT("RootCanvas"), ESearchCase::IgnoreCase) ||
        ComponentName.Equals(TEXT("Root Canvas"), ESearchCase::IgnoreCase) ||
        ComponentName.Equals(TEXT("Canvas Panel"), ESearchCase::IgnoreCase)
    );

    if (bIsCommonRootCanvasName)
    {
        // Check if the root widget is a canvas panel
        if (WidgetBlueprint->WidgetTree->RootWidget && 
            WidgetBlueprint->WidgetTree->RootWidget->IsA<UCanvasPanel>())
        {
            UE_LOG(LogTemp, Display, TEXT("UMGService: Found root canvas panel for common root name: %s"), *ComponentName);
            return true;
        }
    }

    // Try to find the widget by exact name (this handles both named widgets and the root "CanvasPanel")
    UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName));
    if (Widget)
    {
        return true;
    }

    // Final fallback: If searching for "CanvasPanel" and no exact match found, check root widget
    if (ComponentName.Equals(TEXT("CanvasPanel"), ESearchCase::IgnoreCase))
    {
        if (WidgetBlueprint->WidgetTree->RootWidget && 
            WidgetBlueprint->WidgetTree->RootWidget->IsA<UCanvasPanel>())
        {
            UE_LOG(LogTemp, Display, TEXT("UMGService: Found root canvas panel as fallback for: %s"), *ComponentName);
            return true;
        }
    }

    return false;
}

bool FUMGService::SetWidgetPlacement(const FString& BlueprintName, const FString& ComponentName, 
                                    const FVector2D* Position, const FVector2D* Size, const FVector2D* Alignment)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return false;
    }

    UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName));
    if (!Widget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget component: %s"), *ComponentName);
        return false;
    }

    bool bResult = SetCanvasSlotPlacement(Widget, Position, Size, Alignment);
    
    if (bResult)
    {
        WidgetBlueprint->MarkPackageDirty();
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);
    }

    return bResult;
}

bool FUMGService::GetWidgetContainerDimensions(const FString& BlueprintName, const FString& ContainerName, FVector2D& OutDimensions)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return false;
    }

    FString ActualContainerName = ContainerName.IsEmpty() ? TEXT("CanvasPanel_0") : ContainerName;
    UWidget* Container = WidgetBlueprint->WidgetTree->FindWidget(FName(*ActualContainerName));
    
    if (!Container)
    {
        // Try root widget if specific container not found
        Container = WidgetBlueprint->WidgetTree->RootWidget;
    }

    if (!Container)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find container widget: %s"), *ActualContainerName);
        return false;
    }

    // For canvas panels, we can get dimensions from the slot
    if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(Container))
    {
        // Default canvas dimensions - this would need to be enhanced based on actual requirements
        OutDimensions = FVector2D(1920.0f, 1080.0f);
        return true;
    }

    // For other widget types, return default dimensions
    OutDimensions = FVector2D(800.0f, 600.0f);
    return true;
}

bool FUMGService::AddChildWidgetComponentToParent(const FString& BlueprintName, const FString& ParentComponentName,
                                                const FString& ChildComponentName, bool bCreateParentIfMissing,
                                                const FString& ParentComponentType, const FVector2D& ParentPosition,
                                                const FVector2D& ParentSize)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return false;
    }

    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint has no WidgetTree: %s"), *BlueprintName);
        return false;
    }

    // Find the child component
    UWidget* ChildWidget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ChildComponentName));
    if (!ChildWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find child widget component: %s"), *ChildComponentName);
        return false;
    }

    // Find or create the parent component
    UWidget* ParentWidget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ParentComponentName));
    if (!ParentWidget && bCreateParentIfMissing)
    {
        // Create the parent component
        TSharedPtr<FJsonObject> EmptyKwargs = MakeShared<FJsonObject>();
        FString CreateError;
        ParentWidget = WidgetComponentService->CreateWidgetComponent(WidgetBlueprint, ParentComponentName, ParentComponentType, ParentPosition, ParentSize, EmptyKwargs, CreateError);
        if (!ParentWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create parent widget component: %s - %s"), *ParentComponentName, *CreateError);
            return false;
        }
    }
    else if (!ParentWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find parent widget component: %s"), *ParentComponentName);
        return false;
    }

    // Add child to parent
    if (!AddWidgetToParent(ChildWidget, ParentWidget))
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to add child widget to parent"));
        return false;
    }

    // Save the blueprint
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    return true;
}

UWidgetBlueprint* FUMGService::FindWidgetBlueprint(const FString& BlueprintNameOrPath) const
{
    // Check if we already have a full path
    if (BlueprintNameOrPath.StartsWith(TEXT("/Game/")))
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(BlueprintNameOrPath);
        return Cast<UWidgetBlueprint>(Asset);
    }

    // Try common directories
    TArray<FString> SearchPaths = {
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Widgets/%s"), *BlueprintNameOrPath)),
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("UI/%s"), *BlueprintNameOrPath)),
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("UMG/%s"), *BlueprintNameOrPath)),
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Interface/%s"), *BlueprintNameOrPath))
    };

    for (const FString& SearchPath : SearchPaths)
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(SearchPath);
        UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Asset);
        if (WidgetBlueprint)
        {
            return WidgetBlueprint;
        }
    }

    // Use asset registry to search everywhere
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetData;

    FARFilter Filter;
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game")));
    Filter.bRecursivePaths = true;
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);

    for (const FAssetData& Asset : AssetData)
    {
        FString AssetName = Asset.AssetName.ToString();
        if (AssetName.Equals(BlueprintNameOrPath, ESearchCase::IgnoreCase))
        {
            FString AssetPath = Asset.GetSoftObjectPath().ToString();
            UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
            return Cast<UWidgetBlueprint>(LoadedAsset);
        }
    }

    return nullptr;
}

UWidgetBlueprint* FUMGService::CreateWidgetBlueprintInternal(const FString& Name, UClass* ParentClass, const FString& Path) const
{
    // Ensure ParentClass is not null
    if (!ParentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: ParentClass is null, using default UserWidget"));
        ParentClass = UUserWidget::StaticClass();
    }
    
    FString FullPath = Path + TEXT("/") + Name;
    
    // Create package for the new asset
    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create package for path: %s"), *FullPath);
        return nullptr;
    }

    // Create Blueprint using KismetEditorUtilities
    UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        FName(*Name),
        BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(),
        UWidgetBlueprintGeneratedClass::StaticClass()
    );

    UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewBlueprint);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Created blueprint is not a UWidgetBlueprint"));
        UEditorAssetLibrary::DeleteAsset(FullPath);
        return nullptr;
    }

    // Ensure the WidgetTree exists and add default Canvas Panel
    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Widget Blueprint has no WidgetTree, creating one"));
        WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint);
        if (!WidgetBlueprint->WidgetTree)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create WidgetTree"));
            UEditorAssetLibrary::DeleteAsset(FullPath);
            return nullptr;
        }
    }

    if (!WidgetBlueprint->WidgetTree->RootWidget)
    {
        UE_LOG(LogTemp, Display, TEXT("UMGService: Creating root canvas panel for widget: %s"), *Name);
        UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel"));
        if (RootCanvas)
        {
            WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
            UE_LOG(LogTemp, Display, TEXT("UMGService: Successfully created root canvas panel with name 'CanvasPanel'"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create root canvas panel"));
            UEditorAssetLibrary::DeleteAsset(FullPath);
            return nullptr;
        }
    }

    // Finalize and save
    FAssetRegistryModule::AssetCreated(WidgetBlueprint);
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    Package->MarkPackageDirty();
    UEditorAssetLibrary::SaveAsset(FullPath, false);

    return WidgetBlueprint;
}

UClass* FUMGService::FindParentClass(const FString& ParentClassName) const
{
    if (ParentClassName.IsEmpty() || ParentClassName == TEXT("UserWidget"))
    {
        return UUserWidget::StaticClass();
    }

    // Try to find the parent class with various prefixes
    TArray<FString> PossibleClassPaths;
    PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildUMGPath(ParentClassName));
    PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildEnginePath(ParentClassName));
    PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildCorePath(ParentClassName));
    PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Blueprints/%s.%s_C"), *ParentClassName, *ParentClassName)));
    PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("%s.%s_C"), *ParentClassName, *ParentClassName)));

    for (const FString& ClassPath : PossibleClassPaths)
    {
        UClass* FoundClass = LoadObject<UClass>(nullptr, *ClassPath);
        if (FoundClass)
        {
            return FoundClass;
        }
    }

    return nullptr;
}

bool FUMGService::SetWidgetProperty(UWidget* Widget, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue) const
{
    if (!Widget || !PropertyValue.IsValid())
    {
        return false;
    }

    // Use PropertyService for universal property setting (supports enums, structs, all types)
    FString ErrorMessage;
    bool bSuccess = FPropertyService::Get().SetObjectProperty(Widget, PropertyName, PropertyValue, ErrorMessage);
    
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Failed to set property '%s' on widget '%s': %s"), 
               *PropertyName, *Widget->GetClass()->GetName(), *ErrorMessage);
    }
    
    return bSuccess;
}

bool FUMGService::CreateEventBinding(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, const FString& WidgetVarName, const FString& EventName, const FString& FunctionName) const
{
    return FWidgetBindingService::CreateEventBinding(WidgetBlueprint, Widget, WidgetVarName, EventName, FunctionName);
}

bool FUMGService::CreateTextBlockBindingFunction(UWidgetBlueprint* WidgetBlueprint, const FString& TextBlockName, const FString& BindingName, const FString& VariableType) const
{
    return FWidgetBindingService::CreateTextBlockBindingFunction(WidgetBlueprint, TextBlockName, BindingName, VariableType);
}

bool FUMGService::SetCanvasSlotPlacement(UWidget* Widget, const FVector2D* Position, const FVector2D* Size, const FVector2D* Alignment) const
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
    if (!CanvasSlot)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Widget is not in a canvas panel slot"));
        return false;
    }

    if (Position)
    {
        CanvasSlot->SetPosition(*Position);
    }

    if (Size)
    {
        CanvasSlot->SetSize(*Size);
    }

    if (Alignment)
    {
        CanvasSlot->SetAlignment(*Alignment);
    }

    return true;
}

// SetSlotProperty implementation is in UMGSlotPropertyService.cpp

bool FUMGService::AddWidgetToParent(UWidget* ChildWidget, UWidget* ParentWidget) const
{
    if (!ChildWidget || !ParentWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Invalid child or parent widget"));
        return false;
    }

    // Remove child from its current parent if it has one
    if (ChildWidget->GetParent())
    {
        UPanelWidget* CurrentParentPanel = Cast<UPanelWidget>(ChildWidget->GetParent());
        if (CurrentParentPanel)
        {
            CurrentParentPanel->RemoveChild(ChildWidget);
        }
        else
        {
            // Parent might be a content widget (Border, Button, etc.)
            UContentWidget* CurrentParentContent = Cast<UContentWidget>(ChildWidget->GetParent());
            if (CurrentParentContent)
            {
                CurrentParentContent->SetContent(nullptr);
            }
        }
    }

    // Try panel widget first (CanvasPanel, HorizontalBox, VerticalBox, etc.)
    UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
    if (ParentPanel)
    {
        UPanelSlot* NewSlot = ParentPanel->AddChild(ChildWidget);
        if (!NewSlot)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to add child to parent panel"));
            return false;
        }
        return true;
    }

    // Try content widget (Border, Button, ScaleBox, etc. - single child containers)
    UContentWidget* ParentContent = Cast<UContentWidget>(ParentWidget);
    if (ParentContent)
    {
        // Content widgets can only have one child - check if already has content
        if (ParentContent->GetContent())
        {
            UE_LOG(LogTemp, Warning, TEXT("UMGService: Content widget already has a child, replacing it"));
        }
        ParentContent->SetContent(ChildWidget);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("UMGService: Parent widget '%s' is neither a panel nor content widget - cannot add children"),
           *ParentWidget->GetName());
    return false;
}

bool FUMGService::GetWidgetComponentLayout(const FString& BlueprintName, TSharedPtr<FJsonObject>& OutLayoutInfo)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint '%s' not found"), *BlueprintName);
        return false;
    }

    return FWidgetLayoutService::GetWidgetComponentLayout(WidgetBlueprint, OutLayoutInfo);
}

bool FUMGService::CaptureWidgetScreenshot(const FString& BlueprintName, int32 Width, int32 Height,
                                         const FString& Format, TSharedPtr<FJsonObject>& OutScreenshotData)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint '%s' not found"), *BlueprintName);
        return false;
    }

    return FWidgetLayoutService::CaptureWidgetScreenshot(WidgetBlueprint, Width, Height, Format, OutScreenshotData);
}

bool FUMGService::CreateWidgetInputHandler(const FString& WidgetName, const FString& ComponentName,
                                          const FString& InputType, const FString& InputEvent,
                                          const FString& Trigger, const FString& HandlerName,
                                          FString& OutActualHandlerName)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CreateWidgetInputHandler - Widget blueprint '%s' not found"), *WidgetName);
        return false;
    }

    return FWidgetInputHandlerService::CreateWidgetInputHandler(WidgetBlueprint, ComponentName, InputType, InputEvent, Trigger, HandlerName, OutActualHandlerName);
}

bool FUMGService::RemoveWidgetFunctionGraph(const FString& WidgetName, const FString& FunctionName)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::RemoveWidgetFunctionGraph - Widget blueprint '%s' not found"), *WidgetName);
        return false;
    }

    return FWidgetInputHandlerService::RemoveWidgetFunctionGraph(WidgetBlueprint, FunctionName);
}

bool FUMGService::ReorderWidgetChildren(const FString& WidgetName, const FString& ContainerName,
                                        const TArray<FString>& ChildOrder)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::ReorderWidgetChildren - Widget blueprint '%s' not found"), *WidgetName);
        return false;
    }

    UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::ReorderWidgetChildren - Widget tree not found"));
        return false;
    }

    // Find the container widget
    UWidget* ContainerWidget = WidgetTree->FindWidget(FName(*ContainerName));
    if (!ContainerWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::ReorderWidgetChildren - Container '%s' not found"), *ContainerName);
        return false;
    }

    // Must be a panel widget (HorizontalBox, VerticalBox, etc.)
    UPanelWidget* PanelWidget = Cast<UPanelWidget>(ContainerWidget);
    if (!PanelWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::ReorderWidgetChildren - '%s' is not a panel widget"), *ContainerName);
        return false;
    }

    // Collect current children
    TArray<UWidget*> CurrentChildren;
    for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
    {
        CurrentChildren.Add(PanelWidget->GetChildAt(i));
    }

    // Remove all children (we'll re-add them in order)
    while (PanelWidget->GetChildrenCount() > 0)
    {
        PanelWidget->RemoveChildAt(0);
    }

    // Add children back in specified order
    for (const FString& ChildName : ChildOrder)
    {
        UWidget** FoundChild = CurrentChildren.FindByPredicate([&ChildName](UWidget* W) {
            return W && W->GetName() == ChildName;
        });

        if (FoundChild && *FoundChild)
        {
            PanelWidget->AddChild(*FoundChild);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FUMGService::ReorderWidgetChildren - Child '%s' not found in container"), *ChildName);
        }
    }

    // Mark blueprint as modified
    WidgetBlueprint->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FUMGService::ReorderWidgetChildren - Successfully reordered children in '%s'"), *ContainerName);
    return true;
}
