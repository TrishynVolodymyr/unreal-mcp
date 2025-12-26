#include "Services/Blueprint/BlueprintMetadataBuilderService.h"
#include "Services/IBlueprintService.h"
#include "Utils/GraphUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Components/TimelineComponent.h"
#include "Engine/TimelineTemplate.h"
#include "HAL/FileManager.h"

FBlueprintMetadataBuilderService::FBlueprintMetadataBuilderService(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildParentClassInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ParentInfo = MakeShared<FJsonObject>();

    UClass* ParentClass = Blueprint->ParentClass;
    if (ParentClass)
    {
        ParentInfo->SetStringField(TEXT("name"), ParentClass->GetName());
        ParentInfo->SetStringField(TEXT("path"), ParentClass->GetPathName());

        bool bIsNative = ParentClass->IsNative();
        ParentInfo->SetStringField(TEXT("type"), bIsNative ? TEXT("Native") : TEXT("Blueprint"));

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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildInterfacesInfo(UBlueprint* Blueprint) const
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

            TArray<TSharedPtr<FJsonValue>> FunctionsList;
            for (TFieldIterator<UFunction> FuncIt(Interface.Interface, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
            {
                UFunction* InterfaceFunc = *FuncIt;
                TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
                FuncObj->SetStringField(TEXT("name"), InterfaceFunc->GetName());

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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildVariablesInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> VariablesInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> VariablesList;

    UObject* CDO = (Blueprint->GeneratedClass) ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;

    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());
        VarObj->SetBoolField(TEXT("instance_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
        VarObj->SetBoolField(TEXT("blueprint_read_only"), (Variable.PropertyFlags & CPF_BlueprintReadOnly) != 0);

        if (CDO)
        {
            FProperty* Property = CDO->GetClass()->FindPropertyByName(Variable.VarName);
            if (Property)
            {
                FString DefaultValueStr;
                const void* PropertyData = Property->ContainerPtrToValuePtr<void>(CDO);

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
                    DefaultValueStr = ObjValue ? ObjValue->GetPathName() : TEXT("None");
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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildFunctionsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> FunctionsInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> FunctionsList;

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;

        TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
        FuncObj->SetStringField(TEXT("name"), Graph->GetName());

        UK2Node_FunctionEntry* EntryNode = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            EntryNode = Cast<UK2Node_FunctionEntry>(Node);
            if (EntryNode) break;
        }

        if (EntryNode)
        {
            FuncObj->SetBoolField(TEXT("is_pure"), EntryNode->GetFunctionFlags() & FUNC_BlueprintPure);
            FuncObj->SetBoolField(TEXT("is_const"), EntryNode->GetFunctionFlags() & FUNC_Const);

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
            FuncObj->SetStringField(TEXT("category"), EntryNode->MetaData.Category.ToString());
        }

        TArray<TSharedPtr<FJsonValue>> InputsList;
        TArray<TSharedPtr<FJsonValue>> OutputsList;

        if (EntryNode)
        {
            for (const TSharedPtr<FUserPinInfo>& PinInfo : EntryNode->UserDefinedPins)
            {
                if (PinInfo.IsValid())
                {
                    TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
                    ParamObj->SetStringField(TEXT("name"), PinInfo->PinName.ToString());
                    ParamObj->SetStringField(TEXT("type"), GetPinTypeAsString(PinInfo->PinType));
                    InputsList.Add(MakeShared<FJsonValueObject>(ParamObj));
                }
            }
        }

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
                        ParamObj->SetStringField(TEXT("type"), GetPinTypeAsString(PinInfo->PinType));
                        OutputsList.Add(MakeShared<FJsonValueObject>(ParamObj));
                    }
                }
                break;
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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildComponentsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ComponentsInfo = MakeShared<FJsonObject>();

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
        ComponentsInfo->SetArrayField(TEXT("components"), TArray<TSharedPtr<FJsonValue>>());
        ComponentsInfo->SetNumberField(TEXT("count"), 0);
    }

    return ComponentsInfo;
}

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildGraphsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> GraphsInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> GraphsList;

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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildStatusInfo(UBlueprint* Blueprint) const
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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildMetadataInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> MetadataObj = MakeShared<FJsonObject>();

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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildTimelinesInfo(UBlueprint* Blueprint) const
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

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildAssetInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> AssetInfo = MakeShared<FJsonObject>();

    AssetInfo->SetStringField(TEXT("asset_path"), Blueprint->GetPathName());
    AssetInfo->SetStringField(TEXT("package_name"), Blueprint->GetPackage()->GetName());

    FString PackageFilename;
    if (FPackageName::DoesPackageExist(Blueprint->GetPackage()->GetName(), &PackageFilename))
    {
        int64 FileSize = IFileManager::Get().FileSize(*PackageFilename);
        AssetInfo->SetNumberField(TEXT("disk_size_bytes"), static_cast<double>(FileSize));
    }

    return AssetInfo;
}

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildOrphanedNodesInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> OrphanedInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> OrphanedNodesList;

    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        TArray<TSharedPtr<FJsonObject>> OrphanedNodes;
        if (FGraphUtils::GetOrphanedNodesInfo(Graph, OrphanedNodes))
        {
            for (const TSharedPtr<FJsonObject>& NodeInfo : OrphanedNodes)
            {
                TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                NodeObj->SetStringField(TEXT("id"), NodeInfo->GetStringField(TEXT("node_id")));
                NodeObj->SetStringField(TEXT("title"), NodeInfo->GetStringField(TEXT("title")));
                NodeObj->SetStringField(TEXT("graph"), Graph->GetName());
                NodeObj->SetStringField(TEXT("class"), NodeInfo->GetStringField(TEXT("class")));
                NodeObj->SetNumberField(TEXT("pos_x"), NodeInfo->GetNumberField(TEXT("pos_x")));
                NodeObj->SetNumberField(TEXT("pos_y"), NodeInfo->GetNumberField(TEXT("pos_y")));
                NodeObj->SetNumberField(TEXT("input_connections"), NodeInfo->GetNumberField(TEXT("input_connections")));
                NodeObj->SetNumberField(TEXT("output_connections"), NodeInfo->GetNumberField(TEXT("output_connections")));
                OrphanedNodesList.Add(MakeShared<FJsonValueObject>(NodeObj));
            }
        }
    }

    OrphanedInfo->SetArrayField(TEXT("nodes"), OrphanedNodesList);
    OrphanedInfo->SetNumberField(TEXT("count"), OrphanedNodesList.Num());

    return OrphanedInfo;
}

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildGraphWarningsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> WarningsInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> WarningsList;

    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;

            FString NodeClassName = Node->GetClass()->GetName();
            if (NodeClassName.Contains(TEXT("DynamicCast")))
            {
                bool bHasExecInput = false;
                bool bHasExecOutput = false;

                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                    {
                        if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
                        {
                            bHasExecInput = true;
                        }
                        else if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                        {
                            bHasExecOutput = true;
                        }
                    }
                }

                if (!bHasExecInput || !bHasExecOutput)
                {
                    TSharedPtr<FJsonObject> WarningObj = MakeShared<FJsonObject>();
                    WarningObj->SetStringField(TEXT("type"), TEXT("disconnected_cast_exec"));
                    WarningObj->SetStringField(TEXT("node_id"), FGraphUtils::GetReliableNodeId(Node));
                    WarningObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
                    WarningObj->SetStringField(TEXT("graph"), Graph->GetName());
                    WarningObj->SetBoolField(TEXT("has_exec_input"), bHasExecInput);
                    WarningObj->SetBoolField(TEXT("has_exec_output"), bHasExecOutput);
                    WarningObj->SetStringField(TEXT("message"),
                        FString::Printf(TEXT("Cast node '%s' has disconnected exec pins - it will NOT execute at runtime"),
                            *Node->GetNodeTitle(ENodeTitleType::ListView).ToString()));
                    WarningsList.Add(MakeShared<FJsonValueObject>(WarningObj));
                }
            }
        }
    }

    WarningsInfo->SetArrayField(TEXT("warnings"), WarningsList);
    WarningsInfo->SetNumberField(TEXT("count"), WarningsList.Num());

    return WarningsInfo;
}

TSharedPtr<FJsonObject> FBlueprintMetadataBuilderService::BuildGraphNodesInfo(UBlueprint* Blueprint, const FGraphNodesFilter& Filter) const
{
    TSharedPtr<FJsonObject> GraphNodesInfo = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> GraphsList;

    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        if (!Filter.GraphName.IsEmpty() && Graph->GetName() != Filter.GraphName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
        GraphObj->SetStringField(TEXT("name"), Graph->GetName());

        TArray<TSharedPtr<FJsonValue>> NodesList;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;

            if (!MatchesNodeTypeFilter(Node, Filter.NodeType)) continue;
            if (!MatchesEventTypeFilter(Node, Filter.EventType)) continue;

            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("id"), FGraphUtils::GetReliableNodeId(Node));
            NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());

            if (Filter.DetailLevel != EGraphNodesDetailLevel::Summary)
            {
                TSharedPtr<FJsonObject> PinsObj = MakeShared<FJsonObject>();

                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (!Pin) continue;

                    bool bIsExecPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
                    if (Filter.DetailLevel == EGraphNodesDetailLevel::Flow && !bIsExecPin)
                    {
                        continue;
                    }

                    FString PinName = Pin->PinName.ToString();

                    if (Pin->LinkedTo.Num() > 0)
                    {
                        TArray<TSharedPtr<FJsonValue>> ConnectionsList;
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                        {
                            if (LinkedPin && LinkedPin->GetOwningNode())
                            {
                                UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
                                FString CompactConn = FString::Printf(TEXT("%s|%s|%s"),
                                    *FGraphUtils::GetReliableNodeId(LinkedNode),
                                    *LinkedNode->GetNodeTitle(ENodeTitleType::ListView).ToString(),
                                    *LinkedPin->PinName.ToString());
                                ConnectionsList.Add(MakeShared<FJsonValueString>(CompactConn));
                            }
                        }
                        PinsObj->SetArrayField(PinName, ConnectionsList);
                    }
                    else if (Pin->Direction == EGPD_Input && Filter.DetailLevel == EGraphNodesDetailLevel::Full)
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
                            PinsObj->SetStringField(PinName, DefaultValue);
                        }
                    }
                }

                NodeObj->SetObjectField(TEXT("pins"), PinsObj);
            }

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

FString FBlueprintMetadataBuilderService::GetPinTypeAsString(const FEdGraphPinType& PinType) const
{
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean) return TEXT("bool");
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int) return TEXT("int32");
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int64) return TEXT("int64");
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        if (PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float) return TEXT("float");
        if (PinType.PinSubCategory == UEdGraphSchema_K2::PC_Double) return TEXT("double");
        return TEXT("float");
    }
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_String) return TEXT("FString");
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text) return TEXT("FText");
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name) return TEXT("FName");
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte) return TEXT("uint8");

    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
    {
        if (UScriptStruct* Struct = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()))
        {
            return Struct->GetName();
        }
        return TEXT("Struct");
    }

    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
        PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
        PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
        PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
        {
            return Class->GetName();
        }
        return TEXT("Object");
    }

    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Interface)
    {
        if (UClass* Interface = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
        {
            return Interface->GetName();
        }
        return TEXT("Interface");
    }

    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Enum)
    {
        if (UEnum* Enum = Cast<UEnum>(PinType.PinSubCategoryObject.Get()))
        {
            return Enum->GetName();
        }
        return TEXT("Enum");
    }

    return PinType.PinCategory.ToString();
}

bool FBlueprintMetadataBuilderService::MatchesNodeTypeFilter(UEdGraphNode* Node, const FString& NodeType) const
{
    if (NodeType.IsEmpty()) return true;

    FString NodeTypeLower = NodeType.ToLower();

    if (NodeTypeLower == TEXT("event"))
    {
        return Node->IsA<UK2Node_Event>();
    }
    if (NodeTypeLower == TEXT("function"))
    {
        return Node->IsA<UK2Node_FunctionEntry>() || Node->GetClass()->GetName().Contains(TEXT("CallFunction"));
    }
    if (NodeTypeLower == TEXT("variable"))
    {
        FString ClassName = Node->GetClass()->GetName();
        return ClassName.Contains(TEXT("VariableGet")) || ClassName.Contains(TEXT("VariableSet"));
    }
    if (NodeTypeLower == TEXT("comment"))
    {
        return Node->GetClass()->GetName().Contains(TEXT("Comment"));
    }

    return Node->GetClass()->GetName().Contains(NodeType);
}

bool FBlueprintMetadataBuilderService::MatchesEventTypeFilter(UEdGraphNode* Node, const FString& EventType) const
{
    if (EventType.IsEmpty()) return true;

    UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
    if (!EventNode) return false;

    FName EventFuncName = EventNode->GetFunctionName();
    FString EventName = EventFuncName.ToString();

    FString EventTypeLower = EventType.ToLower();

    if (EventTypeLower == TEXT("beginplay")) return EventName.Contains(TEXT("BeginPlay"));
    if (EventTypeLower == TEXT("tick")) return EventName.Contains(TEXT("Tick"));
    if (EventTypeLower == TEXT("endplay")) return EventName.Contains(TEXT("EndPlay"));
    if (EventTypeLower == TEXT("destroyed")) return EventName.Contains(TEXT("Destroyed"));
    if (EventTypeLower == TEXT("constructed") || EventTypeLower == TEXT("construct"))
    {
        return EventName.Contains(TEXT("Construct"));
    }

    return EventName.Contains(EventType);
}
