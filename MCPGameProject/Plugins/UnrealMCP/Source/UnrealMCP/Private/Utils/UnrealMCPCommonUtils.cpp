#include "Utils/UnrealMCPCommonUtils.h"
#include "Utils/GraphUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Variable.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/StructOnScope.h"

// JSON Utilities - Delegated to FJsonUtils
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    return FJsonUtils::CreateErrorResponse(Message);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const FString& Message /* = TEXT("") */)
{
    return FJsonUtils::CreateSuccessResponse(Message);
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    FJsonUtils::GetIntArrayFromJson(JsonObject, FieldName, OutArray);
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    FJsonUtils::GetFloatArrayFromJson(JsonObject, FieldName, OutArray);
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    return FJsonUtils::GetVector2DFromJson(JsonObject, FieldName);
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    return FJsonUtils::GetVectorFromJson(JsonObject, FieldName);
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    return FJsonUtils::GetRotatorFromJson(JsonObject, FieldName);
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    // Early exit for empty names
    if (BlueprintName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Empty blueprint name provided"));
        return nullptr;
    }

    // Step 1: Normalize the path
    FString NormalizedName = BlueprintName;
    
    // Remove .uasset extension if present
    if (NormalizedName.EndsWith(TEXT(".uasset"), ESearchCase::IgnoreCase))
    {
        NormalizedName = NormalizedName.LeftChop(7);
    }
    
    // Handle absolute vs relative paths
    bool bIsAbsolutePath = NormalizedName.StartsWith(TEXT("/"));
    if (bIsAbsolutePath)
    {
        // If it's an absolute path starting with /Game/, use it directly
        if (NormalizedName.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Display, TEXT("Using absolute path: %s"), *NormalizedName);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *NormalizedName);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
        // If it starts with / but not /Game/, prepend /Game/
        else
        {
            NormalizedName = FString(TEXT("/Game")) + NormalizedName;
            UE_LOG(LogTemp, Display, TEXT("Converted to game path: %s"), *NormalizedName);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *NormalizedName);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
    }
    else
    {
        // For relative paths, extract any subdirectories
        FString SubPath;
        FString BaseName;
        if (NormalizedName.Contains(TEXT("/")))
        {
            NormalizedName.Split(TEXT("/"), &SubPath, &BaseName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            
            // Fix: Check if SubPath already starts with "Game"
            if (SubPath.StartsWith(TEXT("Game"), ESearchCase::IgnoreCase))
            {
                // Already has Game prefix, just add leading slash
                NormalizedName = FString::Printf(TEXT("/%s/%s"), *SubPath, *BaseName);
            }
            else
            {
                // Reconstruct with /Game/ prefix
                NormalizedName = FString::Printf(TEXT("/Game/%s/%s"), *SubPath, *BaseName);
            }
            
            UE_LOG(LogTemp, Display, TEXT("Reconstructed path with subdirectory: %s"), *NormalizedName);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *NormalizedName);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
        else
        {
            BaseName = NormalizedName;
        }

        // Try standard locations for relative paths
        TArray<FString> DefaultPaths = {
            FString::Printf(TEXT("/Game/Blueprints/%s"), *BaseName),
            FString::Printf(TEXT("/Game/%s"), *BaseName)
        };

        // Try each default path
        for (const FString& Path : DefaultPaths)
        {
            UE_LOG(LogTemp, Display, TEXT("Trying blueprint at path: %s"), *Path);
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
            if (Blueprint)
            {
                return Blueprint;
            }
        }
    }

    // If still not found, use asset registry for a thorough search
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AllBlueprintAssetData;
    
    // Create a filter for blueprints and widget blueprints
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;

    UE_LOG(LogTemp, Display, TEXT("Performing Asset Registry search for: %s"), *NormalizedName);
    AssetRegistryModule.Get().GetAssets(Filter, AllBlueprintAssetData);
    UE_LOG(LogTemp, Display, TEXT("Found %d total blueprint assets"), AllBlueprintAssetData.Num());

    // First try exact name match
    FString SearchName = FPaths::GetBaseFilename(NormalizedName);
    for (const FAssetData& Asset : AllBlueprintAssetData)
    {
        if (Asset.AssetName.ToString() == SearchName)
        {
            UE_LOG(LogTemp, Display, TEXT("Found exact match: %s"), *Asset.GetObjectPathString());
            return Cast<UBlueprint>(Asset.GetAsset());
        }
    }

    // If exact match fails, try case-insensitive match
    for (const FAssetData& Asset : AllBlueprintAssetData)
    {
        if (Asset.AssetName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found case-insensitive match: %s"), *Asset.GetObjectPathString());
            return Cast<UBlueprint>(Asset.GetAsset());
        }
    }

    UE_LOG(LogTemp, Error, TEXT("Blueprint '%s' not found after exhaustive search"), *BlueprintName);
    return nullptr;
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"),
                *EventName, *FGraphUtils::GetReliableNodeId(EventNode));
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"),
            *EventName, *FGraphUtils::GetReliableNodeId(EventNode));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableGetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();
        
        return VariableGetNode;
    }
    
    return nullptr;
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableSetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();
        
        return VariableSetNode;
    }
    
    return nullptr;
}



UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    return FGraphUtils::ConnectGraphNodes(Graph, SourceNode, SourcePinName, TargetNode, TargetPinName);
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    return FGraphUtils::FindPin(Node, PinName, Direction);
}

// Actor utilities
TSharedPtr<FJsonValue> FUnrealMCPCommonUtils::ActorToJson(AActor* Actor)
{
    return FActorUtils::ActorToJson(Actor);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    return FActorUtils::ActorToJsonObject(Actor, bDetailed);
}

UK2Node_Event* FUnrealMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    return FGraphUtils::FindExistingEventNode(Graph, EventName);
}

bool FUnrealMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName,
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    return FPropertyUtils::SetObjectProperty(Object, PropertyName, Value, OutErrorMessage);
}

bool FUnrealMCPCommonUtils::SetPropertyFromJson(FProperty* Property, void* ContainerPtr, const TSharedPtr<FJsonValue>& JsonValue)
{
    return FPropertyUtils::SetPropertyFromJson(Property, ContainerPtr, JsonValue);
}

// Example implementation for ParseVector (adjust as needed)
bool FUnrealMCPCommonUtils::ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector)
{
    return FGeometryUtils::ParseVector(JsonArray, OutVector);
}

bool FUnrealMCPCommonUtils::ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor)
{
    return FGeometryUtils::ParseLinearColor(JsonArray, OutColor);
}

bool FUnrealMCPCommonUtils::ParseRotator(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FRotator& OutRotator)
{
    return FGeometryUtils::ParseRotator(JsonArray, OutRotator);
}

AActor* FUnrealMCPCommonUtils::FindActorByName(const FString& ActorName)
{
    return FActorUtils::FindActorByName(ActorName);
}

bool FUnrealMCPCommonUtils::CallFunctionByName(UObject* Target, const FString& FunctionName, const TArray<FString>& StringParams, FString& OutError)
{
    return FActorUtils::CallFunctionByName(Target, FunctionName, StringParams, OutError);
}

// ==================== Asset Discovery Implementations ====================

TArray<FString> FUnrealMCPCommonUtils::FindAssetsByType(const FString& AssetType, const FString& SearchPath)
{
    return FAssetUtils::FindAssetsByType(AssetType, SearchPath);
}

TArray<FString> FUnrealMCPCommonUtils::FindAssetsByName(const FString& AssetName, const FString& SearchPath)
{
    return FAssetUtils::FindAssetsByName(AssetName, SearchPath);
}

TArray<FString> FUnrealMCPCommonUtils::FindWidgetBlueprints(const FString& WidgetName, const FString& SearchPath)
{
    return FAssetUtils::FindWidgetBlueprints(WidgetName, SearchPath);
}

TArray<FString> FUnrealMCPCommonUtils::FindBlueprints(const FString& BlueprintName, const FString& SearchPath)
{
    TArray<FString> FoundBlueprints;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Blueprints
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")));
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (BlueprintName.IsEmpty() || 
            AssetData.AssetName.ToString().Contains(BlueprintName, ESearchCase::IgnoreCase))
        {
            FoundBlueprints.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d blueprints matching '%s' in path '%s'"), FoundBlueprints.Num(), *BlueprintName, *SearchPath);
    return FoundBlueprints;
}

TArray<FString> FUnrealMCPCommonUtils::FindDataTables(const FString& TableName, const FString& SearchPath)
{
    TArray<FString> FoundTables;
    
    // Get the Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Create filter for Data Tables
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*SearchPath));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("DataTable")));
    
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);
    
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (TableName.IsEmpty() || 
            AssetData.AssetName.ToString().Contains(TableName, ESearchCase::IgnoreCase))
        {
            FoundTables.Add(AssetData.GetSoftObjectPath().ToString());
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d data tables matching '%s' in path '%s'"), FoundTables.Num(), *TableName, *SearchPath);
    return FoundTables;
}

UClass* FUnrealMCPCommonUtils::FindWidgetClass(const FString& WidgetPath)
{
    return FAssetUtils::FindWidgetClass(WidgetPath);
}

UBlueprint* FUnrealMCPCommonUtils::FindWidgetBlueprint(const FString& WidgetPath)
{
    return FAssetUtils::FindWidgetBlueprint(WidgetPath);
}

UObject* FUnrealMCPCommonUtils::FindAssetByPath(const FString& AssetPath)
{
    return FAssetUtils::FindAssetByPath(AssetPath);
}

UObject* FUnrealMCPCommonUtils::FindAssetByName(const FString& AssetName, const FString& AssetType)
{
    return FAssetUtils::FindAssetByName(AssetName, AssetType);
}

UScriptStruct* FUnrealMCPCommonUtils::FindStructType(const FString& StructPath)
{
    return FAssetUtils::FindStructType(StructPath);
}

TArray<FString> FUnrealMCPCommonUtils::GetCommonAssetSearchPaths(const FString& AssetName)
{
    return FAssetUtils::GetCommonAssetSearchPaths(AssetName);
}

FString FUnrealMCPCommonUtils::NormalizeAssetPath(const FString& AssetPath)
{
    return FAssetUtils::NormalizeAssetPath(AssetPath);
}

bool FUnrealMCPCommonUtils::IsValidAssetPath(const FString& AssetPath)
{
    return FAssetUtils::IsValidAssetPath(AssetPath);
}

// Blueprint Node Creation Functions
UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputActionNode: Graph is null"));
        return nullptr;
    }

    if (ActionName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputActionNode: ActionName is empty"));
        return nullptr;
    }

    // Create the proper UK2Node_InputAction node
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    if (!InputActionNode)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputActionNode: Failed to create UK2Node_InputAction"));
        return nullptr;
    }

    // Set the input action name
    InputActionNode->InputActionName = FName(*ActionName);
    
    // Set position
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    
    // Create new GUID for the node
    InputActionNode->CreateNewGuid();
    
    // Add the node to the graph
    Graph->AddNode(InputActionNode, true, true);
    
    // Post-placement setup
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    UE_LOG(LogTemp, Display, TEXT("CreateInputActionNode: Successfully created input action node for '%s'"), *ActionName);
    return InputActionNode;
}

// Blueprint node inspection utilities
UEdGraphNode* FUnrealMCPCommonUtils::FindNodeInBlueprint(UBlueprint* Blueprint, const FString& NodeName, const FString& GraphName)
{
    if (!Blueprint)
    {
        return nullptr;
    }

    // Get all graphs from the Blueprint
    TArray<UEdGraph*> GraphsToSearch = GetAllGraphsFromBlueprint(Blueprint);
    
    // If a specific graph is requested, filter to only that graph
    if (!GraphName.IsEmpty())
    {
        GraphsToSearch = GraphsToSearch.FilterByPredicate([&GraphName](UEdGraph* Graph)
        {
            return Graph && (Graph->GetName().Contains(GraphName) || 
                           Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase));
        });
    }

    // Search for the node in all relevant graphs
    for (UEdGraph* Graph : GraphsToSearch)
    {
        if (UEdGraphNode* FoundNode = FindNodeInGraph(Graph, NodeName))
        {
            return FoundNode;
        }
    }

    return nullptr;
}

UEdGraphNode* FUnrealMCPCommonUtils::FindNodeInGraph(UEdGraph* Graph, const FString& NodeName)
{
    if (!Graph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Check node title/display name
        FText NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle);
        if (NodeTitle.ToString().Contains(NodeName) || NodeTitle.ToString().Equals(NodeName, ESearchCase::IgnoreCase))
        {
            return Node;
        }

        // Check node class name
        FString NodeClassName = Node->GetClass()->GetName();
        if (NodeClassName.Contains(NodeName) || NodeClassName.Equals(NodeName, ESearchCase::IgnoreCase))
        {
            return Node;
        }

        // For function call nodes, check the function name
        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node))
        {
            if (UFunction* Function = FunctionNode->GetTargetFunction())
            {
                FString FunctionName = Function->GetName();
                if (FunctionName.Contains(NodeName) || FunctionName.Equals(NodeName, ESearchCase::IgnoreCase))
                {
                    return Node;
                }
            }
        }

        // For variable get/set nodes, check variable name
        if (UK2Node_Variable* VariableNode = Cast<UK2Node_Variable>(Node))
        {
            FString VariableName = VariableNode->GetVarName().ToString();
            if (VariableName.Contains(NodeName) || VariableName.Equals(NodeName, ESearchCase::IgnoreCase))
            {
                return Node;
            }
        }
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::GetNodePinInfoRuntime(UEdGraphNode* Node, const FString& PinName)
{
    TSharedPtr<FJsonObject> PinInfoObj = MakeShared<FJsonObject>();
    
    if (!Node)
    {
        return PinInfoObj;
    }

    // Find the requested pin
    UEdGraphPin* FoundPin = nullptr;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        // Check exact pin name match
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
        {
            FoundPin = Pin;
            break;
        }

        // Check pin display name if different from pin name
        FText DisplayName = Pin->PinFriendlyName;
        if (!DisplayName.IsEmpty() && DisplayName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
        {
            FoundPin = Pin;
            break;
        }
    }

    if (FoundPin)
    {
        // Get pin type information
        TSharedPtr<FJsonObject> PinTypeInfo = GetPinTypeInfo(FoundPin->PinType);
        
        PinInfoObj->SetStringField(TEXT("pin_type"), GetPinCategoryDisplayName(FoundPin->PinType.PinCategory));
        PinInfoObj->SetStringField(TEXT("expected_type"), FoundPin->PinType.PinSubCategory.ToString());
        PinInfoObj->SetStringField(TEXT("description"), FoundPin->PinToolTip.IsEmpty() ? TEXT("No description available") : FoundPin->PinToolTip);
        PinInfoObj->SetBoolField(TEXT("is_required"), true); // Most pins are required by default in UE
        PinInfoObj->SetBoolField(TEXT("is_input"), FoundPin->Direction == EGPD_Input);
        PinInfoObj->SetBoolField(TEXT("is_reference"), FoundPin->PinType.bIsReference);
        PinInfoObj->SetBoolField(TEXT("is_array"), FoundPin->PinType.IsArray());
        PinInfoObj->SetObjectField(TEXT("pin_type_details"), PinTypeInfo);
        
        // Add connection information
        PinInfoObj->SetNumberField(TEXT("linked_to_count"), FoundPin->LinkedTo.Num());
        
        TArray<TSharedPtr<FJsonValue>> LinkedPins;
        for (UEdGraphPin* LinkedPin : FoundPin->LinkedTo)
        {
            if (LinkedPin && LinkedPin->GetOwningNode())
            {
                TSharedPtr<FJsonObject> LinkedPinInfo = MakeShared<FJsonObject>();
                LinkedPinInfo->SetStringField(TEXT("node_name"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                LinkedPinInfo->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
                LinkedPins.Add(MakeShared<FJsonValueObject>(LinkedPinInfo));
            }
        }
        PinInfoObj->SetArrayField(TEXT("linked_to"), LinkedPins);
    }

    return PinInfoObj;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::GetPinTypeInfo(const FEdGraphPinType& PinType)
{
    TSharedPtr<FJsonObject> TypeInfo = MakeShared<FJsonObject>();
    
    TypeInfo->SetStringField(TEXT("category"), PinType.PinCategory.ToString());
    TypeInfo->SetStringField(TEXT("subcategory"), PinType.PinSubCategory.ToString());
    TypeInfo->SetBoolField(TEXT("is_array"), PinType.IsArray());
    TypeInfo->SetBoolField(TEXT("is_reference"), PinType.bIsReference);
    TypeInfo->SetBoolField(TEXT("is_const"), PinType.bIsConst);
    TypeInfo->SetBoolField(TEXT("is_weak_pointer"), PinType.bIsWeakPointer);
    
    // Add container type information if it's a container
    if (PinType.IsContainer())
    {
        TypeInfo->SetStringField(TEXT("container_type"), TEXT("Container")); // Simplified for UE 5.7
        TypeInfo->SetStringField(TEXT("value_category"), TEXT("Unknown"));
        TypeInfo->SetStringField(TEXT("value_subcategory"), TEXT("Unknown"));
    }

    // Add object information if it's an object type
    if (PinType.PinSubCategoryObject.IsValid())
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
        {
            TypeInfo->SetStringField(TEXT("object_class"), Class->GetName());
            TypeInfo->SetStringField(TEXT("object_class_path"), Class->GetPathName());
        }
        else if (UScriptStruct* Struct = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()))
        {
            TypeInfo->SetStringField(TEXT("struct_name"), Struct->GetName());
            TypeInfo->SetStringField(TEXT("struct_path"), Struct->GetPathName());
        }
    }

    return TypeInfo;
}

FString FUnrealMCPCommonUtils::GetPinCategoryDisplayName(const FName& Category)
{
    if (Category == UEdGraphSchema_K2::PC_Boolean)
    {
        return TEXT("bool");
    }
    else if (Category == UEdGraphSchema_K2::PC_Byte)
    {
        return TEXT("byte");
    }
    else if (Category == UEdGraphSchema_K2::PC_Int)
    {
        return TEXT("int");
    }
    else if (Category == UEdGraphSchema_K2::PC_Int64)
    {
        return TEXT("int64");
    }
    else if (Category == UEdGraphSchema_K2::PC_Real)
    {
        return TEXT("real");
    }
    else if (Category == UEdGraphSchema_K2::PC_Double)
    {
        return TEXT("double");
    }
    else if (Category == UEdGraphSchema_K2::PC_String)
    {
        return TEXT("string");
    }
    else if (Category == UEdGraphSchema_K2::PC_Text)
    {
        return TEXT("text");
    }
    else if (Category == UEdGraphSchema_K2::PC_Name)
    {
        return TEXT("name");
    }
    else if (Category == UEdGraphSchema_K2::PC_Object)
    {
        return TEXT("object");
    }
    else if (Category == UEdGraphSchema_K2::PC_Class)
    {
        return TEXT("class");
    }
    else if (Category == UEdGraphSchema_K2::PC_Struct)
    {
        return TEXT("struct");
    }
    else if (Category == UEdGraphSchema_K2::PC_Exec)
    {
        return TEXT("exec");
    }
    else if (Category == UEdGraphSchema_K2::PC_Wildcard)
    {
        return TEXT("wildcard");
    }
    
    return Category.ToString();
}

TArray<UEdGraph*> FUnrealMCPCommonUtils::GetAllGraphsFromBlueprint(UBlueprint* Blueprint)
{
    TArray<UEdGraph*> AllGraphs;
    
    if (!Blueprint)
    {
        return AllGraphs;
    }

    // Add all UbergraphPages (event graphs)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }

    // Add all function graphs
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }

    // Add macro graphs if any
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }

    return AllGraphs;
}