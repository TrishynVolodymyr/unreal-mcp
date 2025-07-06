#include "Commands/UnrealMCPUMGCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "WidgetBlueprint.h"
#include <functional>
// We'll create widgets using regular Factory classes
#include "Factories/Factory.h"
// Add missing BlueprintFactory include
#include "Factories/BlueprintFactory.h"
// Add AssetRegistry for searching across content folder
#include "AssetRegistry/AssetRegistryModule.h"
#include "WidgetBlueprintEditor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "JsonObjectConverter.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/Button.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_Event.h"
#include "Misc/Paths.h"
#include "UObject/TextProperty.h" // For FText
#include "UObject/EnumProperty.h" // For Enum properties
#include "Serialization/JsonSerializer.h" // For parsing JSON property values

// Include additional widget components
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Components/Throbber.h"
#include "Components/ExpandableArea.h"
#include "Components/MenuAnchor.h"
#include "Components/RichTextBlock.h"
#include "Components/SafeZone.h"
#include "Components/InvalidationBox.h"
#include "Components/InputKeySelector.h"
#include "Components/MultiLineEditableText.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/GridPanel.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/CircularThrobber.h"
#include "Components/SpinBox.h"
#include "Components/WrapBox.h"
#include "Components/ScaleBox.h"
#include "Components/NamedSlot.h"
#include "Components/RadialSlider.h"
#include "Components/NativeWidgetHost.h"
#include "Components/BackgroundBlur.h"
#include "Components/SafeZone.h"
#include "Components/MenuAnchor.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/UniformGridPanel.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/GridSlot.h"
#include "Components/UniformGridSlot.h"
#include "Components/WrapBoxSlot.h"
#include "Components/RetainerBox.h"
#include "Components/WindowTitleBarArea.h"

#include "Services/UMG/WidgetComponentService.h"
#include "Components/SlateWrapperTypes.h" // For FSlateChildSize, ESlateSizeRule

// Define log category if it doesn't exist
DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCPUMG, Log, All);

// Helper function to find a Widget Blueprint by name
UWidgetBlueprint* FindWidgetBlueprint(const FString& BlueprintNameOrPath)
{
    UE_LOG(LogTemp, Log, TEXT("UMG: Searching for Widget Blueprint: %s"), *BlueprintNameOrPath);
    
    // Check if we already have a full path
    if (BlueprintNameOrPath.StartsWith(TEXT("/Game/")))
    {
        // Try direct loading first with provided path
        FString FullPath = BlueprintNameOrPath;
        
        // Add .uasset extension if not provided
        if (!FPaths::GetExtension(FullPath).IsEmpty())
        {
            // Path already has extension, so we can use it directly
        }
        else if (FullPath.EndsWith(TEXT("."), ESearchCase::CaseSensitive))
        {
            // Remove trailing dot
            FullPath = FullPath.LeftChop(1);
        }
        
        // Try directly loading the asset
        UE_LOG(LogTemp, Log, TEXT("UMG: Trying to load Widget Blueprint with direct path: %s"), *FullPath);
        UObject* Asset = UEditorAssetLibrary::LoadAsset(FullPath);
        UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Asset);
        if (WidgetBlueprint)
        {
            UE_LOG(LogTemp, Log, TEXT("UMG: Found Widget Blueprint at path: %s"), *FullPath);
            return WidgetBlueprint;
        }
    }
    
    // If not found with direct path, try common directories
    TArray<FString> SearchPaths = {
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Widgets/%s"), *BlueprintNameOrPath)),
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("UI/%s"), *BlueprintNameOrPath)),
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("UMG/%s"), *BlueprintNameOrPath)),
        FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Interface/%s"), *BlueprintNameOrPath))
    };
    
    for (const FString& SearchPath : SearchPaths)
    {
        UE_LOG(LogTemp, Log, TEXT("UMG: Trying common path: %s"), *SearchPath);
        UObject* Asset = UEditorAssetLibrary::LoadAsset(SearchPath);
        UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Asset);
        if (WidgetBlueprint)
        {
            UE_LOG(LogTemp, Log, TEXT("UMG: Found Widget Blueprint at path: %s"), *SearchPath);
            return WidgetBlueprint;
        }
    }
    
    // If still not found, use asset registry to search everywhere
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetData;
    
    FARFilter Filter;
    // Use ClassPaths instead of ClassNames (which is deprecated in UE 5.5)
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackageNames.Add(FName(*FString::Printf(TEXT("/Game"))));
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    
    UE_LOG(LogTemp, Log, TEXT("UMG: Searching asset registry for Widget Blueprint with name: %s"), *BlueprintNameOrPath);
    
    for (const FAssetData& Asset : AssetData)
    {
        FString AssetName = Asset.AssetName.ToString();
        
        // Check if asset name matches (case insensitive)
        if (AssetName.Equals(BlueprintNameOrPath, ESearchCase::IgnoreCase))
        {
            // Use GetSoftObjectPath instead of ObjectPath (which is deprecated in UE 5.5)
            FString AssetPath = Asset.GetSoftObjectPath().ToString();
            UE_LOG(LogTemp, Log, TEXT("UMG: Found Widget Blueprint in asset registry: %s"), *AssetPath);
            UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
            UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(LoadedAsset);
            if (WidgetBlueprint)
            {
                return WidgetBlueprint;
            }
        }
    }
    
    // Not found anywhere
    UE_LOG(LogTemp, Warning, TEXT("UMG: Widget Blueprint not found: %s"), *BlueprintNameOrPath);
    return nullptr;
}

// Helper function to safely get an array from a JSON object
bool GetJsonArray(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<TSharedPtr<FJsonValue>>& OutArray)
{
	const TArray<TSharedPtr<FJsonValue>>* ArrayPtr = nullptr;
	if (JsonObject->TryGetArrayField(*FieldName, ArrayPtr) && ArrayPtr)
	{
		OutArray = *ArrayPtr;
		return true;
	}
	return false;
}

FUnrealMCPUMGCommands::FUnrealMCPUMGCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("create_umg_widget_blueprint"))
	{
		return HandleCreateUMGWidgetBlueprint(Params);
	}
	else if (CommandName == TEXT("add_widget_to_viewport"))
	{
		return HandleAddWidgetToViewport(Params);
	}
	else if (CommandName == TEXT("bind_widget_component_event"))
	{
		return HandleBindWidgetComponentEvent(Params);
	}
	else if (CommandName == TEXT("set_text_block_widget_component_binding"))
	{
		return HandleSetTextBlockBinding(Params);
	}
	else if (CommandName == TEXT("check_widget_component_exists"))
	{
		return HandleCheckComponentExists(Params);
	}
	else if (CommandName == TEXT("add_child_widget_component_to_parent"))
	{
		return HandleAddWidgetAsChild(Params);
	}
	else if (CommandName == TEXT("create_parent_and_child_widget_components"))
	{
		return HandleCreateWidgetComponentWithChild(Params);
	}
	else if (CommandName == TEXT("set_widget_component_placement"))
	{
		return HandleSetWidgetPlacement(Params);
	}
	else if (CommandName == TEXT("get_widget_container_component_dimensions"))
	{
		return HandleGetWidgetContainerDimensions(Params);
	}
	else if (CommandName == TEXT("add_widget_component_to_widget"))
	{
		return HandleAddWidgetComponent(Params);
	}
	else if (CommandName == TEXT("set_widget_component_property"))
	{
		return HandleSetWidgetComponentProperty(Params);
	}
	else if (CommandName == TEXT("get_widget_component_layout"))
	{
		return HandleGetWidgetComponentLayout(Params);
	}
	
	return FUnrealMCPCommonUtils::CreateErrorResponse(*FString::Printf(TEXT("Unknown UMG command: %s"), *CommandName));
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Create the full asset path
	FString PackagePath = TEXT("/Game/Widgets/");
	FString AssetName = BlueprintName;
	FString FullPath = PackagePath + AssetName;

	UE_LOG(LogTemp, Log, TEXT("UMG: Creating Widget Blueprint at path: %s"), *FullPath);

	// Check if asset already exists
	if (UEditorAssetLibrary::DoesAssetExist(FullPath))
	{
		UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(FullPath);
		UWidgetBlueprint* ExistingWidgetBP = Cast<UWidgetBlueprint>(ExistingAsset);
		
		// If it exists and is a Widget Blueprint, we can return it
		if (ExistingWidgetBP)
		{
			UE_LOG(LogTemp, Log, TEXT("UMG: Widget Blueprint already exists, returning existing asset"));
			// Create success response for existing blueprint
			TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
			ResultObj->SetStringField(TEXT("name"), BlueprintName);
			ResultObj->SetStringField(TEXT("path"), FullPath);
			ResultObj->SetBoolField(TEXT("already_exists"), true);
			ResultObj->SetBoolField(TEXT("success"), true);
			return ResultObj;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UMG: Asset exists but is not a Widget Blueprint"));
			// Asset exists but is not a Widget Blueprint
			return FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Asset '%s' exists but is not a Widget Blueprint"), *BlueprintName)
			);
		}
	}

	// Find parent class - default is UUserWidget
	UClass* ParentClass = UUserWidget::StaticClass(); 
	if (Params->HasField(TEXT("parent_class")))
	{
		FString ParentClassName;
		Params->TryGetStringField(TEXT("parent_class"), ParentClassName);
		
		if (!ParentClassName.IsEmpty() && ParentClassName != TEXT("UserWidget"))
		{
			// Try to find the parent class with various prefixes
			UClass* FoundClass = nullptr;
			TArray<FString> PossibleClassPaths;
			PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildUMGPath(ParentClassName));
			PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildEnginePath(ParentClassName));
			PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildCorePath(ParentClassName));
			PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("Blueprints/%s.%s_C"), *ParentClassName, *ParentClassName)));
			PossibleClassPaths.Add(FUnrealMCPCommonUtils::BuildGamePath(FString::Printf(TEXT("%s.%s_C"), *ParentClassName, *ParentClassName)));
			
			for (const FString& ClassPath : PossibleClassPaths)
			{
				FoundClass = LoadObject<UClass>(nullptr, *ClassPath);
				if (FoundClass)
				{
					break;
				}
			}
			
			if (FoundClass)
			{
				ParentClass = FoundClass;
				UE_LOG(LogTemp, Log, TEXT("UMG: Using parent class: %s"), *ParentClass->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("UMG: Could not find parent class: %s, using default"), *ParentClassName);
			}
		}
	}

	// Create package for the new asset
	UE_LOG(LogTemp, Log, TEXT("UMG: Creating package for: %s"), *FullPath);
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("UMG: Failed to create package for path: %s"), *FullPath);
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create the Widget Blueprint
	UE_LOG(LogTemp, Log, TEXT("UMG: Creating Widget Blueprint"));
	
	// Create Blueprint using KismetEditorUtilities directly
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,                       // Parent class
		Package,                           // Outer package
		FName(*AssetName),                 // Blueprint name
		BPTYPE_Normal,                     // Blueprint type
		UWidgetBlueprint::StaticClass(),   // Blueprint class
		UWidgetBlueprintGeneratedClass::StaticClass()  // Generated class
	);
	
	if (!NewBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("UMG: Failed to create blueprint"));
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create blueprint"));
	}
	
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewBlueprint);
	if (!WidgetBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("UMG: Created blueprint is not a UWidgetBlueprint, deleting and returning error"));
		UEditorAssetLibrary::DeleteAsset(FullPath);
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Created blueprint is not a Widget Blueprint"));
	}
	
	UE_LOG(LogTemp, Log, TEXT("UMG: Successfully created Widget Blueprint object"));

	// Ensure the WidgetTree exists
	if (!WidgetBlueprint->WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("UMG: Widget Blueprint has no WidgetTree"));
		UEditorAssetLibrary::DeleteAsset(FullPath);
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint has no WidgetTree"));
	}

	// Add a default Canvas Panel root widget if needed
	if (!WidgetBlueprint->WidgetTree->RootWidget)
	{
		UE_LOG(LogTemp, Log, TEXT("UMG: Adding default Canvas Panel to Widget Blueprint"));
		UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		if (!RootCanvas)
		{
			UE_LOG(LogTemp, Error, TEXT("UMG: Failed to create Canvas Panel"));
			UEditorAssetLibrary::DeleteAsset(FullPath);
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Canvas Panel root widget"));
		}
		
		WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
	}

	// Finalize and save the asset
	UE_LOG(LogTemp, Log, TEXT("UMG: Finalizing and saving Widget Blueprint"));
	
	// Update asset registry
	FAssetRegistryModule::AssetCreated(WidgetBlueprint);
	
	// Compile
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	
	// Mark dirty and save
	Package->MarkPackageDirty();
	bool bSaved = UEditorAssetLibrary::SaveAsset(FullPath, false);
	
	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMG: Saving Widget Blueprint encountered issues"));
	}

	// Return success
	UE_LOG(LogTemp, Log, TEXT("UMG: Widget Blueprint creation completed successfully"));
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), BlueprintName);
	ResultObj->SetStringField(TEXT("path"), FullPath);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	// Find the Widget Blueprint using our helper function
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get optional Z-order parameter
	int32 ZOrder = 0;
	Params->TryGetNumberField(TEXT("z_order"), ZOrder);

	// Create widget instance
	UClass* WidgetClass = WidgetBlueprint->GeneratedClass;
	if (!WidgetClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get widget class"));
	}

	// Note: This creates the widget but doesn't add it to viewport
	// The actual addition to viewport should be done through Blueprint nodes
	// as it requires a game context

	// Create success response with instructions
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("class_path"), WidgetClass->GetPathName());
	ResultObj->SetNumberField(TEXT("z_order"), ZOrder);
	ResultObj->SetStringField(TEXT("note"), TEXT("Widget class ready. Use CreateWidget and AddToViewport nodes in Blueprint to display in game."));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	// Use the correct parameter name: widget_component_name
	FString WidgetComponentName; 
	if (!Params->TryGetStringField(TEXT("widget_component_name"), WidgetComponentName)) 
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_component_name parameter"));
		return Response;
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing event_name parameter"));
		return Response;
	}

	FString FunctionName;
	Params->TryGetStringField(TEXT("function_name"), FunctionName);
	if (FunctionName.IsEmpty())
	{
		// Default function name is WidgetComponentName_EventName
		FunctionName = WidgetComponentName + TEXT("_") + EventName; 
	}

	// Load the Widget Blueprint using our helper function
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintName));
		return Response;
	}

	// Create the event graph if it doesn't exist
	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
	if (!EventGraph)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to find or create event graph"));
		return Response;
	}

	// Find the widget in the blueprint using the correct name
	UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(*WidgetComponentName);
	if (!Widget)
	{
		// Update error message to reflect the parameter name used
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to find widget component: %s"), *WidgetComponentName)); 
		return Response;
	}

	// Create the event node (e.g., OnClicked for buttons)
	UK2Node_Event* EventNode = nullptr;
	FName EventFName(*EventName);
	
	UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Searching for existing event node for event '%s' on widget '%s'"), *EventName, *WidgetComponentName);

	// Find existing nodes first
	TArray<UK2Node_Event*> AllEventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, AllEventNodes);
	
	for (UK2Node_Event* Node : AllEventNodes)
	{
		// Check if an existing node matches the specific event and widget class
		UClass* NodeEventParentClass = Node->EventReference.GetMemberParentClass();
		UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Checking existing node: Name='%s', CustomFunc='%s', EventParent='%s', MemberName='%s'"), 
			*Node->GetName(), *Node->CustomFunctionName.ToString(), 
			NodeEventParentClass ? *NodeEventParentClass->GetName() : TEXT("NULL"),
			*Node->EventReference.GetMemberName().ToString()); // Log MemberName too

		// We need to check the MemberName in EventReference for component events
		if (Node->EventReference.GetMemberName() == EventFName && NodeEventParentClass == Widget->GetClass())
		{
			UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Found existing event node: %s"), *Node->GetName());
			EventNode = Node;
			break;
		}
	}

	// If no existing node, create a new one manually
	if (!EventNode)
	{
		UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: No existing node found. Attempting to create MANUALLY..."));
		
		// Find the delegate property on the widget's class using FindPropertyByName and CastField
		FProperty* FoundProperty = Widget->GetClass()->FindPropertyByName(EventFName);
		FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(FoundProperty);
		
		if (!DelegateProperty)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleBindWidgetComponentEvent: Could not find Multicast Delegate Property '%s' on class '%s'"), *EventName, *Widget->GetClass()->GetName());
			Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Event '%s' not found on widget class '%s'"), *EventName, *Widget->GetClass()->GetName()));
			return Response;
		}

		UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Found delegate property '%s'. Creating UK2Node_Event."), *EventName);

		// Calculate position
		float MaxHeight = 0.0f;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			MaxHeight = FMath::Max(MaxHeight, Node->NodePosY);
		}
		const FVector2D NodePos(200, MaxHeight + 200);

		// Create the K2Node_Event
		EventNode = NewObject<UK2Node_Event>(EventGraph);
		EventNode->EventReference.SetExternalDelegateMember(DelegateProperty->GetFName());
		EventNode->bOverrideFunction = true; // Necessary for delegate events?
		EventNode->CustomFunctionName = FName(*FunctionName); // Use the generated or provided function name
		EventNode->NodePosX = NodePos.X;
		EventNode->NodePosY = NodePos.Y;
		
		UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Adding manually created node to graph..."));
		EventGraph->AddNode(EventNode, true);
		
		// Initialize the node
		EventNode->CreateNewGuid();
		EventNode->PostPlacedNewNode();
		EventNode->AllocateDefaultPins();
		EventNode->ReconstructNode(); // Reconstruct for safety
		
		UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Manual node creation complete. Node ID: %s"), *EventNode->NodeGuid.ToString());
	}

	// Check if EventNode is valid now (either found existing or created manually)
	if (!EventNode)
	{
		UE_LOG(LogTemp, Error, TEXT("HandleBindWidgetComponentEvent: EventNode is still NULL after manual creation attempt for event '%s' on widget '%s'"), *EventName, *WidgetComponentName);
		Response->SetStringField(TEXT("error"), TEXT("Failed to create event node (manual attempt)"));
		return Response;
	}

	// Save the Widget Blueprint
	UE_LOG(LogTemp, Log, TEXT("HandleBindWidgetComponentEvent: Event node found/created successfully. Saving blueprint..."));
	WidgetBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("event_name"), EventName);
	Response->SetStringField(TEXT("function_name"), FunctionName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString BindingName;
	if (!Params->TryGetStringField(TEXT("binding_name"), BindingName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing binding_name parameter"));
		return Response;
	}

	// Load the Widget Blueprint using our helper function
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintName));
		return Response;
	}

	// Create a variable for binding if it doesn't exist
	FBlueprintEditorUtils::AddMemberVariable(
		WidgetBlueprint,
		FName(*BindingName),
		FEdGraphPinType(UEdGraphSchema_K2::PC_Text, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType())
	);

	// Find the TextBlock widget
	UTextBlock* TextBlock = Cast<UTextBlock>(WidgetBlueprint->WidgetTree->FindWidget(FName(*WidgetName)));
	if (!TextBlock)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to find TextBlock widget: %s"), *WidgetName));
		return Response;
	}

	// Create binding function
	const FString FunctionName = FString::Printf(TEXT("Get%s"), *BindingName);
	UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
		WidgetBlueprint,
		FName(*FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (FuncGraph)
	{
		// Add the function to the blueprint with proper template parameter
		// Template requires null for last parameter when not using a signature-source
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, FuncGraph, false, nullptr);

		// Create entry node
		UK2Node_FunctionEntry* EntryNode = nullptr;
		
		// Create entry node - use the API that exists in UE 5.5
		EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
		FuncGraph->AddNode(EntryNode, false, false);
		EntryNode->NodePosX = 0;
		EntryNode->NodePosY = 0;
		EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBlueprint->GeneratedClass);
		EntryNode->AllocateDefaultPins();

		// Create get variable node
		UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
		GetVarNode->VariableReference.SetSelfMember(FName(*BindingName));
		FuncGraph->AddNode(GetVarNode, false, false);
		GetVarNode->NodePosX = 200;
		GetVarNode->NodePosY = 0;
		GetVarNode->AllocateDefaultPins();

		// Connect nodes
		UEdGraphPin* EntryThenPin = EntryNode->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* GetVarOutPin = GetVarNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
		if (EntryThenPin && GetVarOutPin)
		{
			EntryThenPin->MakeLinkTo(GetVarOutPin);
		}
	}

	// Save the Widget Blueprint
	WidgetBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("binding_name"), BindingName);
	return Response;
}


TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCheckComponentExists(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}

	// Load the Widget Blueprint using our helper function
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Check if the component exists
	UWidget* ExistingWidget = WidgetBlueprint->WidgetTree->FindWidget(*ComponentName);
	bool bExists = (ExistingWidget != nullptr);

	// Create response
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("component_name"), ComponentName);
	Response->SetBoolField(TEXT("exists"), bExists);
	
	if (bExists)
	{
		// Include component class name if it exists
		Response->SetStringField(TEXT("component_type"), ExistingWidget->GetClass()->GetName());
	}
	
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetAsChild(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ParentComponentName;
	if (!Params->TryGetStringField(TEXT("parent_component_name"), ParentComponentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_component_name' parameter"));
	}

	FString ChildComponentName;
	if (!Params->TryGetStringField(TEXT("child_component_name"), ChildComponentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'child_component_name' parameter"));
	}

	// Load the Widget Blueprint using our helper function
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Find the child widget
	UWidget* ChildWidget = WidgetBlueprint->WidgetTree->FindWidget(*ChildComponentName);
	if (!ChildWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Child widget '%s' not found"), *ChildComponentName));
	}

	// Find the parent widget
	UWidget* ParentWidget = WidgetBlueprint->WidgetTree->FindWidget(*ParentComponentName);
	
	// Check if we need to create the parent
	bool CreateParentIfMissing = false;
	Params->TryGetBoolField(TEXT("create_parent_if_missing"), CreateParentIfMissing);
	
	if (!ParentWidget && CreateParentIfMissing)
	{
		// Get parent component type
		FString ParentComponentType = TEXT("Border");
		Params->TryGetStringField(TEXT("parent_component_type"), ParentComponentType);
		
		// Get position and size for new parent
		FVector2D Position(0.0f, 0.0f);
		if (Params->HasField(TEXT("parent_position")))
		{
			const TArray<TSharedPtr<FJsonValue>>* PosArray;
			if (Params->TryGetArrayField(TEXT("parent_position"), PosArray) && PosArray->Num() >= 2)
			{
				Position.X = (*PosArray)[0]->AsNumber();
				Position.Y = (*PosArray)[1]->AsNumber();
			}
		}
		
		FVector2D Size(300.0f, 200.0f);
		if (Params->HasField(TEXT("parent_size")))
		{
			const TArray<TSharedPtr<FJsonValue>>* SizeArray;
			if (Params->TryGetArrayField(TEXT("parent_size"), SizeArray) && SizeArray->Num() >= 2)
			{
				Size.X = (*SizeArray)[0]->AsNumber();
				Size.Y = (*SizeArray)[1]->AsNumber();
			}
		}
		
		// Create the parent based on the specified type
		if (ParentComponentType == TEXT("Border"))
		{
			UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *ParentComponentName);
			ParentWidget = Border;
		}
		else if (ParentComponentType == TEXT("VerticalBox"))
		{
			UVerticalBox* VerticalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *ParentComponentName);
			ParentWidget = VerticalBox;
		}
		else if (ParentComponentType == TEXT("HorizontalBox"))
		{
			UHorizontalBox* HorizontalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *ParentComponentName);
			ParentWidget = HorizontalBox;
		}
		else if (ParentComponentType == TEXT("ScrollBox"))
		{
			UScrollBox* ScrollBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), *ParentComponentName);
			ParentWidget = ScrollBox;
		}
		else if (ParentComponentType == TEXT("WidgetSwitcher"))
		{
			UWidgetSwitcher* Switcher = WidgetBlueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), *ParentComponentName);
			ParentWidget = Switcher;
		}
		else if (ParentComponentType == TEXT("Overlay"))
		{
			UOverlay* Overlay = WidgetBlueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *ParentComponentName);
			ParentWidget = Overlay;
		}
		else if (ParentComponentType == TEXT("SizeBox"))
		{
			USizeBox* SizeBox = WidgetBlueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *ParentComponentName);
			ParentWidget = SizeBox;
		}
		else
		{
			// Default to a Border if the parent component type is not recognized
			UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *ParentComponentName);
			ParentWidget = Border;
		}
		
		// If we created a new parent widget, add it to the root canvas panel
		if (ParentWidget)
		{
			UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
			if (RootCanvas)
			{
				UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(ParentWidget);
				if (PanelSlot)
				{
					PanelSlot->SetPosition(Position);
					PanelSlot->SetSize(Size);
				}
			}
		}
	}
	
	// Check that we have a valid parent widget now
	if (!ParentWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent widget '%s' not found and wasn't created"), *ParentComponentName));
	}
	
	// Check for circular reference - IMPORTANT: This prevents the recursion depth error
	// Check if the parent is a descendant of the child (which would create a cycle)
	UWidget* TestWidget = ParentWidget;
	while (TestWidget)
	{
		if (TestWidget == ChildWidget)
		{
			// Circular reference detected
			return FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Cannot add '%s' as child of '%s' because it would create a circular reference"),
				*ChildComponentName, *ParentComponentName));
		}
		
		UPanelWidget* TestParent = TestWidget->GetParent();
		if (TestParent)
		{
			TestWidget = TestParent;
		}
		else
		{
			break;
		}
	}

	// Check for single-child widget limitations
	UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
	if (ParentPanel)
	{
		// Check if this is a single-child widget type
		bool bIsSingleChildWidget = ParentWidget->IsA<UBorder>() || 
								  ParentWidget->IsA<USizeBox>() || 
								  ParentWidget->IsA<UScaleBox>() || 
								  ParentWidget->IsA<UBackgroundBlur>() ||
								  ParentWidget->IsA<USafeZone>() ||
								  ParentWidget->IsA<UNamedSlot>() ||
								  ParentWidget->IsA<URetainerBox>() ||
								  ParentWidget->IsA<UWindowTitleBarArea>();

		if (bIsSingleChildWidget && ParentPanel->GetChildrenCount() > 0)
		{
			// Get the widget class name without the 'U' prefix
			FString WidgetClassName = ParentWidget->GetClass()->GetName();
			if (WidgetClassName.StartsWith(TEXT("U")))
			{
				WidgetClassName = WidgetClassName.RightChop(1);
			}

			return FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Cannot add child '%s' to '%s' because %s widgets can only have one child, and it already has a child widget"),
				*ChildComponentName, *ParentComponentName, *WidgetClassName));
		}
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent widget '%s' is not a panel widget that can have children"), *ParentComponentName));
	}
	
	// Get child's current parent if any
	UPanelWidget* CurrentParent = ChildWidget->GetParent();
	if (CurrentParent)
	{
		// Remove from current parent first (this prevents crashes)
		CurrentParent->RemoveChild(ChildWidget);
	}
	
	// Add child to the new parent
	ParentPanel->AddChild(ChildWidget);
		
	// Save the Widget Blueprint
	WidgetBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);
		
	// Create success response
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("parent_component_name"), ParentComponentName);
	Response->SetStringField(TEXT("child_component_name"), ChildComponentName);
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleCreateWidgetComponentWithChild(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ParentComponentName;
	if (!Params->TryGetStringField(TEXT("parent_component_name"), ParentComponentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_component_name' parameter"));
	}

	FString ChildComponentName;
	if (!Params->TryGetStringField(TEXT("child_component_name"), ChildComponentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'child_component_name' parameter"));
	}

	// Load the Widget Blueprint using our helper function
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get parent and child component types
	FString ParentComponentType = TEXT("Border");
	Params->TryGetStringField(TEXT("parent_component_type"), ParentComponentType);
	
	FString ChildComponentType = TEXT("TextBlock");
	Params->TryGetStringField(TEXT("child_component_type"), ChildComponentType);
	
	// Get position and size for parent
	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("parent_position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("parent_position"), PosArray) && PosArray->Num() >= 2)
		{
			Position.X = (*PosArray)[0]->AsNumber();
			Position.Y = (*PosArray)[1]->AsNumber();
		}
	}
	
	FVector2D Size(300.0f, 200.0f);
	if (Params->HasField(TEXT("parent_size")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SizeArray;
		if (Params->TryGetArrayField(TEXT("parent_size"), SizeArray) && SizeArray->Num() >= 2)
		{
			Size.X = (*SizeArray)[0]->AsNumber();
			Size.Y = (*SizeArray)[1]->AsNumber();
		}
	}
	
	// Check if any components with these names already exist
	UWidget* ExistingParent = WidgetBlueprint->WidgetTree->FindWidget(*ParentComponentName);
	if (ExistingParent)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("A widget named '%s' already exists"), *ParentComponentName));
	}
	
	UWidget* ExistingChild = WidgetBlueprint->WidgetTree->FindWidget(*ChildComponentName);
	if (ExistingChild)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("A widget named '%s' already exists"), *ChildComponentName));
	}
	
	// Create the parent widget based on the specified type
	UPanelWidget* ParentWidget = nullptr;
	
	if (ParentComponentType == TEXT("Border"))
	{
		UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *ParentComponentName);
		ParentWidget = Border;
	}
	else if (ParentComponentType == TEXT("VerticalBox"))
	{
		UVerticalBox* VerticalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *ParentComponentName);
		ParentWidget = VerticalBox;
	}
	else if (ParentComponentType == TEXT("HorizontalBox"))
	{
		UHorizontalBox* HorizontalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *ParentComponentName);
		ParentWidget = HorizontalBox;
	}
	else if (ParentComponentType == TEXT("ScrollBox"))
	{
		UScrollBox* ScrollBox = WidgetBlueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), *ParentComponentName);
		ParentWidget = ScrollBox;
	}
	else if (ParentComponentType == TEXT("WidgetSwitcher"))
	{
		UWidgetSwitcher* Switcher = WidgetBlueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), *ParentComponentName);
		ParentWidget = Switcher;
	}
	else if (ParentComponentType == TEXT("Overlay"))
	{
		UOverlay* Overlay = WidgetBlueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *ParentComponentName);
		ParentWidget = Overlay;
	}
	else if (ParentComponentType == TEXT("SizeBox"))
	{
		USizeBox* SizeBox = WidgetBlueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *ParentComponentName);
		ParentWidget = SizeBox;
	}
	else
	{
		// Default to a Border if the parent component type is not recognized
		UBorder* Border = WidgetBlueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *ParentComponentName);
		ParentWidget = Border;
	}
	
	// Create the child widget based on the specified type
	UWidget* ChildWidget = nullptr;
	
	if (ChildComponentType == TEXT("TextBlock"))
	{
		UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *ChildComponentName);
		ChildWidget = TextBlock;
		
		// Check for text attribute
		const TSharedPtr<FJsonObject>* ChildAttributes;
		if (Params->TryGetObjectField(TEXT("child_attributes"), ChildAttributes))
		{
			FString Text;
			if ((*ChildAttributes)->TryGetStringField(TEXT("text"), Text))
			{
				TextBlock->SetText(FText::FromString(Text));
			}
			
			int32 FontSize = 12;
			(*ChildAttributes)->TryGetNumberField(TEXT("font_size"), FontSize);
			
			// Set font size property if provided
			// In UE 5.5, we can't directly set the font size with a simple method,
			// but for a complete implementation we would need to use FSlateFontInfo
			if (FontSize > 0)
			{
				// Apply scaling factor to convert from desired visual size to internal font size
				// Unreal applies a 4/3 (1.3333) multiplier, so we multiply by 4/3 to get the required size
				const float UE_FONT_SCALE_FACTOR = 4.0f / 3.0f; // 1.3333...
				int32 ScaledFontSize = FMath::RoundToInt(FontSize * UE_FONT_SCALE_FACTOR);
				
				// Create a completely new font info instead of modifying existing
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
			}
		}
	}
	else if (ChildComponentType == TEXT("Button"))
	{
		UButton* Button = WidgetBlueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *ChildComponentName);
		ChildWidget = Button;
		
		// Check if we need to add text to the button
		const TSharedPtr<FJsonObject>* ChildAttributes;
		if (Params->TryGetObjectField(TEXT("child_attributes"), ChildAttributes))
		{
			FString ButtonText;
			if ((*ChildAttributes)->TryGetStringField(TEXT("text"), ButtonText))
			{
				// Create text block for the button
				UTextBlock* ButtonTextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(
					UTextBlock::StaticClass(), 
					*(ChildComponentName + TEXT("_Text"))
				);
				
				if (ButtonTextBlock)
				{
					ButtonTextBlock->SetText(FText::FromString(ButtonText));
					
					// Apply font size if provided
					int32 FontSize = 12;
					if ((*ChildAttributes)->TryGetNumberField(TEXT("font_size"), FontSize) && FontSize > 0)
					{
						// Apply scaling factor to convert from desired visual size to internal font size
						// Unreal applies a 4/3 (1.3333) multiplier, so we multiply by 4/3 to get the required size
						const float UE_FONT_SCALE_FACTOR = 4.0f / 3.0f; // 1.3333...
						int32 ScaledFontSize = FMath::RoundToInt(FontSize * UE_FONT_SCALE_FACTOR);
						
						// Create a completely new font info instead of modifying existing
						FSlateFontInfo CurrentFont = ButtonTextBlock->GetFont();
						FSlateFontInfo NewFontInfo(
							CurrentFont.FontObject,
							ScaledFontSize,  // Use the scaled size to get the desired visual size
							CurrentFont.TypefaceFontName
						);
						
						// Preserve any other necessary font properties
						NewFontInfo.FontMaterial = CurrentFont.FontMaterial;
						NewFontInfo.OutlineSettings = CurrentFont.OutlineSettings;
						
						// Apply the font
						ButtonTextBlock->SetFont(NewFontInfo);
					}
					
					Button->AddChild(ButtonTextBlock);
				}
			}
		}
	}
	else if (ChildComponentType == TEXT("Image"))
	{
		UImage* Image = WidgetBlueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *ChildComponentName);
		ChildWidget = Image;
		
		// Check for brush asset path
		const TSharedPtr<FJsonObject>* ChildAttributes;
		if (Params->TryGetObjectField(TEXT("child_attributes"), ChildAttributes))
		{
			FString BrushAssetPath;
			if ((*ChildAttributes)->TryGetStringField(TEXT("brush_asset_path"), BrushAssetPath) && !BrushAssetPath.IsEmpty())
			{
				UObject* BrushAsset = UEditorAssetLibrary::LoadAsset(BrushAssetPath);
				if (BrushAsset && BrushAsset->IsA<UTexture2D>())
				{
					UTexture2D* Texture = Cast<UTexture2D>(BrushAsset);
					FSlateBrush Brush;
					Brush.SetResourceObject(Texture);
					Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
					Image->SetBrush(Brush);
				}
			}
		}
	}
	else if (ChildComponentType == TEXT("VerticalBox"))
	{
		UVerticalBox* VerticalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *ChildComponentName);
		ChildWidget = VerticalBox;
	}
	else if (ChildComponentType == TEXT("HorizontalBox"))
	{
		UHorizontalBox* HorizontalBox = WidgetBlueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *ChildComponentName);
		ChildWidget = HorizontalBox;
	}
	else
	{
		// Default to a TextBlock if the child component type is not recognized or implemented
		UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *ChildComponentName);
		ChildWidget = TextBlock;
		TextBlock->SetText(FText::FromString(TEXT("Default Text")));
	}
	
	// Add the parent to the root canvas panel
	if (ParentWidget)
	{
		UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
		if (RootCanvas)
		{
			UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(ParentWidget);
			if (PanelSlot)
			{
				PanelSlot->SetPosition(Position);
				PanelSlot->SetSize(Size);
			}
		}
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create parent widget"));
	}
	
	// Add the child to the parent
	if (ChildWidget && ParentWidget)
	{
		ParentWidget->AddChild(ChildWidget);
		
		// Save the Widget Blueprint
		WidgetBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);
		
		// Create success response
		Response->SetBoolField(TEXT("success"), true);
		Response->SetStringField(TEXT("parent_component_name"), ParentComponentName);
		Response->SetStringField(TEXT("child_component_name"), ChildComponentName);
		Response->SetStringField(TEXT("parent_component_type"), ParentComponentType);
		Response->SetStringField(TEXT("child_component_type"), ChildComponentType);
		return Response;
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create child widget or add it to parent"));
	}
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetPlacement(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: widget_name"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: component_name"));
    }

    // Find the widget blueprint
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetName);
    if (!WidgetBlueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(*FString::Printf(TEXT("Widget blueprint not found: %s"), *WidgetName));
    }

    // Find the specified widget
    UWidget* TargetWidget = nullptr;
    
    // Only search if we have a valid widget tree
    if (WidgetBlueprint->WidgetTree)
    {
        WidgetBlueprint->WidgetTree->ForEachWidget([&TargetWidget, &ComponentName](UWidget* Widget) {
            if (Widget && Widget->GetName() == ComponentName)
            {
                TargetWidget = Widget;
                // Early out once we've found it
                return false;
            }
            return true;
        });
    }

    if (!TargetWidget)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(*FString::Printf(TEXT("Widget component not found: %s"), *ComponentName));
    }

    // Get the parent slot (likely a Canvas Panel Slot)
    UPanelSlot* ParentSlot = TargetWidget->Slot;
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ParentSlot);
    
    if (!CanvasSlot)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget is not in a Canvas Panel or doesn't have a valid slot"));
    }

    // Get optional parameters
    const TArray<TSharedPtr<FJsonValue>>* PositionArray;
    if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() == 2)
    {
        float X = (*PositionArray)[0]->AsNumber();
        float Y = (*PositionArray)[1]->AsNumber();
        CanvasSlot->SetPosition(FVector2D(X, Y));
    }

    const TArray<TSharedPtr<FJsonValue>>* SizeArray;
    if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() == 2)
    {
        float Width = (*SizeArray)[0]->AsNumber();
        float Height = (*SizeArray)[1]->AsNumber();
        CanvasSlot->SetSize(FVector2D(Width, Height));
    }

    const TArray<TSharedPtr<FJsonValue>>* AlignmentArray;
    if (Params->TryGetArrayField(TEXT("alignment"), AlignmentArray) && AlignmentArray->Num() == 2)
    {
        float HorizontalAlignment = (*AlignmentArray)[0]->AsNumber();
        float VerticalAlignment = (*AlignmentArray)[1]->AsNumber();
        CanvasSlot->SetAlignment(FVector2D(HorizontalAlignment, VerticalAlignment));
    }

    // Save the Widget Blueprint
    WidgetBlueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UEditorAssetLibrary::SaveAsset(WidgetBlueprint->GetPathName(), false);

    // Create success response with current values
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("widget_name"), ComponentName);

    // Include current position in response
    TArray<TSharedPtr<FJsonValue>> CurrentPosition;
    CurrentPosition.Add(MakeShared<FJsonValueNumber>(CanvasSlot->GetPosition().X));
    CurrentPosition.Add(MakeShared<FJsonValueNumber>(CanvasSlot->GetPosition().Y));
    Response->SetArrayField(TEXT("position"), CurrentPosition);

    // Include current size in response
    TArray<TSharedPtr<FJsonValue>> CurrentSize;
    CurrentSize.Add(MakeShared<FJsonValueNumber>(CanvasSlot->GetSize().X));
    CurrentSize.Add(MakeShared<FJsonValueNumber>(CanvasSlot->GetSize().Y));
    Response->SetArrayField(TEXT("size"), CurrentSize);

    // Include current alignment in response
    TArray<TSharedPtr<FJsonValue>> CurrentAlignment;
    CurrentAlignment.Add(MakeShared<FJsonValueNumber>(CanvasSlot->GetAlignment().X));
    CurrentAlignment.Add(MakeShared<FJsonValueNumber>(CanvasSlot->GetAlignment().Y));
    Response->SetArrayField(TEXT("alignment"), CurrentAlignment);

    return Response;
}

// Helper function to create error response
TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage)
{
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), false);
    Response->SetStringField(TEXT("error"), ErrorMessage);
    return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetContainerDimensions(const TSharedPtr<FJsonObject>& InJson)
{
    // Get the widget name and container name from the input JSON
    FString WidgetName;
    FString ContainerName;
    if (!InJson->TryGetStringField(TEXT("widget_name"), WidgetName) ||
        !InJson->TryGetStringField(TEXT("container_name"), ContainerName))
    {
        return MakeShared<FJsonObject>();
    }

    // Find the widget blueprint
    UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(WidgetName);
    if (!WidgetBP || !WidgetBP->WidgetTree)
    {
        return CreateErrorResponse(TEXT("Widget not found"));
    }

    // Find the container widget
    UWidget* ContainerWidget = WidgetBP->WidgetTree->FindWidget(FName(*ContainerName));
    if (!ContainerWidget)
    {
        return CreateErrorResponse(TEXT("Container widget not found: ") + ContainerName);
    }

    // Create response JSON
    TSharedPtr<FJsonObject> ResponseJson = MakeShared<FJsonObject>();
    
    // Force layout prepass to ensure sizes are computed
    ContainerWidget->ForceLayoutPrepass();

    // Get the desired size of the container
    FVector2D DesiredSize = ContainerWidget->GetDesiredSize();
    
    // Try to get dimensions from canvas panel slot
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ContainerWidget->Slot))
    {
        FVector2D Position = CanvasSlot->GetPosition();
        FVector2D Size = CanvasSlot->GetSize();
        
        ResponseJson->SetNumberField(TEXT("x"), Position.X);
        ResponseJson->SetNumberField(TEXT("y"), Position.Y);
        ResponseJson->SetNumberField(TEXT("width"), Size.X);
        ResponseJson->SetNumberField(TEXT("height"), Size.Y);
        ResponseJson->SetBoolField(TEXT("from_slot"), true);
        
        // Also include the desired size
        ResponseJson->SetNumberField(TEXT("desired_width"), DesiredSize.X);
        ResponseJson->SetNumberField(TEXT("desired_height"), DesiredSize.Y);
    }
    else
    {
        // No canvas slot, use desired size
        ResponseJson->SetNumberField(TEXT("x"), 0);
        ResponseJson->SetNumberField(TEXT("y"), 0);
        ResponseJson->SetNumberField(TEXT("width"), DesiredSize.X);
        ResponseJson->SetNumberField(TEXT("height"), DesiredSize.Y);
        ResponseJson->SetBoolField(TEXT("from_slot"), false);
    }

    // Get parent widget dimensions
    UWidget* ParentWidget = ContainerWidget->GetParent();
    if (ParentWidget)
    {
        // Force layout prepass on parent
        ParentWidget->ForceLayoutPrepass();
        FVector2D ParentSize = ParentWidget->GetDesiredSize();
        
        // If parent is a canvas panel, use its actual size
        if (UCanvasPanel* ParentCanvas = Cast<UCanvasPanel>(ParentWidget))
        {
            if (UCanvasPanelSlot* ParentSlot = Cast<UCanvasPanelSlot>(ParentCanvas->Slot))
            {
                ParentSize = ParentSlot->GetSize();
            }
        }
        
        ResponseJson->SetNumberField(TEXT("parent_width"), ParentSize.X);
        ResponseJson->SetNumberField(TEXT("parent_height"), ParentSize.Y);
        ResponseJson->SetBoolField(TEXT("has_parent_size"), true);
    }
    else 
    {
        ResponseJson->SetBoolField(TEXT("has_parent_size"), false);
    }

    // Get viewport size if this is the root widget
    if (ContainerName.Equals(TEXT("RootCanvas")))
    {
        if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
        {
            FIntPoint ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
            ResponseJson->SetNumberField(TEXT("viewport_width"), ViewportSize.X);
            ResponseJson->SetNumberField(TEXT("viewport_height"), ViewportSize.Y);
            ResponseJson->SetBoolField(TEXT("has_viewport_size"), true);
        }
        else
        {
            ResponseJson->SetBoolField(TEXT("has_viewport_size"), false);
        }
    }

    ResponseJson->SetBoolField(TEXT("success"), true);
    return ResponseJson;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleAddWidgetComponent(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}

	FString ComponentType;
	if (!Params->TryGetStringField(TEXT("component_type"), ComponentType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_type' parameter"));
	}

	// Find the Widget Blueprint
	UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(BlueprintName);
	if (!WidgetBlueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get position parameter if it exists
	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		TArray<TSharedPtr<FJsonValue>> PosArray;
		if (GetJsonArray(Params, TEXT("position"), PosArray) && PosArray.Num() >= 2)
		{
			Position.X = PosArray[0]->AsNumber();
			Position.Y = PosArray[1]->AsNumber();
		}
	}

	// Get size parameter if it exists
	FVector2D Size(100.0f, 100.0f);
	if (Params->HasField(TEXT("size")))
	{
		TArray<TSharedPtr<FJsonValue>> SizeArray;
		if (GetJsonArray(Params, TEXT("size"), SizeArray) && SizeArray.Num() >= 2)
		{
			Size.X = SizeArray[0]->AsNumber();
			Size.Y = SizeArray[1]->AsNumber();
		}
	}

	// Get kwargs object if it exists
	const TSharedPtr<FJsonValue>* KwargsJsonPtr = Params->Values.Find(TEXT("kwargs"));
	if (!KwargsJsonPtr || (*KwargsJsonPtr)->Type != EJson::Object) {
		UE_LOG(LogUnrealMCPUMG, Error, TEXT("Missing or invalid 'kwargs' parameter"));
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'kwargs' parameter"));
	}
	TSharedPtr<FJsonObject> KwargsObject = (*KwargsJsonPtr)->AsObject();

	// Use WidgetComponentService to create the widget
	FWidgetComponentService WidgetComponentService;
	UWidget* CreatedWidget = WidgetComponentService.CreateWidgetComponent(
		WidgetBlueprint, 
		ComponentName, 
		ComponentType, 
		Position, 
		Size, 
		KwargsObject
	);

	// Make sure widget was created
	if (!CreatedWidget)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create widget component"));
	}

	// Add to canvas panel
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Root Canvas Panel not found"));
	}

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(CreatedWidget);
	PanelSlot->SetPosition(Position);
	PanelSlot->SetSize(Size);

	// Mark the package dirty and compile
	WidgetBlueprint->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);

	// Build success response
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("component_name"), ComponentName);
	Response->SetStringField(TEXT("component_type"), ComponentType);
	
	TArray<TSharedPtr<FJsonValue>> PosArray;
	PosArray.Add(MakeShared<FJsonValueNumber>(Position.X));
	PosArray.Add(MakeShared<FJsonValueNumber>(Position.Y));
	Response->SetArrayField(TEXT("position"), PosArray);
	
	TArray<TSharedPtr<FJsonValue>> SizeArray;
	SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
	SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
	Response->SetArrayField(TEXT("size"), SizeArray);

	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleSetWidgetComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Log the entire parameters object for debugging
    FString ParamsString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsString);
    FJsonSerializer::Serialize(Params.ToSharedRef(), Writer);
    UE_LOG(LogUnrealMCPUMG, Log, TEXT("HandleSetWidgetComponentProperty - Raw Input: %s"), *ParamsString);

    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        UE_LOG(LogUnrealMCPUMG, Error, TEXT("Missing parameter: widget_name"));
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing parameter: widget_name"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        UE_LOG(LogUnrealMCPUMG, Error, TEXT("Missing parameter: component_name"));
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing parameter: component_name"));
    }

    // Fetch 'kwargs' from params
    const TSharedPtr<FJsonValue>* KwargsJsonPtr = Params->Values.Find(TEXT("kwargs"));
    if (!KwargsJsonPtr || (*KwargsJsonPtr)->Type != EJson::Object) {
        UE_LOG(LogUnrealMCPUMG, Error, TEXT("Missing or invalid 'kwargs' parameter"));
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'kwargs' parameter"));
    }
    TSharedPtr<FJsonObject> KwargsObject = (*KwargsJsonPtr)->AsObject();

    // Find the widget blueprint and component
    UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(WidgetName);
    if (!WidgetBP || !WidgetBP->WidgetTree) {
        return FUnrealMCPCommonUtils::CreateErrorResponse(*FString::Printf(TEXT("Widget Blueprint not found or has no WidgetTree: %s"), *WidgetName));
    }
    UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*ComponentName));
    if (!TargetWidget) {
        return FUnrealMCPCommonUtils::CreateErrorResponse(*FString::Printf(TEXT("Component '%s' not found in Widget Blueprint '%s'"), *ComponentName, *WidgetName));
    }

    TArray<FString> PropertyNames;
    KwargsObject->Values.GetKeys(PropertyNames);

    TArray<FString> SuccessProps;
    TMap<FString, FString> FailedProps;

    for (const FString& PropertyName : PropertyNames) {
        TSharedPtr<FJsonValue> PropertyValue = KwargsObject->TryGetField(PropertyName);
        FProperty* Property = FindFProperty<FProperty>(TargetWidget->GetClass(), FName(*PropertyName));
        if (!Property) {
            FailedProps.Add(PropertyName, FString::Printf(TEXT("Property '%s' not found on component '%s' (Class: %s)"), *PropertyName, *ComponentName, *TargetWidget->GetClass()->GetName()));
            continue;
        }
        void* PropertyData = Property->ContainerPtrToValuePtr<void>(TargetWidget);
        FString ErrorMessage;
        if (!FUnrealMCPCommonUtils::SetPropertyFromJson(Property, PropertyData, PropertyValue)) {
            FailedProps.Add(PropertyName, FString::Printf(TEXT("Failed to set property '%s'. Check value type/format."), *PropertyName));
            continue;
        }
        SuccessProps.Add(PropertyName);
    }

    // If any property was set, mark blueprint as modified and compile
    if (SuccessProps.Num() > 0) {
        FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
        FKismetEditorUtilities::CompileBlueprint(WidgetBP);
    }

    // Build response
    TSharedPtr<FJsonObject> Response = FUnrealMCPCommonUtils::CreateSuccessResponse();
    Response->SetStringField(TEXT("widget_name"), WidgetName);
    Response->SetStringField(TEXT("component_name"), ComponentName);

    TArray<TSharedPtr<FJsonValue>> SuccessArray;
    for (const FString& Prop : SuccessProps) {
        SuccessArray.Add(MakeShared<FJsonValueString>(Prop));
    }
    Response->SetArrayField(TEXT("success_properties"), SuccessArray);

    TArray<TSharedPtr<FJsonValue>> FailedArray;
    for (const auto& Pair : FailedProps) {
        TSharedPtr<FJsonObject> FailObj = MakeShared<FJsonObject>();
        FailObj->SetStringField(TEXT("property"), Pair.Key);
        FailObj->SetStringField(TEXT("error"), Pair.Value);
        FailedArray.Add(MakeShared<FJsonValueObject>(FailObj));
    }
    Response->SetArrayField(TEXT("failed_properties"), FailedArray);

    // --- SLOT PROPERTY HANDLING FOR UHorizontalBoxSlot ---
    UPanelSlot* PanelSlot = TargetWidget->Slot;
    UHorizontalBoxSlot* HorizontalBoxSlot = Cast<UHorizontalBoxSlot>(PanelSlot);
    if (HorizontalBoxSlot) {
        // Padding
        if (KwargsObject->HasField(TEXT("padding"))) {
            const TArray<TSharedPtr<FJsonValue>>* PaddingArrayPtr;
            if (KwargsObject->TryGetArrayField(TEXT("padding"), PaddingArrayPtr) && PaddingArrayPtr) {
                const TArray<TSharedPtr<FJsonValue>>& PaddingArray = *PaddingArrayPtr;
                if (PaddingArray.Num() == 4) {
                    float Left = PaddingArray[0]->AsNumber();
                    float Top = PaddingArray[1]->AsNumber();
                    float Right = PaddingArray[2]->AsNumber();
                    float Bottom = PaddingArray[3]->AsNumber();
                    HorizontalBoxSlot->SetPadding(FMargin(Left, Top, Right, Bottom));
                } else if (PaddingArray.Num() == 1) {
                    float Pad = PaddingArray[0]->AsNumber();
                    HorizontalBoxSlot->SetPadding(FMargin(Pad));
                }
            }
        }
        // Horizontal Alignment
        if (KwargsObject->HasField(TEXT("horizontal_alignment"))) {
            FString AlignStr = KwargsObject->GetStringField(TEXT("horizontal_alignment"));
            EHorizontalAlignment HAlign = HAlign_Fill;
            if (AlignStr.Equals(TEXT("Left"), ESearchCase::IgnoreCase)) HAlign = HAlign_Left;
            else if (AlignStr.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) HAlign = HAlign_Center;
            else if (AlignStr.Equals(TEXT("Right"), ESearchCase::IgnoreCase)) HAlign = HAlign_Right;
            else if (AlignStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase)) HAlign = HAlign_Fill;
            HorizontalBoxSlot->SetHorizontalAlignment(HAlign);
        }
        // Vertical Alignment
        if (KwargsObject->HasField(TEXT("vertical_alignment"))) {
            FString AlignStr = KwargsObject->GetStringField(TEXT("vertical_alignment"));
            EVerticalAlignment VAlign = VAlign_Fill;
            if (AlignStr.Equals(TEXT("Top"), ESearchCase::IgnoreCase)) VAlign = VAlign_Top;
            else if (AlignStr.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) VAlign = VAlign_Center;
            else if (AlignStr.Equals(TEXT("Bottom"), ESearchCase::IgnoreCase)) VAlign = VAlign_Bottom;
            else if (AlignStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase)) VAlign = VAlign_Fill;
            HorizontalBoxSlot->SetVerticalAlignment(VAlign);
        }
        // Size Rule and Value
        if (KwargsObject->HasField(TEXT("size_rule"))) {
            FString SizeRuleStr = KwargsObject->GetStringField(TEXT("size_rule"));
            ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic;
            if (SizeRuleStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase)) SizeRule = ESlateSizeRule::Fill;
            else if (SizeRuleStr.Equals(TEXT("Auto"), ESearchCase::IgnoreCase)) SizeRule = ESlateSizeRule::Automatic;
            float SizeValue = 1.0f;
            if (KwargsObject->HasField(TEXT("size_value"))) {
                SizeValue = static_cast<float>(KwargsObject->GetNumberField(TEXT("size_value")));
            }
            FSlateChildSize ChildSize(SizeRule);
            ChildSize.Value = SizeValue;
            HorizontalBoxSlot->SetSize(ChildSize);
        }
        // Mark blueprint as modified and compile
        FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
        FKismetEditorUtilities::CompileBlueprint(WidgetBP);
        // Build response
        Response->SetStringField(TEXT("widget_name"), WidgetName);
        Response->SetStringField(TEXT("component_name"), ComponentName);
        Response->SetArrayField(TEXT("success_properties"), {});
        Response->SetArrayField(TEXT("failed_properties"), {});
        return Response;
    }
    // --- SLOT PROPERTY HANDLING FOR UVerticalBoxSlot ---
    UVerticalBoxSlot* VerticalBoxSlot = Cast<UVerticalBoxSlot>(PanelSlot);
    if (VerticalBoxSlot) {
        // Padding
        if (KwargsObject->HasField(TEXT("padding"))) {
            const TArray<TSharedPtr<FJsonValue>>* PaddingArrayPtr;
            if (KwargsObject->TryGetArrayField(TEXT("padding"), PaddingArrayPtr) && PaddingArrayPtr) {
                const TArray<TSharedPtr<FJsonValue>>& PaddingArray = *PaddingArrayPtr;
                if (PaddingArray.Num() == 4) {
                    float Left = PaddingArray[0]->AsNumber();
                    float Top = PaddingArray[1]->AsNumber();
                    float Right = PaddingArray[2]->AsNumber();
                    float Bottom = PaddingArray[3]->AsNumber();
                    VerticalBoxSlot->SetPadding(FMargin(Left, Top, Right, Bottom));
                } else if (PaddingArray.Num() == 1) {
                    float Pad = PaddingArray[0]->AsNumber();
                    VerticalBoxSlot->SetPadding(FMargin(Pad));
                }
            }
        }
        // Vertical Alignment
        if (KwargsObject->HasField(TEXT("vertical_alignment"))) {
            FString AlignStr = KwargsObject->GetStringField(TEXT("vertical_alignment"));
            EVerticalAlignment VAlign = VAlign_Fill;
            if (AlignStr.Equals(TEXT("Top"), ESearchCase::IgnoreCase)) VAlign = VAlign_Top;
            else if (AlignStr.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) VAlign = VAlign_Center;
            else if (AlignStr.Equals(TEXT("Bottom"), ESearchCase::IgnoreCase)) VAlign = VAlign_Bottom;
            else if (AlignStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase)) VAlign = VAlign_Fill;
            VerticalBoxSlot->SetVerticalAlignment(VAlign);
        }
        // Size Rule and Value
        if (KwargsObject->HasField(TEXT("size_rule"))) {
            FString SizeRuleStr = KwargsObject->GetStringField(TEXT("size_rule"));
            ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic;
            if (SizeRuleStr.Equals(TEXT("Fill"), ESearchCase::IgnoreCase)) SizeRule = ESlateSizeRule::Fill;
            else if (SizeRuleStr.Equals(TEXT("Auto"), ESearchCase::IgnoreCase)) SizeRule = ESlateSizeRule::Automatic;
            float SizeValue = 1.0f;
            if (KwargsObject->HasField(TEXT("size_value"))) {
                SizeValue = static_cast<float>(KwargsObject->GetNumberField(TEXT("size_value")));
            }
            FSlateChildSize ChildSize(SizeRule);
            ChildSize.Value = SizeValue;
            VerticalBoxSlot->SetSize(ChildSize);
        }
        // Mark blueprint as modified and compile
        FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
        FKismetEditorUtilities::CompileBlueprint(WidgetBP);
        // Build response
        Response->SetStringField(TEXT("widget_name"), WidgetName);
        Response->SetStringField(TEXT("component_name"), ComponentName);
        Response->SetArrayField(TEXT("success_properties"), {});
        Response->SetArrayField(TEXT("failed_properties"), {});
        return Response;
    }

    return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPUMGCommands::HandleGetWidgetComponentLayout(const TSharedPtr<FJsonObject>& Params)
{
    // Get the widget blueprint name parameter
    FString WidgetBlueprintName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetBlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'widget_name' parameter"));
    }

    // Find the widget blueprint
    UWidgetBlueprint* WidgetBlueprint = FindWidgetBlueprint(WidgetBlueprintName);
    if (!WidgetBlueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetBlueprintName)
        );
    }

    // Check if the WidgetTree exists
    if (!WidgetBlueprint->WidgetTree)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' has no WidgetTree"), *WidgetBlueprintName)
        );
    }

    // Create response object
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetBoolField(TEXT("success"), true);
    
    // Helper function to handle Slot properties differently based on type
    auto ProcessWidgetSlot = [](UWidget* Widget) -> TSharedPtr<FJsonObject> {
        TSharedPtr<FJsonObject> SlotProperties = MakeShared<FJsonObject>();

        if (!Widget || !Widget->Slot)
        {
            return SlotProperties;
        }

        // Try each slot type and extract relevant properties
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            // Position
            FVector2D Position = CanvasSlot->GetPosition();
            TArray<TSharedPtr<FJsonValue>> PositionArray;
            PositionArray.Add(MakeShared<FJsonValueNumber>(Position.X));
            PositionArray.Add(MakeShared<FJsonValueNumber>(Position.Y));
            SlotProperties->SetArrayField(TEXT("position"), PositionArray);

            // Size
            FVector2D Size = CanvasSlot->GetSize();
            TArray<TSharedPtr<FJsonValue>> SizeArray;
            SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
            SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
            SlotProperties->SetArrayField(TEXT("size"), SizeArray);

            // Alignment
            FVector2D Alignment = CanvasSlot->GetAlignment();
            TArray<TSharedPtr<FJsonValue>> AlignmentArray;
            AlignmentArray.Add(MakeShared<FJsonValueNumber>(Alignment.X));
            AlignmentArray.Add(MakeShared<FJsonValueNumber>(Alignment.Y));
            SlotProperties->SetArrayField(TEXT("alignment"), AlignmentArray);

            // Z-Order
            SlotProperties->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
        }
        else if (UVerticalBoxSlot* VerticalBoxSlot = Cast<UVerticalBoxSlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = VerticalBoxSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Vertical Alignment
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)VerticalBoxSlot->GetVerticalAlignment()));

            // Size
            SlotProperties->SetStringField(TEXT("size_rule"), 
                StaticEnum<ESlateSizeRule::Type>()->GetNameStringByValue((int64)VerticalBoxSlot->GetSize().SizeRule));
            SlotProperties->SetNumberField(TEXT("size_value"), VerticalBoxSlot->GetSize().Value);
        }
        else if (UHorizontalBoxSlot* HorizontalBoxSlot = Cast<UHorizontalBoxSlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = HorizontalBoxSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Horizontal Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)HorizontalBoxSlot->GetHorizontalAlignment()));

            // Size
            SlotProperties->SetStringField(TEXT("size_rule"), 
                StaticEnum<ESlateSizeRule::Type>()->GetNameStringByValue((int64)HorizontalBoxSlot->GetSize().SizeRule));
            SlotProperties->SetNumberField(TEXT("size_value"), HorizontalBoxSlot->GetSize().Value);
        }
        else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = BorderSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)BorderSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)BorderSlot->GetVerticalAlignment()));
        }
        else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = OverlaySlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)OverlaySlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)OverlaySlot->GetVerticalAlignment()));
        }
        else if (UGridSlot* GridSlot = Cast<UGridSlot>(Widget->Slot))
        {
            // Row and Column
            SlotProperties->SetNumberField(TEXT("row"), GridSlot->GetRow());
            SlotProperties->SetNumberField(TEXT("column"), GridSlot->GetColumn());
            
            // Row and Column Span
            SlotProperties->SetNumberField(TEXT("row_span"), GridSlot->GetRowSpan());
            SlotProperties->SetNumberField(TEXT("column_span"), GridSlot->GetColumnSpan());
            
            // Padding
            FMargin Padding = GridSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)GridSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)GridSlot->GetVerticalAlignment()));
        }
        else if (UUniformGridSlot* UniformGridSlot = Cast<UUniformGridSlot>(Widget->Slot))
        {
            // Row and Column
            SlotProperties->SetNumberField(TEXT("row"), UniformGridSlot->GetRow());
            SlotProperties->SetNumberField(TEXT("column"), UniformGridSlot->GetColumn());
            
            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)UniformGridSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)UniformGridSlot->GetVerticalAlignment()));
        }
        else if (UWrapBoxSlot* WrapBoxSlot = Cast<UWrapBoxSlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = WrapBoxSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);
            
            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)WrapBoxSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)WrapBoxSlot->GetVerticalAlignment()));
        }
        else if (UScrollBoxSlot* ScrollBoxSlot = Cast<UScrollBoxSlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = ScrollBoxSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)ScrollBoxSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)ScrollBoxSlot->GetVerticalAlignment()));
        }
        else if (UWidgetSwitcherSlot* SwitcherSlot = Cast<UWidgetSwitcherSlot>(Widget->Slot))
        {
            // Padding
            FMargin Padding = SwitcherSlot->GetPadding();
            TArray<TSharedPtr<FJsonValue>> PaddingArray;
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Left));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Top));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Right));
            PaddingArray.Add(MakeShared<FJsonValueNumber>(Padding.Bottom));
            SlotProperties->SetArrayField(TEXT("padding"), PaddingArray);

            // Horizontal and Vertical Alignment
            SlotProperties->SetStringField(TEXT("horizontal_alignment"), 
                StaticEnum<EHorizontalAlignment>()->GetNameStringByValue((int64)SwitcherSlot->GetHorizontalAlignment()));
            SlotProperties->SetStringField(TEXT("vertical_alignment"), 
                StaticEnum<EVerticalAlignment>()->GetNameStringByValue((int64)SwitcherSlot->GetVerticalAlignment()));
        }
        // Add generic slot info
        SlotProperties->SetStringField(TEXT("slot_type"), Widget->Slot->GetClass()->GetName());

        return SlotProperties;
    };

    // Build a hierarchical representation of the widget tree
    TFunction<TSharedPtr<FJsonObject>(UWidget*)> ProcessWidgetHierarchy;
    ProcessWidgetHierarchy = [&ProcessWidgetHierarchy, &ProcessWidgetSlot](UWidget* Widget) -> TSharedPtr<FJsonObject> {
        if (!Widget)
        {
            return nullptr;
        }

        TSharedPtr<FJsonObject> ComponentObj = MakeShared<FJsonObject>();
        
        // Basic component info
        ComponentObj->SetStringField(TEXT("name"), Widget->GetName());
        ComponentObj->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
        
        // Get slot properties
        ComponentObj->SetObjectField(TEXT("slot_properties"), ProcessWidgetSlot(Widget));
        
        // If this is a panel widget, process its children recursively
        UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget);
        if (PanelWidget && PanelWidget->GetChildrenCount() > 0)
        {
            TArray<TSharedPtr<FJsonValue>> ChildrenArray;
            
            for (int32 i = 0; i < PanelWidget->GetChildrenCount(); i++)
            {
                UWidget* ChildWidget = PanelWidget->GetChildAt(i);
                if (ChildWidget)
                {
                    TSharedPtr<FJsonObject> ChildObj = ProcessWidgetHierarchy(ChildWidget);
                    if (ChildObj.IsValid())
                    {
                        ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildObj));
                    }
                }
            }
            
            if (ChildrenArray.Num() > 0)
            {
                ComponentObj->SetArrayField(TEXT("children"), ChildrenArray);
            }
        }
        
        return ComponentObj;
    };

    // Start at the root widget (usually a Canvas Panel)
    UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget;
    TSharedPtr<FJsonObject> RootObj = ProcessWidgetHierarchy(RootWidget);
    
    if (RootObj.IsValid())
    {
        // Set the hierarchy data directly as a JSON object
        TSharedPtr<FJsonObject> HierarchyData = MakeShared<FJsonObject>();
        HierarchyData->SetObjectField(TEXT("root"), RootObj);
        
        // Set data field to the actual JSON object (not serialized string)
        Response->SetObjectField(TEXT("data"), HierarchyData);
        Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully analyzed widget component hierarchy")));
    }
    else
    {
        // Set empty hierarchy data
        TSharedPtr<FJsonObject> EmptyData = MakeShared<FJsonObject>();
        Response->SetObjectField(TEXT("data"), EmptyData);
        Response->SetStringField(TEXT("message"), TEXT("No root widget found in blueprint"));
    }
    
    return Response;
}