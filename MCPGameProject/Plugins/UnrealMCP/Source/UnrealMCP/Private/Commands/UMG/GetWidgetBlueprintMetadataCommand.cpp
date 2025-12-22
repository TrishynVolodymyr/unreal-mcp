#include "Commands/UMG/GetWidgetBlueprintMetadataCommand.h"
#include "Services/UMG/IUMGService.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGetWidgetBlueprintMetadata, Log, All);

FGetWidgetBlueprintMetadataCommand::FGetWidgetBlueprintMetadataCommand(TSharedPtr<IUMGService> InUMGService)
	: UMGService(InUMGService)
{
}

FString FGetWidgetBlueprintMetadataCommand::GetCommandName() const
{
	return TEXT("get_widget_blueprint_metadata");
}

FString FGetWidgetBlueprintMetadataCommand::Execute(const FString& Parameters)
{
	UE_LOG(LogGetWidgetBlueprintMetadata, Log, TEXT("Executing get_widget_blueprint_metadata command"));

	// Parse JSON parameters
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(TEXT("Invalid JSON parameters"));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Validate parameters
	FString ValidationError;
	if (!ValidateParamsInternal(JsonObject, ValidationError))
	{
		TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(ValidationError);
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
		return OutputString;
	}

	// Execute and return result
	TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return OutputString;
}

bool FGetWidgetBlueprintMetadataCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	FString ValidationError;
	return ValidateParamsInternal(JsonObject, ValidationError);
}

bool FGetWidgetBlueprintMetadataCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
	if (!Params.IsValid())
	{
		OutError = TEXT("Invalid JSON parameters");
		return false;
	}

	// Check for widget_name (required)
	if (!Params->HasField(TEXT("widget_name")))
	{
		OutError = TEXT("Missing required parameter: widget_name");
		return false;
	}

	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	if (WidgetName.IsEmpty())
	{
		OutError = TEXT("widget_name cannot be empty");
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	// Parse requested fields (default to "*" if not specified)
	TArray<FString> RequestedFields;
	if (Params->HasField(TEXT("fields")))
	{
		const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
		if (Params->TryGetArrayField(TEXT("fields"), FieldsArray))
		{
			for (const TSharedPtr<FJsonValue>& FieldValue : *FieldsArray)
			{
				FString FieldStr;
				if (FieldValue->TryGetString(FieldStr))
				{
					RequestedFields.Add(FieldStr.ToLower());
				}
			}
		}
	}

	// If no fields specified, default to all
	if (RequestedFields.Num() == 0)
	{
		RequestedFields.Add(TEXT("*"));
	}

	// Optional container name for dimensions
	FString ContainerName = TEXT("CanvasPanel_0");
	if (Params->HasField(TEXT("container_name")))
	{
		ContainerName = Params->GetStringField(TEXT("container_name"));
	}

	// Find the widget blueprint using asset registry
	UWidgetBlueprint* WidgetBlueprint = nullptr;
	TArray<FString> AttemptedPaths;

	// Check if full path provided
	if (WidgetName.StartsWith(TEXT("/Game/")) || WidgetName.StartsWith(TEXT("/Script/")))
	{
		// Try multiple path variations
		TArray<FString> PathVariations;

		if (WidgetName.Contains(TEXT(".")))
		{
			// Path already has asset suffix - try as-is first
			PathVariations.Add(WidgetName);
			// Also try without suffix
			int32 DotIndex;
			if (WidgetName.FindLastChar(TEXT('.'), DotIndex))
			{
				FString PathWithoutSuffix = WidgetName.Left(DotIndex);
				FString AssetName = FPaths::GetBaseFilename(PathWithoutSuffix);
				PathVariations.Add(PathWithoutSuffix + TEXT(".") + AssetName);
			}
		}
		else
		{
			// No suffix - add asset name suffix
			FString AssetName = FPaths::GetBaseFilename(WidgetName);
			PathVariations.Add(WidgetName + TEXT(".") + AssetName);
			PathVariations.Add(WidgetName);
		}

		for (const FString& Path : PathVariations)
		{
			AttemptedPaths.Add(Path);
			UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Attempting to load Widget Blueprint at path: '%s'"), *Path);
			UObject* Asset = UEditorAssetLibrary::LoadAsset(Path);
			WidgetBlueprint = Cast<UWidgetBlueprint>(Asset);
			if (WidgetBlueprint)
			{
				UE_LOG(LogGetWidgetBlueprintMetadata, Log, TEXT("Successfully found Widget Blueprint at: '%s'"), *Path);
				break;
			}
		}
	}

	// If not found by path, search using asset registry
	if (!WidgetBlueprint)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		FARFilter Filter;
		Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
		Filter.PackagePaths.Add(FName(TEXT("/Game")));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssets(Filter, AssetData);

		// Extract just the name for comparison
		FString SearchName = FPaths::GetBaseFilename(WidgetName);
		if (SearchName.IsEmpty())
		{
			SearchName = WidgetName;
		}

		for (const FAssetData& Asset : AssetData)
		{
			if (Asset.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
			{
				FString AssetPath = Asset.GetSoftObjectPath().ToString();
				AttemptedPaths.Add(FString::Printf(TEXT("AssetRegistry:%s"), *AssetPath));
				UE_LOG(LogGetWidgetBlueprintMetadata, Display, TEXT("Found in asset registry, loading: '%s'"), *AssetPath);
				UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
				WidgetBlueprint = Cast<UWidgetBlueprint>(LoadedAsset);
				if (WidgetBlueprint)
				{
					UE_LOG(LogGetWidgetBlueprintMetadata, Log, TEXT("Successfully loaded Widget Blueprint from registry: '%s'"), *AssetPath);
					break;
				}
			}
		}
	}

	if (!WidgetBlueprint)
	{
		FString AttemptedPathsStr = FString::Join(AttemptedPaths, TEXT(", "));
		return CreateErrorResponse(FString::Printf(TEXT("Widget blueprint '%s' not found. Tried paths: [%s]"), *WidgetName, *AttemptedPathsStr));
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget blueprint '%s' has no widget tree"), *WidgetName));
	}

	// Build metadata response
	TSharedPtr<FJsonObject> MetadataObj = MakeShared<FJsonObject>();
	MetadataObj->SetStringField(TEXT("widget_name"), WidgetName);
	MetadataObj->SetStringField(TEXT("asset_path"), WidgetBlueprint->GetPathName());

	// Parent class info
	if (WidgetBlueprint->ParentClass)
	{
		MetadataObj->SetStringField(TEXT("parent_class"), WidgetBlueprint->ParentClass->GetName());
	}

	// Build requested sections
	if (ShouldIncludeField(RequestedFields, TEXT("components")))
	{
		TSharedPtr<FJsonObject> ComponentsInfo = BuildComponentsInfo(WidgetBlueprint);
		if (ComponentsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("components"), ComponentsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("layout")))
	{
		TSharedPtr<FJsonObject> LayoutInfo = BuildLayoutInfo(WidgetBlueprint);
		if (LayoutInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("layout"), LayoutInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("dimensions")))
	{
		TSharedPtr<FJsonObject> DimensionsInfo = BuildDimensionsInfo(WidgetBlueprint, ContainerName);
		if (DimensionsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("dimensions"), DimensionsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("hierarchy")))
	{
		TSharedPtr<FJsonObject> HierarchyInfo = BuildHierarchyInfo(WidgetBlueprint);
		if (HierarchyInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("hierarchy"), HierarchyInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("bindings")))
	{
		TSharedPtr<FJsonObject> BindingsInfo = BuildBindingsInfo(WidgetBlueprint);
		if (BindingsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("bindings"), BindingsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("events")))
	{
		TSharedPtr<FJsonObject> EventsInfo = BuildEventsInfo(WidgetBlueprint);
		if (EventsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("events"), EventsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("variables")))
	{
		TSharedPtr<FJsonObject> VariablesInfo = BuildVariablesInfo(WidgetBlueprint);
		if (VariablesInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("variables"), VariablesInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("functions")))
	{
		TSharedPtr<FJsonObject> FunctionsInfo = BuildFunctionsInfo(WidgetBlueprint);
		if (FunctionsInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("functions"), FunctionsInfo);
		}
	}

	if (ShouldIncludeField(RequestedFields, TEXT("orphaned_nodes")))
	{
		TSharedPtr<FJsonObject> OrphanedNodesInfo = BuildOrphanedNodesInfo(WidgetBlueprint);
		if (OrphanedNodesInfo.IsValid())
		{
			MetadataObj->SetObjectField(TEXT("orphaned_nodes"), OrphanedNodesInfo);
		}
	}

	return CreateSuccessResponse(MetadataObj);
}

bool FGetWidgetBlueprintMetadataCommand::ShouldIncludeField(const TArray<FString>& RequestedFields, const FString& FieldName) const
{
	if (RequestedFields.Contains(TEXT("*")))
	{
		return true;
	}
	return RequestedFields.Contains(FieldName.ToLower());
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildComponentsInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> ComponentsInfo = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> ComponentsList;

	// Collect all widgets
	TArray<UWidget*> AllWidgets;
	if (WidgetBlueprint->WidgetTree->RootWidget)
	{
		CollectAllWidgets(WidgetBlueprint->WidgetTree->RootWidget, AllWidgets);
	}

	for (UWidget* Widget : AllWidgets)
	{
		TSharedPtr<FJsonObject> ComponentObj = MakeShared<FJsonObject>();
		ComponentObj->SetStringField(TEXT("name"), Widget->GetName());
		ComponentObj->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
		ComponentObj->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);
		ComponentObj->SetBoolField(TEXT("is_visible"), Widget->GetVisibility() != ESlateVisibility::Hidden);
		ComponentObj->SetBoolField(TEXT("is_enabled"), Widget->GetIsEnabled());

		// Check if it's a panel (container) widget
		ComponentObj->SetBoolField(TEXT("is_container"), Widget->IsA<UPanelWidget>());

		// Add available delegate events for this widget
		TArray<FString> DelegateEvents = GetAvailableDelegateEvents(Widget);
		if (DelegateEvents.Num() > 0)
		{
			TArray<TSharedPtr<FJsonValue>> EventsArray;
			for (const FString& EventName : DelegateEvents)
			{
				EventsArray.Add(MakeShared<FJsonValueString>(EventName));
			}
			ComponentObj->SetArrayField(TEXT("available_events"), EventsArray);
		}

		ComponentsList.Add(MakeShared<FJsonValueObject>(ComponentObj));
	}

	ComponentsInfo->SetArrayField(TEXT("components"), ComponentsList);
	ComponentsInfo->SetNumberField(TEXT("count"), ComponentsList.Num());

	return ComponentsInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildLayoutInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> LayoutInfo = MakeShared<FJsonObject>();

	if (!WidgetBlueprint->WidgetTree->RootWidget)
	{
		LayoutInfo->SetStringField(TEXT("message"), TEXT("No root widget"));
		return LayoutInfo;
	}

	// Build hierarchical layout (reuse service's BuildWidgetHierarchy logic)
	TSharedPtr<FJsonObject> HierarchyData = BuildWidgetInfo(WidgetBlueprint->WidgetTree->RootWidget);
	if (HierarchyData.IsValid())
	{
		LayoutInfo->SetObjectField(TEXT("root"), HierarchyData);
	}

	return LayoutInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildDimensionsInfo(UWidgetBlueprint* WidgetBlueprint, const FString& ContainerName) const
{
	TSharedPtr<FJsonObject> DimensionsInfo = MakeShared<FJsonObject>();

	FString ActualContainerName = ContainerName.IsEmpty() ? TEXT("CanvasPanel_0") : ContainerName;
	UWidget* Container = WidgetBlueprint->WidgetTree->FindWidget(FName(*ActualContainerName));

	if (!Container)
	{
		// Try root widget
		Container = WidgetBlueprint->WidgetTree->RootWidget;
		if (Container)
		{
			ActualContainerName = Container->GetName();
		}
	}

	if (!Container)
	{
		DimensionsInfo->SetStringField(TEXT("error"), TEXT("No container found"));
		return DimensionsInfo;
	}

	DimensionsInfo->SetStringField(TEXT("container_name"), ActualContainerName);
	DimensionsInfo->SetStringField(TEXT("container_type"), Container->GetClass()->GetName());

	// Default dimensions (canvas panels typically use design-time dimensions)
	if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(Container))
	{
		DimensionsInfo->SetNumberField(TEXT("width"), 1920.0f);
		DimensionsInfo->SetNumberField(TEXT("height"), 1080.0f);
	}
	else
	{
		DimensionsInfo->SetNumberField(TEXT("width"), 800.0f);
		DimensionsInfo->SetNumberField(TEXT("height"), 600.0f);
	}

	return DimensionsInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildHierarchyInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> HierarchyInfo = MakeShared<FJsonObject>();

	if (!WidgetBlueprint->WidgetTree->RootWidget)
	{
		HierarchyInfo->SetStringField(TEXT("message"), TEXT("No root widget"));
		return HierarchyInfo;
	}

	// Build simple hierarchy tree (names and types only, for quick overview)
	TFunction<TSharedPtr<FJsonObject>(UWidget*)> BuildSimpleHierarchy = [&](UWidget* Widget) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> WidgetObj = MakeShared<FJsonObject>();
		WidgetObj->SetStringField(TEXT("name"), Widget->GetName());
		WidgetObj->SetStringField(TEXT("type"), Widget->GetClass()->GetName());

		if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
		{
			TArray<TSharedPtr<FJsonValue>> ChildrenArray;
			for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
			{
				if (UWidget* ChildWidget = PanelWidget->GetChildAt(i))
				{
					TSharedPtr<FJsonObject> ChildObj = BuildSimpleHierarchy(ChildWidget);
					if (ChildObj.IsValid())
					{
						ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildObj));
					}
				}
			}
			WidgetObj->SetArrayField(TEXT("children"), ChildrenArray);
		}

		return WidgetObj;
	};

	TSharedPtr<FJsonObject> RootHierarchy = BuildSimpleHierarchy(WidgetBlueprint->WidgetTree->RootWidget);
	if (RootHierarchy.IsValid())
	{
		HierarchyInfo->SetObjectField(TEXT("root"), RootHierarchy);
	}

	return HierarchyInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildBindingsInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> BindingsInfo = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> BindingsList;

	for (const FDelegateEditorBinding& Binding : WidgetBlueprint->Bindings)
	{
		TSharedPtr<FJsonObject> BindingObj = MakeShared<FJsonObject>();
		BindingObj->SetStringField(TEXT("widget_name"), Binding.ObjectName);
		BindingObj->SetStringField(TEXT("property_name"), Binding.PropertyName.ToString());
		BindingObj->SetStringField(TEXT("function_name"), Binding.FunctionName.ToString());
		BindingObj->SetStringField(TEXT("source_property"), Binding.SourceProperty.ToString());

		// Binding kind
		FString KindStr;
		switch (Binding.Kind)
		{
		case EBindingKind::Function:
			KindStr = TEXT("Function");
			break;
		case EBindingKind::Property:
			KindStr = TEXT("Property");
			break;
		default:
			KindStr = TEXT("Unknown");
		}
		BindingObj->SetStringField(TEXT("kind"), KindStr);

		BindingsList.Add(MakeShared<FJsonValueObject>(BindingObj));
	}

	BindingsInfo->SetArrayField(TEXT("bindings"), BindingsList);
	BindingsInfo->SetNumberField(TEXT("count"), BindingsList.Num());

	return BindingsInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildEventsInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> EventsInfo = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> EventsList;

	// Find all event nodes in the blueprint
	TArray<UK2Node_Event*> AllEventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, AllEventNodes);

	for (UK2Node_Event* EventNode : AllEventNodes)
	{
		TSharedPtr<FJsonObject> EventObj = MakeShared<FJsonObject>();
		EventObj->SetStringField(TEXT("event_name"), EventNode->EventReference.GetMemberName().ToString());
		EventObj->SetStringField(TEXT("custom_function_name"), EventNode->CustomFunctionName.ToString());

		if (UClass* EventClass = EventNode->EventReference.GetMemberParentClass())
		{
			EventObj->SetStringField(TEXT("event_class"), EventClass->GetName());
		}

		EventsList.Add(MakeShared<FJsonValueObject>(EventObj));
	}

	EventsInfo->SetArrayField(TEXT("events"), EventsList);
	EventsInfo->SetNumberField(TEXT("count"), EventsList.Num());

	return EventsInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildVariablesInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> VariablesInfo = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> VariablesList;

	for (const FBPVariableDescription& Variable : WidgetBlueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
		VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());

		if (!Variable.VarType.PinSubCategory.IsNone())
		{
			VarObj->SetStringField(TEXT("sub_type"), Variable.VarType.PinSubCategory.ToString());
		}

		if (Variable.VarType.PinSubCategoryObject.IsValid())
		{
			VarObj->SetStringField(TEXT("object_type"), Variable.VarType.PinSubCategoryObject->GetName());
		}

		VarObj->SetBoolField(TEXT("is_public"), Variable.PropertyFlags & CPF_BlueprintVisible);
		VarObj->SetBoolField(TEXT("is_read_only"), Variable.PropertyFlags & CPF_BlueprintReadOnly);
		VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());

		VariablesList.Add(MakeShared<FJsonValueObject>(VarObj));
	}

	VariablesInfo->SetArrayField(TEXT("variables"), VariablesList);
	VariablesInfo->SetNumberField(TEXT("count"), VariablesList.Num());

	return VariablesInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildFunctionsInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> FunctionsInfo = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> FunctionsList;

	for (UEdGraph* Graph : WidgetBlueprint->FunctionGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
		FuncObj->SetStringField(TEXT("name"), Graph->GetName());

		// Find function entry node for more details
		UK2Node_FunctionEntry* EntryNode = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			EntryNode = Cast<UK2Node_FunctionEntry>(Node);
			if (EntryNode)
			{
				break;
			}
		}

		if (EntryNode)
		{
			FuncObj->SetBoolField(TEXT("is_pure"), (EntryNode->GetFunctionFlags() & FUNC_BlueprintPure) != 0);
			FuncObj->SetBoolField(TEXT("is_const"), (EntryNode->GetFunctionFlags() & FUNC_Const) != 0);
			FuncObj->SetStringField(TEXT("category"), EntryNode->MetaData.Category.ToString());
		}

		// Get function signature from generated class
		if (WidgetBlueprint->GeneratedClass)
		{
			UFunction* Function = WidgetBlueprint->GeneratedClass->FindFunctionByName(Graph->GetFName());
			if (Function)
			{
				TArray<TSharedPtr<FJsonValue>> InputsList;
				TArray<TSharedPtr<FJsonValue>> OutputsList;

				for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
				{
					FProperty* Prop = *PropIt;
					TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
					ParamObj->SetStringField(TEXT("name"), Prop->GetName());
					ParamObj->SetStringField(TEXT("type"), Prop->GetCPPType());

					if (Prop->HasAnyPropertyFlags(CPF_ReturnParm) || Prop->HasAnyPropertyFlags(CPF_OutParm))
					{
						OutputsList.Add(MakeShared<FJsonValueObject>(ParamObj));
					}
					else if (Prop->HasAnyPropertyFlags(CPF_Parm))
					{
						InputsList.Add(MakeShared<FJsonValueObject>(ParamObj));
					}
				}

				FuncObj->SetArrayField(TEXT("inputs"), InputsList);
				FuncObj->SetArrayField(TEXT("outputs"), OutputsList);
			}
		}

		FunctionsList.Add(MakeShared<FJsonValueObject>(FuncObj));
	}

	FunctionsInfo->SetArrayField(TEXT("functions"), FunctionsList);
	FunctionsInfo->SetNumberField(TEXT("count"), FunctionsList.Num());

	return FunctionsInfo;
}

void FGetWidgetBlueprintMetadataCommand::CollectAllWidgets(UWidget* Widget, TArray<UWidget*>& OutWidgets) const
{
	if (!Widget)
	{
		return;
	}

	OutWidgets.Add(Widget);

	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
	{
		for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
		{
			CollectAllWidgets(PanelWidget->GetChildAt(i), OutWidgets);
		}
	}
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildWidgetInfo(UWidget* Widget) const
{
	if (!Widget)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> WidgetInfo = MakeShared<FJsonObject>();

	// Basic info
	WidgetInfo->SetStringField(TEXT("name"), Widget->GetName());
	WidgetInfo->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
	WidgetInfo->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);
	WidgetInfo->SetBoolField(TEXT("is_visible"), Widget->GetVisibility() != ESlateVisibility::Hidden);
	WidgetInfo->SetBoolField(TEXT("is_enabled"), Widget->GetIsEnabled());

	// Slot properties
	if (Widget->Slot)
	{
		TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
		SlotInfo->SetStringField(TEXT("slot_type"), Widget->Slot->GetClass()->GetName());

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			FVector2D Position = CanvasSlot->GetPosition();
			FVector2D Size = CanvasSlot->GetSize();
			FVector2D Alignment = CanvasSlot->GetAlignment();
			FAnchors Anchors = CanvasSlot->GetAnchors();

			TArray<TSharedPtr<FJsonValue>> PositionArray;
			PositionArray.Add(MakeShared<FJsonValueNumber>(Position.X));
			PositionArray.Add(MakeShared<FJsonValueNumber>(Position.Y));
			SlotInfo->SetArrayField(TEXT("position"), PositionArray);

			TArray<TSharedPtr<FJsonValue>> SizeArray;
			SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
			SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
			SlotInfo->SetArrayField(TEXT("size"), SizeArray);

			TArray<TSharedPtr<FJsonValue>> AlignmentArray;
			AlignmentArray.Add(MakeShared<FJsonValueNumber>(Alignment.X));
			AlignmentArray.Add(MakeShared<FJsonValueNumber>(Alignment.Y));
			SlotInfo->SetArrayField(TEXT("alignment"), AlignmentArray);

			TSharedPtr<FJsonObject> AnchorsObj = MakeShared<FJsonObject>();
			AnchorsObj->SetNumberField(TEXT("min_x"), Anchors.Minimum.X);
			AnchorsObj->SetNumberField(TEXT("min_y"), Anchors.Minimum.Y);
			AnchorsObj->SetNumberField(TEXT("max_x"), Anchors.Maximum.X);
			AnchorsObj->SetNumberField(TEXT("max_y"), Anchors.Maximum.Y);
			SlotInfo->SetObjectField(TEXT("anchors"), AnchorsObj);

			SlotInfo->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
		}
		else if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Widget->Slot))
		{
			FMargin Padding = HBoxSlot->GetPadding();
			TSharedPtr<FJsonObject> PaddingObj = MakeShared<FJsonObject>();
			PaddingObj->SetNumberField(TEXT("left"), Padding.Left);
			PaddingObj->SetNumberField(TEXT("top"), Padding.Top);
			PaddingObj->SetNumberField(TEXT("right"), Padding.Right);
			PaddingObj->SetNumberField(TEXT("bottom"), Padding.Bottom);
			SlotInfo->SetObjectField(TEXT("padding"), PaddingObj);

			SlotInfo->SetStringField(TEXT("h_align"), UEnum::GetValueAsString(HBoxSlot->GetHorizontalAlignment()));
			SlotInfo->SetStringField(TEXT("v_align"), UEnum::GetValueAsString(HBoxSlot->GetVerticalAlignment()));
		}
		else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Widget->Slot))
		{
			FMargin Padding = VBoxSlot->GetPadding();
			TSharedPtr<FJsonObject> PaddingObj = MakeShared<FJsonObject>();
			PaddingObj->SetNumberField(TEXT("left"), Padding.Left);
			PaddingObj->SetNumberField(TEXT("top"), Padding.Top);
			PaddingObj->SetNumberField(TEXT("right"), Padding.Right);
			PaddingObj->SetNumberField(TEXT("bottom"), Padding.Bottom);
			SlotInfo->SetObjectField(TEXT("padding"), PaddingObj);

			SlotInfo->SetStringField(TEXT("h_align"), UEnum::GetValueAsString(VBoxSlot->GetHorizontalAlignment()));
			SlotInfo->SetStringField(TEXT("v_align"), UEnum::GetValueAsString(VBoxSlot->GetVerticalAlignment()));
		}

		WidgetInfo->SetObjectField(TEXT("slot"), SlotInfo);
	}

	// Widget type-specific properties
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		TSharedPtr<FJsonObject> TextProps = MakeShared<FJsonObject>();
		TextProps->SetStringField(TEXT("text"), TextBlock->GetText().ToString());
		TextProps->SetNumberField(TEXT("font_size"), TextBlock->GetFont().Size);
		WidgetInfo->SetObjectField(TEXT("text_properties"), TextProps);
	}
	else if (UButton* Button = Cast<UButton>(Widget))
	{
		TSharedPtr<FJsonObject> ButtonProps = MakeShared<FJsonObject>();
		ButtonProps->SetBoolField(TEXT("is_focusable"), Button->GetIsFocusable());
		WidgetInfo->SetObjectField(TEXT("button_properties"), ButtonProps);
	}
	else if (UBorder* Border = Cast<UBorder>(Widget))
	{
		TSharedPtr<FJsonObject> BorderProps = MakeShared<FJsonObject>();
		FLinearColor BrushColor = Border->GetBrushColor();
		BorderProps->SetStringField(TEXT("brush_color"),
			FString::Printf(TEXT("rgba(%d,%d,%d,%.2f)"),
				FMath::RoundToInt(BrushColor.R * 255),
				FMath::RoundToInt(BrushColor.G * 255),
				FMath::RoundToInt(BrushColor.B * 255),
				BrushColor.A));
		WidgetInfo->SetObjectField(TEXT("border_properties"), BorderProps);
	}

	// Children (for panel widgets)
	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
	{
		TArray<TSharedPtr<FJsonValue>> ChildrenArray;
		for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
		{
			if (UWidget* ChildWidget = PanelWidget->GetChildAt(i))
			{
				TSharedPtr<FJsonObject> ChildInfo = BuildWidgetInfo(ChildWidget);
				if (ChildInfo.IsValid())
				{
					ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildInfo));
				}
			}
		}
		WidgetInfo->SetArrayField(TEXT("children"), ChildrenArray);
	}

	return WidgetInfo;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& MetadataObj) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), true);

	// Copy all metadata fields to response
	if (MetadataObj.IsValid())
	{
		for (auto& Pair : MetadataObj->Values)
		{
			ResponseObj->SetField(Pair.Key, Pair.Value);
		}
	}

	return ResponseObj;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
	ResponseObj->SetBoolField(TEXT("success"), false);
	ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
	return ResponseObj;
}

TArray<FString> FGetWidgetBlueprintMetadataCommand::GetAvailableDelegateEvents(UWidget* Widget) const
{
	TArray<FString> DelegateEvents;

	if (!Widget)
	{
		return DelegateEvents;
	}

	// Iterate through all properties of the widget class looking for multicast delegates
	for (TFieldIterator<FProperty> PropIt(Widget->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (!Property)
		{
			continue;
		}

		// Check if it's a multicast delegate property (these are bindable events)
		if (FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Property))
		{
			// Skip internal/private delegates that start with underscore
			FString PropName = Property->GetName();
			if (!PropName.StartsWith(TEXT("_")))
			{
				DelegateEvents.Add(PropName);
			}
		}
	}

	return DelegateEvents;
}

TSharedPtr<FJsonObject> FGetWidgetBlueprintMetadataCommand::BuildOrphanedNodesInfo(UWidgetBlueprint* WidgetBlueprint) const
{
	TSharedPtr<FJsonObject> OrphanedInfo = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> OrphanedNodesList;

	// Collect all graphs from the widget blueprint
	TArray<UEdGraph*> AllGraphs;
	WidgetBlueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			// Check if node has any connected pins
			bool bHasConnection = false;
			bool bHasInputConnection = false;
			bool bHasOutputConnection = false;

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->LinkedTo.Num() > 0)
				{
					bHasConnection = true;

					if (Pin->Direction == EGPD_Input)
					{
						bHasInputConnection = true;
					}
					else if (Pin->Direction == EGPD_Output)
					{
						bHasOutputConnection = true;
					}
				}
			}

			// Node is considered orphaned if it has no connections at all
			bool bIsOrphaned = !bHasConnection;

			// Event/Entry nodes are only orphaned if they have no output connections
			if (Cast<UK2Node_Event>(Node) || Cast<UK2Node_FunctionEntry>(Node))
			{
				bIsOrphaned = !bHasOutputConnection;
			}

			if (bIsOrphaned)
			{
				TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
				NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
				NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
				NodeObj->SetStringField(TEXT("node_type"), Node->GetClass()->GetName());
				NodeObj->SetStringField(TEXT("graph_name"), Graph->GetName());
				NodeObj->SetNumberField(TEXT("position_x"), Node->NodePosX);
				NodeObj->SetNumberField(TEXT("position_y"), Node->NodePosY);

				OrphanedNodesList.Add(MakeShared<FJsonValueObject>(NodeObj));
			}
		}
	}

	OrphanedInfo->SetArrayField(TEXT("nodes"), OrphanedNodesList);
	OrphanedInfo->SetNumberField(TEXT("count"), OrphanedNodesList.Num());

	return OrphanedInfo;
}
