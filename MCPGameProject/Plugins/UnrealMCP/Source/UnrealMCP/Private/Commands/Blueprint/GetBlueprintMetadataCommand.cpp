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
    FString ParseError;

    if (!ParseParameters(Parameters, BlueprintName, Fields, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }

    UBlueprint* Blueprint = FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }

    TSharedPtr<FJsonObject> Metadata = BuildMetadata(Blueprint, Fields);
    return CreateSuccessResponse(Metadata);
}

FString FGetBlueprintMetadataCommand::GetCommandName() const
{
    return TEXT("get_blueprint_metadata");
}

bool FGetBlueprintMetadataCommand::ValidateParams(const FString& Parameters) const
{
    FString BlueprintName;
    TArray<FString> Fields;
    FString ParseError;
    return ParseParameters(Parameters, BlueprintName, Fields, ParseError);
}

bool FGetBlueprintMetadataCommand::ParseParameters(const FString& JsonString, FString& OutBlueprintName, TArray<FString>& OutFields, FString& OutError) const
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

    // Parse optional fields array
    const TArray<TSharedPtr<FJsonValue>>* FieldsArray;
    if (JsonObject->TryGetArrayField(TEXT("fields"), FieldsArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *FieldsArray)
        {
            OutFields.Add(Value->AsString());
        }
    }
    else
    {
        // Default to all fields
        OutFields.Add(TEXT("*"));
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

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildMetadata(UBlueprint* Blueprint, const TArray<FString>& Fields) const
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

    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());
        VarObj->SetBoolField(TEXT("instance_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
        VarObj->SetBoolField(TEXT("blueprint_read_only"), (Variable.PropertyFlags & CPF_BlueprintReadOnly) != 0);

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

        // Get inputs and outputs from the generated function (if compiled)
        if (Blueprint->GeneratedClass)
        {
            UFunction* Function = Blueprint->GeneratedClass->FindFunctionByName(Graph->GetFName());
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

TSharedPtr<FJsonObject> FGetBlueprintMetadataCommand::BuildComponentsInfo(UBlueprint* Blueprint) const
{
    TSharedPtr<FJsonObject> ComponentsInfo = MakeShared<FJsonObject>();

    // Use BlueprintService to get comprehensive component list (same as list_blueprint_components)
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
