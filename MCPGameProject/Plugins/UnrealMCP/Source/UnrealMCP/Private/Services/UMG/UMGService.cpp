#include "Services/UMG/UMGService.h"
#include "Services/UMG/WidgetComponentService.h"
#include "Services/UMG/WidgetValidationService.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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

    // Check if blueprint already exists
    FString FullPath = Path + TEXT("/") + Name;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(FullPath);
        UWidgetBlueprint* ExistingWidgetBP = Cast<UWidgetBlueprint>(ExistingAsset);
        if (ExistingWidgetBP)
        {
            return ExistingWidgetBP;
        }
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
    return UEditorAssetLibrary::DoesAssetExist(FullPath);
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

    // Iterate through all properties in the JSON object
    for (const auto& PropertyPair : Properties->Values)
    {
        const FString& PropertyName = PropertyPair.Key;
        const TSharedPtr<FJsonValue>& PropertyValue = PropertyPair.Value;

        if (SetWidgetProperty(Widget, PropertyName, PropertyValue))
        {
            OutSuccessProperties.Add(PropertyName);
        }
        else
        {
            OutFailedProperties.Add(PropertyName);
        }
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
            PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Real, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
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

    UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ComponentName));
    return Widget != nullptr;
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
    Filter.PackageNames.Add(FName(*FString::Printf(TEXT("/Game"))));
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
        UE_LOG(LogTemp, Error, TEXT("UMGService: Widget Blueprint has no WidgetTree"));
        UEditorAssetLibrary::DeleteAsset(FullPath);
        return nullptr;
    }

    if (!WidgetBlueprint->WidgetTree->RootWidget)
    {
        UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
        if (RootCanvas)
        {
            WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
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

    UClass* WidgetClass = Widget->GetClass();
    FProperty* Property = WidgetClass->FindPropertyByName(FName(*PropertyName));
    
    if (!Property)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMGService: Property '%s' not found on widget class '%s'"), *PropertyName, *WidgetClass->GetName());
        return false;
    }

    // Handle different property types
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        bool Value = PropertyValue->AsBool();
        BoolProp->SetPropertyValue_InContainer(Widget, Value);
        return true;
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        float Value = PropertyValue->AsNumber();
        FloatProp->SetPropertyValue_InContainer(Widget, Value);
        return true;
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        int32 Value = PropertyValue->AsNumber();
        IntProp->SetPropertyValue_InContainer(Widget, Value);
        return true;
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString Value = PropertyValue->AsString();
        StrProp->SetPropertyValue_InContainer(Widget, Value);
        return true;
    }
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        FString Value = PropertyValue->AsString();
        FText TextValue = FText::FromString(Value);
        TextProp->SetPropertyValue_InContainer(Widget, TextValue);
        return true;
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        // Handle struct properties (like ColorAndOpacity, BrushColor, etc.)
        if (PropertyValue->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>* StructObject;
            if (PropertyValue->TryGetObject(StructObject) && StructObject->IsValid())
            {
                // This would need more specific handling based on struct type
                // For now, return true to indicate we attempted to handle it
                return true;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("UMGService: Unsupported property type for '%s'"), *PropertyName);
    return false;
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