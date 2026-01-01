// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/BlueprintAction/BlueprintNodePinInfoService.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectIterator.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "UObject/UnrealType.h"
#include "EdGraphSchema_K2.h"

FBlueprintNodePinInfoService::FBlueprintNodePinInfoService()
{
}

FBlueprintNodePinInfoService::~FBlueprintNodePinInfoService()
{
}

FString FBlueprintNodePinInfoService::GetNodePinInfo(
    const FString& NodeName,
    const FString& PinName,
    const FString& ClassName
)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Looking for pin '%s' on node '%s' using runtime inspection"), *PinName, *NodeName);
    
    // Try to find node in currently loaded Blueprints
    UEdGraphNode* FoundNode = nullptr;
    
    // Search for the node in all loaded Blueprints using runtime inspection
    TArray<UBlueprint*> LoadedBlueprints;
    
    // Get all loaded Blueprint assets
    for (TObjectIterator<UBlueprint> BlueprintItr; BlueprintItr; ++BlueprintItr)
    {
        if (UBlueprint* Blueprint = *BlueprintItr)
        {
            LoadedBlueprints.Add(Blueprint);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Searching through %d loaded Blueprints"), LoadedBlueprints.Num());

    // Search for the node across all loaded Blueprints
    for (UBlueprint* Blueprint : LoadedBlueprints)
    {
        FoundNode = FUnrealMCPCommonUtils::FindNodeInBlueprint(Blueprint, NodeName);
        if (FoundNode)
        {
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Found node '%s' in Blueprint '%s'"), *NodeName, *Blueprint->GetName());
            break;
        }
    }

    if (FoundNode)
    {
        // Get pin information using runtime inspection
        TSharedPtr<FJsonObject> PinInfo = FUnrealMCPCommonUtils::GetNodePinInfoRuntime(FoundNode, PinName);
        
        if (PinInfo->HasField(TEXT("pin_type")))
        {
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), PinInfo);
            ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found pin information for '%s' on node '%s' using runtime inspection"), *PinName, *NodeName));
        }
        else
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), MakeShared<FJsonObject>());
            ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Pin '%s' not found on node '%s'"), *PinName, *NodeName));
            
            // Provide available pins for this node
            TArray<TSharedPtr<FJsonValue>> AvailablePins;
            for (UEdGraphPin* Pin : FoundNode->Pins)
            {
                if (Pin)
                {
                    AvailablePins.Add(MakeShared<FJsonValueString>(Pin->PinName.ToString()));
                }
            }
            ResultObj->SetArrayField(TEXT("available_pins"), AvailablePins);
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Provided %d available pins for node '%s'"), AvailablePins.Num(), *NodeName);
        }
    }
    else
    {
        // Node not found in loaded Blueprints - try looking up as a library function
        UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Node '%s' not found in loaded Blueprints, trying library function lookup"), *NodeName);

        TSharedPtr<FJsonObject> LibraryPinInfo;
        TArray<FString> AvailablePins;

        if (GetLibraryFunctionPinInfo(NodeName, PinName, ClassName, LibraryPinInfo, &AvailablePins))
        {
            // Found in library functions
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), LibraryPinInfo);
            ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found pin information for '%s' on library function '%s'"), *PinName, *NodeName));
            ResultObj->SetStringField(TEXT("source"), TEXT("library_function"));
        }
        else if (AvailablePins.Num() > 0)
        {
            // Found the function but not the specific pin
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), MakeShared<FJsonObject>());
            ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Pin '%s' not found on library function '%s'"), *PinName, *NodeName));

            TArray<TSharedPtr<FJsonValue>> PinArray;
            for (const FString& Pin : AvailablePins)
            {
                PinArray.Add(MakeShared<FJsonValueString>(Pin));
            }
            ResultObj->SetArrayField(TEXT("available_pins"), PinArray);
            ResultObj->SetStringField(TEXT("hint"), TEXT("For container functions (Map_Add, Array_Add), pin names are: TargetMap/TargetArray, Key, Value, ReturnValue. Note that wildcard pins resolve their type when connected."));
        }
        else
        {
            // Not found anywhere
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("node_name"), NodeName);
            ResultObj->SetStringField(TEXT("pin_name"), PinName);
            ResultObj->SetObjectField(TEXT("pin_info"), MakeShared<FJsonObject>());
            ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("Node '%s' not found in loaded Blueprints or library functions"), *NodeName));

            // Provide list of available node types from loaded Blueprints
            TArray<TSharedPtr<FJsonValue>> AvailableNodes;
            TSet<FString> UniqueNodeNames;

            for (UBlueprint* Blueprint : LoadedBlueprints)
            {
                TArray<UEdGraph*> AllGraphs = FUnrealMCPCommonUtils::GetAllGraphsFromBlueprint(Blueprint);
                for (UEdGraph* Graph : AllGraphs)
                {
                    if (Graph)
                    {
                        for (UEdGraphNode* Node : Graph->Nodes)
                        {
                            if (Node)
                            {
                                FString NodeDisplayName = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                                if (!NodeDisplayName.IsEmpty() && !UniqueNodeNames.Contains(NodeDisplayName))
                                {
                                    UniqueNodeNames.Add(NodeDisplayName);
                                    AvailableNodes.Add(MakeShared<FJsonValueString>(NodeDisplayName));

                                    // Limit the number of examples to avoid overly large responses
                                    if (AvailableNodes.Num() >= 50)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        if (AvailableNodes.Num() >= 50)
                        {
                            break;
                        }
                    }
                }
                if (AvailableNodes.Num() >= 50)
                {
                    break;
                }
            }

            ResultObj->SetArrayField(TEXT("available_nodes"), AvailableNodes);
            ResultObj->SetStringField(TEXT("hint"), TEXT("Try specifying class_name parameter for library functions (e.g., class_name='BlueprintMapLibrary' for Map_Add)"));
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Provided %d example node names from loaded Blueprints"), AvailableNodes.Num());
        }
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Returning JSON response: %s"), *OutputString);
    
    return OutputString;
}

FString FBlueprintNodePinInfoService::BuildPinInfoResult(
    bool bSuccess,
    const FString& Message,
    TSharedPtr<FJsonObject> PinInfo
)
{
    // TODO: Move JSON result building for pin info
    // Pattern used throughout GetNodePinInfo
    
    return FString();
}

bool FBlueprintNodePinInfoService::GetKnownPinInfo(
    const FString& NodeName,
    const FString& PinName,
    TSharedPtr<FJsonObject>& OutPinInfo
)
{
    // TODO: Move the hardcoded pin info database (lines ~1190-1280)
    // This is a large switch/if-else structure with pin information for:
    // - Create Widget node
    // - Get Controller node
    // - Cast nodes
    // - etc.
    
    return false;
}

TArray<FString> FBlueprintNodePinInfoService::GetAvailablePinsForNode(const FString& NodeName)
{
    // TODO: Move logic to list available pins (lines ~1297-1305)
    // Returns array of known pin names for a given node

    TArray<FString> AvailablePins;
    return AvailablePins;
}

bool FBlueprintNodePinInfoService::GetLibraryFunctionPinInfo(
    const FString& FunctionName,
    const FString& PinName,
    const FString& ClassName,
    TSharedPtr<FJsonObject>& OutPinInfo,
    TArray<FString>* OutAvailablePins
)
{
    UE_LOG(LogTemp, Warning, TEXT("GetLibraryFunctionPinInfo: Looking for function '%s' pin '%s' class '%s'"),
           *FunctionName, *PinName, *ClassName);

    // Normalize function name for matching (handle "Map Add" vs "Map_Add" vs "Add")
    FString NormalizedName = FunctionName;
    NormalizedName.ReplaceInline(TEXT(" "), TEXT("_"));

    // Also create a variant without underscores for display name matching
    FString DisplayName = FunctionName;
    DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));

    FString SearchLower = NormalizedName.ToLower();
    FString DisplayLower = DisplayName.ToLower();
    FString ClassNameLower = ClassName.ToLower();

    // Search the Blueprint Action Database
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();

    UFunction* FoundFunction = nullptr;
    FString FoundClassName;

    for (auto Iterator(ActionRegistry.CreateConstIterator()); Iterator; ++Iterator)
    {
        const FBlueprintActionDatabase::FActionList& ActionList = Iterator.Value();
        for (UBlueprintNodeSpawner* NodeSpawner : ActionList)
        {
            if (!NodeSpawner)
            {
                continue;
            }

            // Check if this is a function spawner
            UBlueprintFunctionNodeSpawner* FunctionSpawner = Cast<UBlueprintFunctionNodeSpawner>(NodeSpawner);
            if (!FunctionSpawner)
            {
                continue;
            }

            UFunction const* Function = FunctionSpawner->GetFunction();
            if (!Function)
            {
                continue;
            }

            FString FuncName = Function->GetName();
            FString FuncDisplayName = Function->GetDisplayNameText().ToString();
            FString FuncClass = Function->GetOwnerClass()->GetName();

            FString FuncNameLower = FuncName.ToLower();
            FString FuncDisplayLower = FuncDisplayName.ToLower();
            FString FuncClassLower = FuncClass.ToLower();

            // Check if function name matches
            bool bNameMatches = FuncNameLower == SearchLower ||
                               FuncNameLower.Contains(SearchLower) ||
                               FuncDisplayLower == DisplayLower ||
                               FuncDisplayLower.Contains(DisplayLower);

            // Check if class name matches (if specified)
            bool bClassMatches = ClassName.IsEmpty() ||
                                FuncClassLower == ClassNameLower ||
                                FuncClassLower.Contains(ClassNameLower);

            if (bNameMatches && bClassMatches)
            {
                UE_LOG(LogTemp, Warning, TEXT("GetLibraryFunctionPinInfo: Found matching function '%s' in class '%s'"),
                       *FuncName, *FuncClass);

                FoundFunction = const_cast<UFunction*>(Function);
                FoundClassName = FuncClass;
                break;
            }
        }

        if (FoundFunction)
        {
            break;
        }
    }

    if (!FoundFunction)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetLibraryFunctionPinInfo: Function '%s' not found in Blueprint Action Database"), *FunctionName);
        return false;
    }

    // Collect all pin names from the function
    TArray<FString> AllPinNames;
    AllPinNames.Add(TEXT("execute"));  // Input exec pin
    AllPinNames.Add(TEXT("then"));     // Output exec pin

    // Iterate through function parameters
    FProperty* FoundProperty = nullptr;
    bool bIsReturnValue = false;

    for (TFieldIterator<FProperty> PropIt(FoundFunction); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropName = Property->GetName();
        AllPinNames.Add(PropName);

        UE_LOG(LogTemp, Warning, TEXT("GetLibraryFunctionPinInfo: Found parameter '%s'"), *PropName);

        // Check if this is the pin we're looking for
        if (PropName.Equals(PinName, ESearchCase::IgnoreCase))
        {
            FoundProperty = Property;
            bIsReturnValue = Property->HasAnyPropertyFlags(CPF_ReturnParm);
        }
    }

    // Also check for return value
    if (FProperty* ReturnProp = FoundFunction->GetReturnProperty())
    {
        AllPinNames.Add(TEXT("ReturnValue"));
        if (PinName.Equals(TEXT("ReturnValue"), ESearchCase::IgnoreCase))
        {
            FoundProperty = ReturnProp;
            bIsReturnValue = true;
        }
    }

    // Output available pins
    if (OutAvailablePins)
    {
        *OutAvailablePins = AllPinNames;
    }

    // If we didn't find the specific pin
    if (!FoundProperty)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetLibraryFunctionPinInfo: Pin '%s' not found on function '%s'. Available: %s"),
               *PinName, *FunctionName, *FString::Join(AllPinNames, TEXT(", ")));
        return false;
    }

    // Build pin info from the property
    OutPinInfo = BuildPinInfoFromFunctionParam(FoundFunction, FoundProperty, bIsReturnValue);
    OutPinInfo->SetStringField(TEXT("class_name"), FoundClassName);

    return true;
}

TSharedPtr<FJsonObject> FBlueprintNodePinInfoService::BuildPinInfoFromFunctionParam(
    UFunction* Function,
    FProperty* Property,
    bool bIsReturnValue
)
{
    TSharedPtr<FJsonObject> PinInfo = MakeShared<FJsonObject>();

    if (!Property)
    {
        return PinInfo;
    }

    FString PropName = Property->GetName();
    bool bIsInput = !bIsReturnValue && !Property->HasAnyPropertyFlags(CPF_OutParm);
    bool bIsReference = Property->HasAnyPropertyFlags(CPF_ReferenceParm | CPF_OutParm);
    bool bIsConst = Property->HasAnyPropertyFlags(CPF_ConstParm);

    // Determine pin type category
    FString PinType = TEXT("unknown");
    FString ExpectedType = TEXT("");
    FString Description = TEXT("");
    bool bIsWildcard = false;

    // Check for different property types
    if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
    {
        PinType = TEXT("object");
        if (ObjProp->PropertyClass)
        {
            ExpectedType = ObjProp->PropertyClass->GetName();
        }
    }
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        PinType = TEXT("class");
        if (ClassProp->MetaClass)
        {
            ExpectedType = FString::Printf(TEXT("Class<%s>"), *ClassProp->MetaClass->GetName());
        }
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        PinType = TEXT("struct");
        if (StructProp->Struct)
        {
            ExpectedType = StructProp->Struct->GetName();
        }
    }
    else if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        PinType = TEXT("map");
        // Check if it's a wildcard (template) map
        if (MapProp->KeyProp && MapProp->ValueProp)
        {
            FString KeyType = MapProp->KeyProp->GetCPPType();
            FString ValueType = MapProp->ValueProp->GetCPPType();
            ExpectedType = FString::Printf(TEXT("Map<%s, %s>"), *KeyType, *ValueType);

            // Check for wildcard types
            if (KeyType.Contains(TEXT("Wildcard")) || ValueType.Contains(TEXT("Wildcard")))
            {
                bIsWildcard = true;
            }
        }
        Description = TEXT("Map container - type resolves when connected to typed Map variable");
    }
    else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        PinType = TEXT("array");
        if (ArrayProp->Inner)
        {
            ExpectedType = FString::Printf(TEXT("Array<%s>"), *ArrayProp->Inner->GetCPPType());
        }
        Description = TEXT("Array container - type resolves when connected to typed Array variable");
    }
    else if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        PinType = TEXT("set");
        if (SetProp->ElementProp)
        {
            ExpectedType = FString::Printf(TEXT("Set<%s>"), *SetProp->ElementProp->GetCPPType());
        }
        Description = TEXT("Set container - type resolves when connected to typed Set variable");
    }
    else if (CastField<FIntProperty>(Property))
    {
        PinType = TEXT("int");
        ExpectedType = TEXT("Integer");
    }
    else if (CastField<FFloatProperty>(Property) || CastField<FDoubleProperty>(Property))
    {
        PinType = TEXT("real");
        ExpectedType = TEXT("Float");
    }
    else if (CastField<FBoolProperty>(Property))
    {
        PinType = TEXT("bool");
        ExpectedType = TEXT("Boolean");
    }
    else if (CastField<FStrProperty>(Property))
    {
        PinType = TEXT("string");
        ExpectedType = TEXT("String");
    }
    else if (CastField<FNameProperty>(Property))
    {
        PinType = TEXT("name");
        ExpectedType = TEXT("Name");
    }
    else if (CastField<FTextProperty>(Property))
    {
        PinType = TEXT("text");
        ExpectedType = TEXT("Text");
    }
    else
    {
        // Check for wildcard/generic params (often used in template functions)
        FString CPPType = Property->GetCPPType();
        if (CPPType.Contains(TEXT("Wildcard")) || CPPType.Contains(TEXT("Template")))
        {
            PinType = TEXT("wildcard");
            bIsWildcard = true;
            ExpectedType = TEXT("Any (resolves on connection)");
            Description = TEXT("Wildcard pin - type is determined when connected to a typed pin");
        }
    }

    // Build the pin info object
    PinInfo->SetStringField(TEXT("pin_name"), PropName);
    PinInfo->SetStringField(TEXT("pin_type"), PinType);
    PinInfo->SetStringField(TEXT("expected_type"), ExpectedType);
    PinInfo->SetBoolField(TEXT("is_input"), bIsInput);
    // Note: In Blueprint, all function parameters are typically required
    // Optional parameters would need metadata inspection, but for library functions they're usually required
    PinInfo->SetBoolField(TEXT("is_required"), true);
    PinInfo->SetBoolField(TEXT("is_reference"), bIsReference);
    PinInfo->SetBoolField(TEXT("is_const"), bIsConst);
    PinInfo->SetBoolField(TEXT("is_wildcard"), bIsWildcard);

    if (!Description.IsEmpty())
    {
        PinInfo->SetStringField(TEXT("description"), Description);
    }
    else if (bIsReference)
    {
        PinInfo->SetStringField(TEXT("description"), FString::Printf(TEXT("Reference parameter - modifies %s in-place"), *PropName));
    }

    // Add hint for wildcard pins
    if (bIsWildcard)
    {
        PinInfo->SetStringField(TEXT("hint"), TEXT("Connect your typed variable FIRST to resolve the wildcard type, then connect other pins."));
    }

    UE_LOG(LogTemp, Warning, TEXT("BuildPinInfoFromFunctionParam: Built info for '%s' - type=%s, expected=%s, isInput=%d, isRef=%d, isWildcard=%d"),
           *PropName, *PinType, *ExpectedType, bIsInput, bIsReference, bIsWildcard);

    return PinInfo;
}
