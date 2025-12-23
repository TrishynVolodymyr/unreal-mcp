#include "Commands/Blueprint/GetBlueprintMetadataCommand.h"
#include "Engine/Blueprint.h"
#include "Services/AssetDiscoveryService.h"
#include "Services/IBlueprintService.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Components/TimelineComponent.h"
#include "Engine/TimelineTemplate.h"
#include "HAL/FileManager.h"

FGetBlueprintMetadataCommand::FGetBlueprintMetadataCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FGetBlueprintMetadataCommand::Execute(const FString& Parameters)
{
    FString BlueprintName;
    TArray<FString> Fields;
    FGraphNodesFilter Filter;
    FString ParseError;

    if (!ParseParameters(Parameters, BlueprintName, Fields, Filter, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    UBlueprint* Blueprint = FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }

    TSharedPtr<FJsonObject> Metadata = BuildMetadata(Blueprint, Fields, Filter);
    return CreateSuccessResponse(Metadata);
}

FString FGetBlueprintMetadataCommand::GetCommandName() const
{
    return TEXT("get_blueprint_metadata");
}

bool FGetBlueprintMetadataCommand::ValidateParams(const FString& Parameters) const
{
    // Only validate basic JSON structure - detailed validation happens in Execute
    // so we can return meaningful error messages
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // Just check blueprint_name exists
    FString BlueprintName;
    return JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
}

bool FGetBlueprintMetadataCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FString>& OutFields, FGraphNodesFilter& OutFilter, FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), OutBlueprintName))
    {
        OutError = TEXT("Missing required 'blueprint_name' parameter");
        return false;
    }

    // Parse required fields array - at least one field must be specified
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
    if (JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray) && FieldsArray->Num() > 0)
    {
        for (const TSharedPtr<FJsonValue>& Value : *FieldsArray)
        {
            OutFields.Add(Value->AsString());
        }
    }
    else
    {
        OutError = TEXT("Missing required 'fields' parameter. Specify at least one field (e.g., [\"components\"], [\"variables\"], [\"graph_nodes\"])");
        return false;
    }

    // Parse optional filters for graph_nodes field
    JsonObject->TryGetStringField(TEXT("graph_name"), OutFilter.GraphName);
    JsonObject->TryGetStringField(TEXT("node_type"), OutFilter.NodeType);
    JsonObject->TryGetStringField(TEXT("event_type"), OutFilter.EventType);

    // Validate: graph_nodes requires graph_name to be specified
    if (OutFields.Contains(TEXT("graph_nodes")) && OutFilter.GraphName.IsEmpty())
    {
        OutError = TEXT("When requesting 'graph_nodes' field, 'graph_name' parameter is required to limit response size");
        return false;
    }

    return true;
}

UBlueprint* FGetBlueprintMetadataCommand::FindBlueprint(const FString& BlueprintName) const
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Handle full path
    if (BlueprintName.StartsWith(TEXT("/Game/")) || BlueprintName.StartsWith(TEXT("/Script/")))
    {
        FString ObjectPath = BlueprintName;

        // If path doesn't contain '.', append the asset name (e.g., /Game/Foo/Bar -> /Game/Foo/Bar.Bar)
        if (!ObjectPath.Contains(TEXT(".")))
        {
            FString AssetName = FPaths::GetBaseFilename(ObjectPath);
            ObjectPath = ObjectPath + TEXT(".") + AssetName;
        }

        FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
        if (AssetData.IsValid())
        {
            return Cast<UBlueprint>(AssetData.GetAsset());
        }
    }

    // Handle simple name using FAssetDiscoveryService
    TArray<FString> FoundBlueprints = FAssetDiscoveryService::Get().FindBlueprints(BlueprintName);
    if (FoundBlueprints.Num() > 0)
    {
        return LoadObject<UBlueprint>(nullptr, *FoundBlueprints[0]);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildMetadata(UBlueprint* Blueprint, const TArray<FString>& Fields, const FGraphNodesFilter& Filter) const
{
    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();

    if (ShouldIncludeField(TEXT("parent_class"), Fields))
    {
        Metadata->SetObjectField(TEXT("parent_class"), BuildParentClassInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("interfaces"), Fields))
    {
        Metadata->SetObjectField(TEXT("interfaces"), BuildInterfacesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("variables"), Fields))
    {
        Metadata->SetObjectField(TEXT("variables"), BuildVariablesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("functions"), Fields))
    {
        Metadata->SetObjectField(TEXT("functions"), BuildFunctionsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("components"), Fields))
    {
        Metadata->SetObjectField(TEXT("components"), BuildComponentsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("graphs"), Fields))
    {
        Metadata->SetObjectField(TEXT("graphs"), BuildGraphsInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("status"), Fields))
    {
        Metadata->SetObjectField(TEXT("status"), BuildStatusInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("metadata"), Fields))
    {
        Metadata->SetObjectField(TEXT("metadata"), BuildMetadataInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("timelines"), Fields))
    {
        Metadata->SetObjectField(TEXT("timelines"), BuildTimelinesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("asset_info"), Fields))
    {
        Metadata->SetObjectField(TEXT("asset_info"), BuildAssetInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("orphaned_nodes"), Fields))
    {
        Metadata->SetObjectField(TEXT("orphaned_nodes"), BuildOrphanedNodesInfo(Blueprint));
    }
    if (ShouldIncludeField(TEXT("graph_nodes"), Fields))
    {
        Metadata->SetObjectField(TEXT("graph_nodes"), BuildGraphNodesInfo(Blueprint, Filter));
    }

    return Metadata;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildParentClassInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ParentInfo = MakeShared<FJsonObject>();

    UClass* ParentClass = Blueprint->ParentClass;
    if (ParentClass)
    {
        ParentInfo->SetStringField(TEXT("name"), ParentClass->GetName());
        ParentInfo->SetStringField(TEXT("path"), ParentClass->GetPathName());

        // Determine if native or Blueprint
        bool bIsNative = ParentClass->IsNative();
        ParentInfo->SetStringField(TEXT("type"), bIsNative ? TEXT("Native") : TEXT("Blueprint"));

        // Build inheritance chain
        TArray<TSharedPtr<FJsonValue>> InheritanceChain;
        UClass* CurrentClass = ParentClass;
        while (CurrentClass)
        {
            TSharedPtr<FJsonObject> ClassInfo = MakeShared<FJsonObject>();
            ClassInfo->SetStringField(TEXT("name"), CurrentClass->GetName());
            ClassInfo->SetStringField(TEXT("path"), CurrentClass->GetPathName());
            InheritanceChain.Add(MakeShared<FJsonValueObject>(ClassInfo));
            CurrentClass = CurrentClass->GetSuperClass();
        }
        ParentInfo->SetArrayField(TEXT("inheritance_chain"), InheritanceChain);
    }
    else
    {
        ParentInfo->SetStringField(TEXT("name"), TEXT("None"));
    }

    return ParentInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildInterfacesInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> InterfacesInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> InterfacesList;

    for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
    {
        TSharedPtr<FJsonObject> InterfaceObj = MakeShared<FJsonObject>();

        if (Interface.Interface)
        {
            InterfaceObj->SetStringField(TEXT("name"), Interface.Interface->GetName());
            InterfaceObj->SetStringField(TEXT("path"), Interface.Interface->GetPathName());

            // Get interface functions
            TArray<TSharedPtr<FJsonValue>> FunctionsList;
            for (TFieldIterator<UFunction> FuncIt(Interface.Interface, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
            {
                UFunction* InterfaceFunc = *FuncIt;
                TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
                FuncObj->SetStringField(TEXT("name"), InterfaceFunc->GetName());

                // Check if implemented
                UFunction* ImplementedFunc = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->FindFunctionByName(InterfaceFunc->GetFName()) : nullptr;
                FuncObj->SetBoolField(TEXT("implemented"), ImplementedFunc != nullptr);

                FunctionsList.Add(MakeShared<FJsonValueObject>(FuncObj));
            }
            InterfaceObj->SetArrayField(TEXT("functions"), FunctionsList);
        }

        InterfacesList.Add(MakeShared<FJsonValueObject>(InterfaceObj));
    }

    InterfacesInfo->SetArrayField(TEXT("interfaces"), InterfacesList);
    InterfacesInfo->SetNumberField(TEXT("count"), InterfacesList.Num());

    return InterfacesInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildVariablesInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> VariablesInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> VariablesList;

    // Get CDO for default values
    UObject* CDO = (Blueprint->GeneratedClass) ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;

    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());
        VarObj->SetBoolField(TEXT("instance_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
        VarObj->SetBoolField(TEXT("blueprint_read_only"), (Variable.PropertyFlags & CPF_BlueprintReadOnly) != 0);

        // Get default value from CDO
        if (CDO)
        {
            FProperty* Property = CDO->GetClass()->FindPropertyByName(Variable.VarName);
            if (Property)
            {
                FString DefaultValueStr;
                const void* PropertyData = Property->ContainerPtrToValuePtr<void>(CDO);

                // Handle different property types
                if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
                {
                    DefaultValueStr = BoolProp->GetPropertyValue(PropertyData) ? TEXT("true") : TEXT("false");
                }
                else if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
                {
                    DefaultValueStr = FString::FromInt(IntProp->GetPropertyValue(PropertyData));
                }
                else if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
                {
                    DefaultValueStr = FString::SanitizeFloat(FloatProp->GetPropertyValue(PropertyData));
                }
                else if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
                {
                    DefaultValueStr = FString::SanitizeFloat(DoubleProp->GetPropertyValue(PropertyData));
                }
                else if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
                {
                    DefaultValueStr = StrProp->GetPropertyValue(PropertyData);
                }
                else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
                {
                    DefaultValueStr = NameProp->GetPropertyValue(PropertyData).ToString();
                }
                else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
                {
                    DefaultValueStr = TextProp->GetPropertyValue(PropertyData).ToString();
                }
                else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
                {
                    UObject* ObjValue = ObjProp->GetObjectPropertyValue(PropertyData);
                    if (ObjValue)
                    {
                        DefaultValueStr = ObjValue->GetPathName();
                    }
                    else
                    {
                        DefaultValueStr = TEXT("None");
                    }
                }
                else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
                {
                    FScriptArrayHelper ArrayHelper(ArrayProp, PropertyData);
                    DefaultValueStr = FString::Printf(TEXT("[Array: %d elements]"), ArrayHelper.Num());
                }
                else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
                {
                    DefaultValueStr = FString::Printf(TEXT("[Struct: %s]"), *StructProp->Struct->GetName());
                }
                else
                {
                    // Fallback: try to export as string
                    Property->ExportTextItem_Direct(DefaultValueStr, PropertyData, nullptr, nullptr, PPF_None);
                }

                VarObj->SetStringField(TEXT("default_value"), DefaultValueStr);
            }
        }

        VariablesList.Add(MakeShared<FJsonValueObject>(VarObj));
    }

    VariablesInfo->SetArrayField(TEXT("variables"), VariablesList);
    VariablesInfo->SetNumberField(TEXT("count"), VariablesList.Num());

    return VariablesInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildFunctionsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> FunctionsInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> FunctionsList;

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
        FuncObj->SetStringField(TEXT("name"), Graph->GetName());

        // Find the function entry node to get detailed info
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
            // Get function flags
            FuncObj->SetBoolField(TEXT("is_pure"), EntryNode->GetFunctionFlags() & FUNC_BlueprintPure);
            FuncObj->SetBoolField(TEXT("is_const"), EntryNode->GetFunctionFlags() & FUNC_Const);

            // Get access specifier
            FString AccessSpecifier = TEXT("Public");
            if (EntryNode->GetFunctionFlags() & FUNC_Protected)
            {
                AccessSpecifier = TEXT("Protected");
            }
            else if (EntryNode->GetFunctionFlags() & FUNC_Private)
            {
                AccessSpecifier = TEXT("Private");
            }
            FuncObj->SetStringField(TEXT("access"), AccessSpecifier);

            // Get category if available
            FuncObj->SetStringField(TEXT("category"), EntryNode->MetaData.Category.ToString());
        }

        // Get inputs and outputs from the entry node's UserDefinedPins (most accurate, includes uncompiled changes)
        // This reads directly from the graph definition rather than the compiled GeneratedClass
        TArray<TSharedPtr<FJsonValue>> InputsList;
        TArray<TSharedPtr<FJsonValue>> OutputsList;

        if (EntryNode)
        {
            // Read inputs from entry node's user defined pins
            for (const TSharedPtr<FUserPinInfo>& PinInfo : EntryNode->UserDefinedPins)
            {
                if (PinInfo.IsValid())
                {
                    TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
                    ParamObj->SetStringField(TEXT("name"), PinInfo->PinName.ToString());

                    // Get type info from pin type
                    FString TypeString = GetPinTypeAsString(PinInfo->PinType);
                    ParamObj->SetStringField(TEXT("type"), TypeString);

                    InputsList.Add(MakeShared<FJsonValueObject>(ParamObj));
                }
            }
        }

        // Find result node for outputs
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node);
            if (ResultNode)
            {
                for (const TSharedPtr<FUserPinInfo>& PinInfo : ResultNode->UserDefinedPins)
                {
                    if (PinInfo.IsValid())
                    {
                        TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
                        ParamObj->SetStringField(TEXT("name"), PinInfo->PinName.ToString());

                        FString TypeString = GetPinTypeAsString(PinInfo->PinType);
                        ParamObj->SetStringField(TEXT("type"), TypeString);

                        OutputsList.Add(MakeShared<FJsonValueObject>(ParamObj));
                    }
                }
                break; // Only need first result node
            }
        }

        FuncObj->SetArrayField(TEXT("inputs"), InputsList);
        FuncObj->SetArrayField(TEXT("outputs"), OutputsList);

        FunctionsList.Add(MakeShared<FJsonValueObject>(FuncObj));
    }

    FunctionsInfo->SetArrayField(TEXT("functions"), FunctionsList);
    FunctionsInfo->SetNumberField(TEXT("count"), FunctionsList.Num());

    return FunctionsInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildComponentsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ComponentsInfo = MakeShared<FJsonObject>();

    // Use BlueprintService to get comprehensive component list
    TArray<TPair<FString, FString>> Components;
    if (BlueprintService.GetBlueprintComponents(Blueprint, Components))
    {
        TArray<TSharedPtr<FJsonValue>> ComponentsList;

        for (const auto& Component : Components)
        {
            TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
            CompObj->SetStringField(TEXT("name"), Component.Key);
            CompObj->SetStringField(TEXT("type"), Component.Value);
            ComponentsList.Add(MakeShared<FJsonValueObject>(CompObj));
        }

        ComponentsInfo->SetArrayField(TEXT("components"), ComponentsList);
        ComponentsInfo->SetNumberField(TEXT("count"), ComponentsList.Num());
    }
    else
    {
        // Fallback to empty array if service call fails
        ComponentsInfo->SetArrayField(TEXT("components"), TArray<TSharedPtr<FJsonValue>>());
        ComponentsInfo->SetNumberField(TEXT("count"), 0);
    }

    return ComponentsInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildGraphsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> GraphsInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> GraphsList;

    // Collect all graphs
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        if (Graph)
        {
            TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
            GraphObj->SetStringField(TEXT("name"), Graph->GetName());
            GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

            GraphsList.Add(MakeShared<FJsonValueObject>(GraphObj));
        }
    }

    GraphsInfo->SetArrayField(TEXT("graphs"), GraphsList);
    GraphsInfo->SetNumberField(TEXT("count"), GraphsList.Num());

    return GraphsInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildStatusInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> StatusInfo = MakeShared<FJsonObject>();

    FString StatusString;
    switch (Blueprint->Status)
    {
        case BS_Unknown: StatusString = TEXT("Unknown"); break;
        case BS_Dirty: StatusString = TEXT("Dirty"); break;
        case BS_Error: StatusString = TEXT("Error"); break;
        case BS_UpToDate: StatusString = TEXT("UpToDate"); break;
        case BS_BeingCreated: StatusString = TEXT("BeingCreated"); break;
        case BS_UpToDateWithWarnings: StatusString = TEXT("UpToDateWithWarnings"); break;
        default: StatusString = TEXT("Unknown"); break;
    }
    StatusInfo->SetStringField(TEXT("status"), StatusString);

    FString TypeString;
    switch (Blueprint->BlueprintType)
    {
        case BPTYPE_Normal: TypeString = TEXT("Normal"); break;
        case BPTYPE_Const: TypeString = TEXT("Const"); break;
        case BPTYPE_MacroLibrary: TypeString = TEXT("MacroLibrary"); break;
        case BPTYPE_Interface: TypeString = TEXT("Interface"); break;
        case BPTYPE_LevelScript: TypeString = TEXT("LevelScript"); break;
        case BPTYPE_FunctionLibrary: TypeString = TEXT("FunctionLibrary"); break;
        default: TypeString = TEXT("Unknown"); break;
    }
    StatusInfo->SetStringField(TEXT("blueprint_type"), TypeString);

    return StatusInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildMetadataInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> MetadataObj = MakeShared<FJsonObject>();

    // Get metadata from Blueprint class
    if (Blueprint->GeneratedClass)
    {
        UClass* BPClass = Blueprint->GeneratedClass;

        MetadataObj->SetStringField(TEXT("display_name"), BPClass->GetMetaData(TEXT("DisplayName")));
        MetadataObj->SetStringField(TEXT("description"), BPClass->GetMetaData(TEXT("BlueprintDescription")));
        MetadataObj->SetStringField(TEXT("category"), BPClass->GetMetaData(TEXT("Category")));
        MetadataObj->SetStringField(TEXT("namespace"), BPClass->GetMetaData(TEXT("BlueprintNamespace")));
    }

    return MetadataObj;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildTimelinesInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> TimelinesInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> TimelinesList;

    for (UTimelineTemplate* Timeline : Blueprint->Timelines)
    {
        if (Timeline)
        {
            TSharedPtr<FJsonObject> TimelineObj = MakeShared<FJsonObject>();
            TimelineObj->SetStringField(TEXT("name"), Timeline->GetName());

            int32 TrackCount = Timeline->FloatTracks.Num() + Timeline->VectorTracks.Num() +
                              Timeline->LinearColorTracks.Num() + Timeline->EventTracks.Num();
            TimelineObj->SetNumberField(TEXT("track_count"), TrackCount);

            TimelinesList.Add(MakeShared<FJsonValueObject>(TimelineObj));
        }
    }

    TimelinesInfo->SetArrayField(TEXT("timelines"), TimelinesList);
    TimelinesInfo->SetNumberField(TEXT("count"), TimelinesList.Num());

    return TimelinesInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildAssetInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> AssetInfo = MakeShared<FJsonObject>();

    AssetInfo->SetStringField(TEXT("asset_path"), Blueprint->GetPathName());
    AssetInfo->SetStringField(TEXT("package_name"), Blueprint->GetPackage()->GetName());

    // Get file size
    FString PackageFilename;
    if (FPackageName::DoesPackageExist(Blueprint->GetPackage()->GetName(), &PackageFilename))
    {
        int64 FileSize = IFileManager::Get().FileSize(*PackageFilename);
        AssetInfo->SetNumberField(TEXT("disk_size_bytes"), static_cast<double>(FileSize));
    }

    return AssetInfo;
}

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildOrphanedNodesInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> OrphanedInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> OrphanedNodesList;

    // Collect all graphs
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

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

            // Check if node has any connected pins (excluding exec pins for certain checks)
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
            // Event nodes (like BeginPlay) are not orphaned if they have outputs connected
            bool bIsOrphaned = !bHasConnection;

            // Skip event entry nodes - they're supposed to be entry points
            if (Cast<UK2Node_Event>(Node) || Cast<UK2Node_FunctionEntry>(Node))
            {
                // Event/Entry nodes are only orphaned if they have no output connections
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

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildGraphNodesInfo(UBlueprint* Blueprint, const FGraphNodesFilter& Filter) const
{
    TSharedPtr<FJsonObject> GraphNodesInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> GraphsList;

    // Collect graphs to process
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph)
        {
            continue;
        }

        // Filter by graph name if specified
        if (!Filter.GraphName.IsEmpty() && Graph->GetName() != Filter.GraphName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
        GraphObj->SetStringField(TEXT("name"), Graph->GetName());

        TArray<TSharedPtr<FJsonValue>> NodesList;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node)
            {
                continue;
            }

            // Apply node type filter
            if (!MatchesNodeTypeFilter(Node, Filter.NodeType))
            {
                continue;
            }

            // Apply event type filter
            if (!MatchesEventTypeFilter(Node, Filter.EventType))
            {
                continue;
            }

            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
            NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
            NodeObj->SetStringField(TEXT("node_type"), Node->GetClass()->GetName());
            NodeObj->SetNumberField(TEXT("position_x"), Node->NodePosX);
            NodeObj->SetNumberField(TEXT("position_y"), Node->NodePosY);

            // Build pins array with connection info
            TArray<TSharedPtr<FJsonValue>> PinsList;

            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin)
                {
                    continue;
                }

                TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                PinObj->SetStringField(TEXT("pin_name"), Pin->PinName.ToString());
                PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));

                // Get pin type as readable string
                FString PinTypeStr = GetPinTypeAsString(Pin->PinType);
                // Add exec indicator
                if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                {
                    PinTypeStr = TEXT("exec");
                }
                PinObj->SetStringField(TEXT("type"), PinTypeStr);

                // Get default value for unconnected input pins
                if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 0)
                {
                    FString DefaultValue = Pin->DefaultValue;
                    if (DefaultValue.IsEmpty() && Pin->DefaultObject)
                    {
                        DefaultValue = Pin->DefaultObject->GetName();
                    }
                    if (DefaultValue.IsEmpty() && !Pin->DefaultTextValue.IsEmpty())
                    {
                        DefaultValue = Pin->DefaultTextValue.ToString();
                    }
                    if (!DefaultValue.IsEmpty())
                    {
                        PinObj->SetStringField(TEXT("default_value"), DefaultValue);
                    }
                }

                // Build connections array
                TArray<TSharedPtr<FJsonValue>> ConnectionsList;
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (LinkedPin && LinkedPin->GetOwningNode())
                    {
                        TSharedPtr<FJsonObject> ConnectionObj = MakeShared<FJsonObject>();
                        ConnectionObj->SetStringField(TEXT("node_id"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
                        ConnectionObj->SetStringField(TEXT("node_title"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString());
                        ConnectionObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
                        ConnectionsList.Add(MakeShared<FJsonValueObject>(ConnectionObj));
                    }
                }

                if (ConnectionsList.Num() > 0)
                {
                    PinObj->SetArrayField(TEXT("connected_to"), ConnectionsList);
                }

                PinsList.Add(MakeShared<FJsonValueObject>(PinObj));
            }

            NodeObj->SetArrayField(TEXT("pins"), PinsList);
            NodesList.Add(MakeShared<FJsonValueObject>(NodeObj));
        }

        GraphObj->SetArrayField(TEXT("nodes"), NodesList);
        GraphObj->SetNumberField(TEXT("node_count"), NodesList.Num());
        GraphsList.Add(MakeShared<FJsonValueObject>(GraphObj));
    }

    GraphNodesInfo->SetArrayField(TEXT("graphs"), GraphsList);
    GraphNodesInfo->SetNumberField(TEXT("graph_count"), GraphsList.Num());

    return GraphNodesInfo;
}

FString FGetBlueprintMetadataCommand::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Metadata) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("metadata"), Metadata);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

FString FGetBlueprintMetadataCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);

    return OutputString;
}

bool FGetBlueprintMetadataCommand::ShouldIncludeField(const FString& FieldName, const TArray<FString>& RequestedFields) const
{
    // If "*" is in the list, include all fields
    if (RequestedFields.Contains(TEXT("*")))
    {
        return true;
    }

    return RequestedFields.Contains(FieldName);
}

FString FGetBlueprintMetadataCommand::GetPinTypeAsString(const FEdGraphPinType& PinType) const
{
    // Handle basic types
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
    {
        return TEXT("bool");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
    {
        return TEXT("int32");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
    {
        return TEXT("int64");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        // Check subcategory for float vs double
        if (PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
        {
            return TEXT("float");
        }
        else if (PinType.PinSubCategory == UEdGraphSchema_K2::PC_Double)
        {
            return TEXT("double");
        }
        return TEXT("float"); // Default
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_String)
    {
        return TEXT("FString");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
    {
        return TEXT("FText");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
    {
        return TEXT("FName");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte)
    {
        return TEXT("uint8");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
    {
        // Return the struct name
        if (UScriptStruct* Struct = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()))
        {
            return Struct->GetName();
        }
        return TEXT("Struct");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
             PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
             PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
             PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
    {
        // Return the class name - this handles Blueprint classes like BP_DialogueNPC
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
        {
            return Class->GetName();
        }
        return TEXT("Object");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Interface)
    {
        if (UClass* Interface = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
        {
            return Interface->GetName();
        }
        return TEXT("Interface");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Enum)
    {
        if (UEnum* Enum = Cast<UEnum>(PinType.PinSubCategoryObject.Get()))
        {
            return Enum->GetName();
        }
        return TEXT("Enum");
    }

    // Fallback: return the category name
    return PinType.PinCategory.ToString();
}

bool FGetBlueprintMetadataCommand::MatchesNodeTypeFilter(UEdGraphNode* Node, const FString& NodeType) const
{
    if (NodeType.IsEmpty())
    {
        return true;
    }

    FString NodeTypeLower = NodeType.ToLower();

    // Check for Event type
    if (NodeTypeLower == TEXT("event"))
    {
        return Node->IsA<UK2Node_Event>();
    }

    // Check for Function type (function entry or call nodes)
    if (NodeTypeLower == TEXT("function"))
    {
        return Node->IsA<UK2Node_FunctionEntry>() || Node->GetClass()->GetName().Contains(TEXT("CallFunction"));
    }

    // Check for Variable type
    if (NodeTypeLower == TEXT("variable"))
    {
        FString ClassName = Node->GetClass()->GetName();
        return ClassName.Contains(TEXT("VariableGet")) || ClassName.Contains(TEXT("VariableSet"));
    }

    // Check for Comment type
    if (NodeTypeLower == TEXT("comment"))
    {
        return Node->GetClass()->GetName().Contains(TEXT("Comment"));
    }

    // Generic class name match
    return Node->GetClass()->GetName().Contains(NodeType);
}

bool FGetBlueprintMetadataCommand::MatchesEventTypeFilter(UEdGraphNode* Node, const FString& EventType) const
{
    if (EventType.IsEmpty())
    {
        return true;
    }

    // Only applies to Event nodes
    UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
    if (!EventNode)
    {
        return false;
    }

    // Get the event's function name
    FName EventFuncName = EventNode->GetFunctionName();
    FString EventName = EventFuncName.ToString();

    // Common event name mappings
    FString EventTypeLower = EventType.ToLower();

    if (EventTypeLower == TEXT("beginplay"))
    {
        return EventName.Contains(TEXT("BeginPlay"));
    }
    if (EventTypeLower == TEXT("tick"))
    {
        return EventName.Contains(TEXT("Tick"));
    }
    if (EventTypeLower == TEXT("endplay"))
    {
        return EventName.Contains(TEXT("EndPlay"));
    }
    if (EventTypeLower == TEXT("destroyed"))
    {
        return EventName.Contains(TEXT("Destroyed"));
    }
    if (EventTypeLower == TEXT("constructed") || EventTypeLower == TEXT("construct"))
    {
        return EventName.Contains(TEXT("Construct"));
    }

    // Generic match
    return EventName.Contains(EventType);
}
