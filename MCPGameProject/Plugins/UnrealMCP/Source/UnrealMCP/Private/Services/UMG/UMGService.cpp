#include "Services/UMG/UMGService.h"
#include "Services/UMG/WidgetComponentService.h"
#include "Services/UMG/WidgetValidationService.h"
#include "Services/PropertyService.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_VariableGet.h"
#include "UObject/TextProperty.h"
#include "UObject/EnumProperty.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Includes for screenshot capture
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Base64.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/World.h"
#include "Editor.h"

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
                                        const FVector2D& Size, const TSharedPtr<FJsonObject>& Kwargs)
{
    // Validate parameters
    if (ValidationService)
    {
        FWidgetValidationResult ValidationResult = ValidationService->ValidateWidgetComponentCreation(BlueprintName, ComponentName, ComponentType, Position, Size, Kwargs);
        if (!ValidationResult.bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Widget component creation validation failed: %s"), *ValidationResult.ErrorMessage);
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
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find widget blueprint: %s"), *BlueprintName);
        return nullptr;
    }

    if (!WidgetComponentService)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: WidgetComponentService is null"));
        return nullptr;
    }

    return WidgetComponentService->CreateWidgetComponent(WidgetBlueprint, ComponentName, ComponentType, Position, Size, Kwargs);
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

    // Use PropertyService for universal property setting
    TArray<FString> SuccessProps;
    TMap<FString, FString> FailedProps;
    
    FPropertyService::Get().SetObjectProperties(Widget, Properties, SuccessProps, FailedProps);
    
    // Convert to output format
    OutSuccessProperties = SuccessProps;
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



    OutActualFunctionName = FunctionName.IsEmpty() ? (ComponentName + TEXT("_") + EventName) : FunctionName;
    
    return CreateEventBinding(WidgetBlueprint, Widget, EventName, OutActualFunctionName);
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

    return CreateTextBlockBindingFunction(WidgetBlueprint, BindingName, VariableType);
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
        ParentWidget = WidgetComponentService->CreateWidgetComponent(WidgetBlueprint, ParentComponentName, ParentComponentType, ParentPosition, ParentSize, EmptyKwargs);
        if (!ParentWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create parent widget component: %s"), *ParentComponentName);
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

bool FUMGService::CreateParentAndChildWidgetComponents(const FString& BlueprintName, const FString& ParentComponentName,
                                                     const FString& ChildComponentName, const FString& ParentComponentType,
                                                     const FString& ChildComponentType, const FVector2D& ParentPosition,
                                                     const FVector2D& ParentSize, const TSharedPtr<FJsonObject>& ChildAttributes)
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

    // Create the parent component
    TSharedPtr<FJsonObject> EmptyKwargs = MakeShared<FJsonObject>();
    UWidget* ParentWidget = WidgetComponentService->CreateWidgetComponent(WidgetBlueprint, ParentComponentName, ParentComponentType, ParentPosition, ParentSize, EmptyKwargs);
    if (!ParentWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create parent widget component: %s"), *ParentComponentName);
        return false;
    }

    // Create the child component
    UWidget* ChildWidget = WidgetComponentService->CreateWidgetComponent(WidgetBlueprint, ChildComponentName, ChildComponentType, FVector2D(0.0f, 0.0f), FVector2D(100.0f, 50.0f), ChildAttributes);
    if (!ChildWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to create child widget component: %s"), *ChildComponentName);
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

bool FUMGService::CreateEventBinding(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, const FString& EventName, const FString& FunctionName) const
{
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
    if (!EventGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to find event graph"));
        return false;
    }

    FName EventFName(*EventName);
    
    // Check if event node already exists
    TArray<UK2Node_Event*> AllEventNodes;
    FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, AllEventNodes);
    
    for (UK2Node_Event* Node : AllEventNodes)
    {
        UClass* NodeEventParentClass = Node->EventReference.GetMemberParentClass();
        if (Node->EventReference.GetMemberName() == EventFName && NodeEventParentClass == Widget->GetClass())
        {
            // Event already bound
            return true;
        }
    }

    // Find the delegate property on the widget's class
    FProperty* FoundProperty = Widget->GetClass()->FindPropertyByName(EventFName);
    FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(FoundProperty);
    
    if (!DelegateProperty)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Could not find delegate property '%s' on class '%s'"), *EventName, *Widget->GetClass()->GetName());
        return false;
    }

    // Calculate position for new node
    float MaxHeight = 0.0f;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        MaxHeight = FMath::Max(MaxHeight, Node->NodePosY);
    }
    const FVector2D NodePos(200, MaxHeight + 200);

    // Create the event node
    UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);
    EventNode->EventReference.SetExternalDelegateMember(DelegateProperty->GetFName());
    EventNode->bOverrideFunction = true;
    EventNode->CustomFunctionName = FName(*FunctionName);
    EventNode->NodePosX = NodePos.X;
    EventNode->NodePosY = NodePos.Y;
    
    EventGraph->AddNode(EventNode, true);
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    EventNode->ReconstructNode();

    // Save the blueprint
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    return true;
}

bool FUMGService::CreateTextBlockBindingFunction(UWidgetBlueprint* WidgetBlueprint, const FString& BindingName, const FString& VariableType) const
{
    const FString FunctionName = FString::Printf(TEXT("Get%s"), *BindingName);

    // Check if function already exists
    for (UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == FunctionName)
        {
            return true; // Function already exists
        }
    }

    // Create binding function
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        WidgetBlueprint,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );

    if (!FuncGraph)
    {
        return false;
    }

    FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, FuncGraph, false, nullptr);

    // Find or create entry node
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        EntryNode = Cast<UK2Node_FunctionEntry>(Node);
        if (EntryNode)
        {
            break;
        }
    }

    if (!EntryNode)
    {
        EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
        FuncGraph->AddNode(EntryNode, false, false);
        EntryNode->NodePosX = 0;
        EntryNode->NodePosY = 0;
        EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBlueprint->GeneratedClass);
        EntryNode->AllocateDefaultPins();
    }

    // Create get variable node
    UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
    GetVarNode->VariableReference.SetSelfMember(FName(*BindingName));
    FuncGraph->AddNode(GetVarNode, false, false);
    GetVarNode->NodePosX = 200;
    GetVarNode->NodePosY = 0;
    GetVarNode->AllocateDefaultPins();

    // Create function result node
    UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
    FuncGraph->AddNode(ResultNode, false, false);
    ResultNode->NodePosX = 400;
    ResultNode->NodePosY = 0;
    ResultNode->UserDefinedPins.Empty();

    // Set up return pin type
    FEdGraphPinType ReturnPinType;
    if (VariableType == TEXT("Text"))
    {
        ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
    }
    else if (VariableType == TEXT("String"))
    {
        ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (VariableType == TEXT("Int") || VariableType == TEXT("Integer"))
    {
        ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (VariableType == TEXT("Float"))
    {
        ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        ReturnPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
    }
    else if (VariableType == TEXT("Boolean") || VariableType == TEXT("Bool"))
    {
        ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else
    {
        ReturnPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
    }

    // Add return value pin
    TSharedPtr<FUserPinInfo> ReturnPin = MakeShared<FUserPinInfo>();
    ReturnPin->PinName = TEXT("ReturnValue");
    ReturnPin->PinType = ReturnPinType;
    ReturnPin->DesiredPinDirection = EGPD_Output;
    ResultNode->UserDefinedPins.Add(ReturnPin);
    ResultNode->ReconstructNode();

    // Connect the nodes
    UEdGraphPin* GetVarOutputPin = GetVarNode->FindPin(FName(*BindingName), EGPD_Output);
    UEdGraphPin* ResultInputPin = ResultNode->FindPin(TEXT("ReturnValue"), EGPD_Input);
    
    if (GetVarOutputPin && ResultInputPin)
    {
        GetVarOutputPin->MakeLinkTo(ResultInputPin);
    }

    // Save the blueprint
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    return true;
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

bool FUMGService::AddWidgetToParent(UWidget* ChildWidget, UWidget* ParentWidget) const
{
    if (!ChildWidget || !ParentWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Invalid child or parent widget"));
        return false;
    }

    // Check if parent is a panel widget that can contain children
    UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
    if (!ParentPanel)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Parent widget is not a panel widget"));
        return false;
    }

    // Remove child from its current parent if it has one
    if (ChildWidget->GetParent())
    {
        UPanelWidget* CurrentParent = Cast<UPanelWidget>(ChildWidget->GetParent());
        if (CurrentParent)
        {
            CurrentParent->RemoveChild(ChildWidget);
        }
    }

    // Add child to new parent
    UPanelSlot* NewSlot = ParentPanel->AddChild(ChildWidget);
    if (!NewSlot)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to add child to parent panel"));
        return false;
    }

    return true;
}

bool FUMGService::GetWidgetComponentLayout(const FString& BlueprintName, TSharedPtr<FJsonObject>& OutLayoutInfo)
{
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint '%s' not found"), *BlueprintName);
        return false;
    }

    if (!WidgetBlueprint->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget blueprint '%s' has no widget tree"), *BlueprintName);
        return false;
    }

    OutLayoutInfo = MakeShareable(new FJsonObject);
    
    // Get the root widget
    UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget;
    if (!RootWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Widget blueprint '%s' has no root widget"), *BlueprintName);
        OutLayoutInfo->SetBoolField(TEXT("success"), true);
        OutLayoutInfo->SetStringField(TEXT("message"), TEXT("Widget has no root widget"));
        return true;
    }

    // Build hierarchical layout information
    TSharedPtr<FJsonObject> HierarchyInfo = BuildWidgetHierarchy(RootWidget);
    if (HierarchyInfo.IsValid())
    {
        OutLayoutInfo->SetObjectField(TEXT("hierarchy"), HierarchyInfo);
        OutLayoutInfo->SetBoolField(TEXT("success"), true);
        OutLayoutInfo->SetStringField(TEXT("message"), TEXT("Successfully retrieved widget component layout"));
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("UMGService: Failed to build widget hierarchy for '%s'"), *BlueprintName);
    return false;
}

TSharedPtr<FJsonObject> FUMGService::BuildWidgetHierarchy(UWidget* Widget) const
{
    if (!Widget)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> WidgetInfo = MakeShareable(new FJsonObject);
    
    // Basic widget information
    WidgetInfo->SetStringField(TEXT("name"), Widget->GetName());
    WidgetInfo->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
    
    // Get slot properties based on slot type
    TSharedPtr<FJsonObject> SlotProperties = MakeShareable(new FJsonObject);
    if (Widget->Slot)
    {
        FString SlotTypeName = Widget->Slot->GetClass()->GetName();
        SlotProperties->SetStringField(TEXT("slot_type"), SlotTypeName);
        
        // Handle different slot types
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            // Canvas panel slot properties
            FVector2D Position = CanvasSlot->GetPosition();
            FVector2D Size = CanvasSlot->GetSize();
            
            TArray<TSharedPtr<FJsonValue>> PositionArray;
            PositionArray.Add(MakeShareable(new FJsonValueNumber(Position.X)));
            PositionArray.Add(MakeShareable(new FJsonValueNumber(Position.Y)));
            SlotProperties->SetArrayField(TEXT("position"), PositionArray);
            
            TArray<TSharedPtr<FJsonValue>> SizeArray;
            SizeArray.Add(MakeShareable(new FJsonValueNumber(Size.X)));
            SizeArray.Add(MakeShareable(new FJsonValueNumber(Size.Y)));
            SlotProperties->SetArrayField(TEXT("size"), SizeArray);
            
            SlotProperties->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
        }
        else if (Widget->Slot->IsA<UPanelSlot>())
        {
            // Generic panel slot - try to get common properties
            UPanelSlot* PanelSlot = Cast<UPanelSlot>(Widget->Slot);
            if (PanelSlot)
            {
                // Try to get padding if available
                if (PanelSlot->GetClass()->FindPropertyByName(TEXT("Padding")))
                {
                    // This is a simplified approach - in a full implementation,
                    // you'd need to handle specific slot types individually
                    SlotProperties->SetStringField(TEXT("note"), TEXT("Panel slot properties available but not fully implemented"));
                }
            }
        }
    }
    
    WidgetInfo->SetObjectField(TEXT("slot_properties"), SlotProperties);
    
    // Handle child widgets for panel widgets
    TArray<TSharedPtr<FJsonValue>> ChildrenArray;
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
    {
        for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
        {
            UWidget* ChildWidget = PanelWidget->GetChildAt(i);
            if (ChildWidget)
            {
                TSharedPtr<FJsonObject> ChildInfo = BuildWidgetHierarchy(ChildWidget);
                if (ChildInfo.IsValid())
                {
                    ChildrenArray.Add(MakeShareable(new FJsonValueObject(ChildInfo)));
                }
            }
        }
    }
    
    WidgetInfo->SetArrayField(TEXT("children"), ChildrenArray);

    return WidgetInfo;
}

bool FUMGService::CaptureWidgetScreenshot(const FString& BlueprintName, int32 Width, int32 Height,
                                         const FString& Format, TSharedPtr<FJsonObject>& OutScreenshotData)
{
    UE_LOG(LogTemp, Log, TEXT("FUMGService::CaptureWidgetScreenshot - Capturing screenshot for '%s' at %dx%d"),
           *BlueprintName, Width, Height);

    // Find the widget blueprint
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Widget blueprint '%s' not found"), *BlueprintName);
        return false;
    }

    // Verify the widget has a generated class
    if (!WidgetBlueprint->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Widget blueprint '%s' has no generated class"), *BlueprintName);
        return false;
    }

    // Get the editor world
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - No editor world available"));
        return false;
    }

    // Create a preview instance of the widget
    UClass* GeneratedClass = WidgetBlueprint->GeneratedClass;
    if (!GeneratedClass || !GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Invalid or incompatible generated class"));
        return false;
    }
    UUserWidget* PreviewWidget = CreateWidget<UUserWidget>(EditorWorld, GeneratedClass);
    if (!PreviewWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Failed to create widget preview instance"));
        return false;
    }

    // Get the Slate widget
    TSharedPtr<SWidget> SlateWidget = PreviewWidget->TakeWidget();
    if (!SlateWidget.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Failed to get Slate widget"));
        PreviewWidget->RemoveFromParent();
        PreviewWidget->MarkAsGarbage();
        return false;
    }

    // Create render target
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, true);
    RenderTarget->UpdateResourceImmediate(true);

    // Render widget to texture
    FWidgetRenderer WidgetRenderer(true, false);
    WidgetRenderer.DrawWidget(
        RenderTarget,
        SlateWidget.ToSharedRef(),
        FVector2D(Width, Height),
        0.0f);

    // Flush rendering commands to ensure texture is ready
    FlushRenderingCommands();

    // Read pixels from render target
    FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
    if (!RTResource)
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Failed to get render target resource"));
        PreviewWidget->RemoveFromParent();
        PreviewWidget->MarkAsGarbage();
        return false;
    }

    TArray<FColor> OutPixels;
    if (!RTResource->ReadPixels(OutPixels))
    {
        UE_LOG(LogTemp, Error, TEXT("FUMGService::CaptureWidgetScreenshot - Failed to read pixels from render target"));
        PreviewWidget->RemoveFromParent();
        PreviewWidget->MarkAsGarbage();
        return false;
    }

    // Compress to PNG or JPEG
    TArray<uint8> CompressedImage;
    FString ActualFormat = Format.ToLower();

    if (ActualFormat == TEXT("jpg") || ActualFormat == TEXT("jpeg"))
    {
        // JPEG compression - use thumbnail compression for JPEG
        FImageUtils::ThumbnailCompressImageArray(Width, Height, OutPixels, CompressedImage);
    }
    else
    {
        // PNG compression (default)
        ActualFormat = TEXT("png");
        TArray64<uint8> CompressedImage64;
        FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(OutPixels), CompressedImage64);
        // Convert from TArray64 to TArray
        CompressedImage.Append(CompressedImage64.GetData(), CompressedImage64.Num());
    }

    // Encode as base64
    FString Base64Image = FBase64::Encode(CompressedImage);

    // Build response
    OutScreenshotData = MakeShareable(new FJsonObject);
    OutScreenshotData->SetBoolField(TEXT("success"), true);
    OutScreenshotData->SetStringField(TEXT("image_base64"), Base64Image);
    OutScreenshotData->SetNumberField(TEXT("width"), Width);
    OutScreenshotData->SetNumberField(TEXT("height"), Height);
    OutScreenshotData->SetStringField(TEXT("format"), ActualFormat);
    OutScreenshotData->SetNumberField(TEXT("image_size_bytes"), CompressedImage.Num());

    // Clean up
    PreviewWidget->RemoveFromParent();
    PreviewWidget->MarkAsGarbage();
    RenderTarget->MarkAsGarbage();

    UE_LOG(LogTemp, Log, TEXT("FUMGService::CaptureWidgetScreenshot - Screenshot captured successfully, %d bytes"),
           CompressedImage.Num());

    return true;
}
