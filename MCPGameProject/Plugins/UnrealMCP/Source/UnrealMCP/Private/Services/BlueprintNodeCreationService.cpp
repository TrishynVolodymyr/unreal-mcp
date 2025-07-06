#include "Services/BlueprintNodeCreationService.h"
#include "Services/MacroDiscoveryService.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "BlueprintNodeBinder.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Framework/Commands/UIAction.h"
#include "Engine/Engine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_ComponentBoundEvent.h"

FBlueprintNodeCreationService::FBlueprintNodeCreationService()
{
}

FString FBlueprintNodeCreationService::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    // Parse JSON parameters
    TSharedPtr<FJsonObject> ParamsObject;
    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: JsonParams = '%s'"), *JsonParams);
    if (!ParseJsonParameters(JsonParams, ParamsObject, ResultObj))
    {
        // Return the specific error details that were already set by ParseJsonParameters
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = FindBlueprintByName(BlueprintName);
    if (!Blueprint)
    {
        return BuildNodeResult(false, FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Get the event graph
    UEdGraph* EventGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetFName() == "EventGraph")
        {
            EventGraph = Graph;
            break;
        }
    }
    
    if (!EventGraph)
    {
        return BuildNodeResult(false, TEXT("Could not find EventGraph in blueprint"));
    }
    
    // Parse node position
    int32 PositionX, PositionY;
    ParseNodePosition(NodePosition, PositionX, PositionY);
    
    // Log the creation attempt
    LogNodeCreationAttempt(FunctionName, BlueprintName, ClassName, PositionX, PositionY);
    
    UEdGraphNode* NewNode = nullptr;
    FString NodeTitle = TEXT("Unknown");
    FString NodeType = TEXT("Unknown");
    UClass* TargetClass = nullptr;
    
    // Check if this is a control flow node request
    if (FunctionName.Equals(TEXT("Branch"), ESearchCase::IgnoreCase) || 
        FunctionName.Equals(TEXT("IfThenElse"), ESearchCase::IgnoreCase) ||
        FunctionName.Equals(TEXT("UK2Node_IfThenElse"), ESearchCase::IgnoreCase))
    {
        UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
        BranchNode->NodePosX = PositionX;
        BranchNode->NodePosY = PositionY;
        BranchNode->CreateNewGuid();
        EventGraph->AddNode(BranchNode, true, true);
        BranchNode->PostPlacedNewNode();
        BranchNode->AllocateDefaultPins();
        NewNode = BranchNode;
        NodeTitle = TEXT("Branch");
        NodeType = TEXT("UK2Node_IfThenElse");
    }
    else if (FunctionName.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ExecutionSequence"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("UK2Node_ExecutionSequence"), ESearchCase::IgnoreCase))
    {
        UK2Node_ExecutionSequence* SequenceNode = NewObject<UK2Node_ExecutionSequence>(EventGraph);
        SequenceNode->NodePosX = PositionX;
        SequenceNode->NodePosY = PositionY;
        SequenceNode->CreateNewGuid();
        EventGraph->AddNode(SequenceNode, true, true);
        SequenceNode->PostPlacedNewNode();
        SequenceNode->AllocateDefaultPins();
        NewNode = SequenceNode;
        NodeTitle = TEXT("Sequence");
        NodeType = TEXT("UK2Node_ExecutionSequence");
    }
    else if (FunctionName.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Custom Event"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("UK2Node_CustomEvent"), ESearchCase::IgnoreCase))
    {
        UK2Node_CustomEvent* CustomEventNode = NewObject<UK2Node_CustomEvent>(EventGraph);
        
        // Set custom event name from parameters if provided
        FString EventName = TEXT("CustomEvent"); // Default name
        if (ParamsObject.IsValid())
        {
            FString ParamEventName;
            if (ParamsObject->TryGetStringField(TEXT("event_name"), ParamEventName) && !ParamEventName.IsEmpty())
            {
                EventName = ParamEventName;
            }
        }
        
        CustomEventNode->CustomFunctionName = FName(*EventName);
        CustomEventNode->NodePosX = PositionX;
        CustomEventNode->NodePosY = PositionY;
        CustomEventNode->CreateNewGuid();
        EventGraph->AddNode(CustomEventNode, true, true);
        CustomEventNode->PostPlacedNewNode();
        CustomEventNode->AllocateDefaultPins();
        NewNode = CustomEventNode;
        NodeTitle = EventName;
        NodeType = TEXT("UK2Node_CustomEvent");
    }
    else if (FunctionName.Equals(TEXT("Cast"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("UK2Node_DynamicCast"), ESearchCase::IgnoreCase))
    {
        UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(EventGraph);
        
        // Set target type if provided in parameters
        if (ParamsObject.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: ParamsObject is valid for Cast node"));
            FString TargetTypeName;
            
            // Check if target_type is in kwargs sub-object
            const TSharedPtr<FJsonObject>* KwargsObject;
            if (ParamsObject->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found kwargs object"));
                if ((*KwargsObject)->TryGetStringField(TEXT("target_type"), TargetTypeName) && !TargetTypeName.IsEmpty())
                {
                    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found target_type in kwargs: '%s'"), *TargetTypeName);
                }
            }
            // Also check at root level for backwards compatibility
            else if (ParamsObject->TryGetStringField(TEXT("target_type"), TargetTypeName) && !TargetTypeName.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found target_type parameter: '%s'"), *TargetTypeName);
            }
            
            if (!TargetTypeName.IsEmpty())
            {
                // Find the class to cast to
                UClass* CastTargetClass = nullptr;
                
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Looking for target type '%s'"), *TargetTypeName);
                
                // Common class mappings
                if (TargetTypeName.Equals(TEXT("PlayerController"), ESearchCase::IgnoreCase))
                {
                    CastTargetClass = APlayerController::StaticClass();
                    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found PlayerController class"));
                }
                else if (TargetTypeName.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase))
                {
                    CastTargetClass = APawn::StaticClass();
                    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found Pawn class"));
                }
                else if (TargetTypeName.Equals(TEXT("Actor"), ESearchCase::IgnoreCase))
                {
                    CastTargetClass = AActor::StaticClass();
                    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found Actor class"));
                }
                else
                {
                    // Try to find the class by name
                    CastTargetClass = UClass::TryFindTypeSlow<UClass>(TargetTypeName);
                    if (!CastTargetClass)
                    {
                        // Try with /Script/Engine. prefix
                        FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *TargetTypeName);
                        CastTargetClass = LoadClass<UObject>(nullptr, *EnginePath);
                    }
                    
                    // If still not found, try to find it as a Blueprint class
                    if (!CastTargetClass)
                    {
                        FAssetRegistryModule& AssetReg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                        TArray<FAssetData> BPAssets;
                        AssetReg.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BPAssets);
                        
                        for (const FAssetData& AssetData : BPAssets)
                        {
                            FString AssetName = AssetData.AssetName.ToString();
                            
                            // Try exact match first (most reliable)
                            bool bIsMatch = AssetName.Equals(TargetTypeName, ESearchCase::IgnoreCase);
                            
                            // If no exact match, try matching against the generated class name
                            if (!bIsMatch)
                            {
                                if (UBlueprint* TestBP = Cast<UBlueprint>(AssetData.GetAsset()))
                                {
                                    if (TestBP->GeneratedClass)
                                    {
                                        FString ClassName = TestBP->GeneratedClass->GetName();
                                        // Remove common Blueprint prefixes for comparison
                                        if (ClassName.StartsWith(TEXT("BP_")))
                                        {
                                            ClassName = ClassName.Mid(3);
                                        }
                                        bIsMatch = ClassName.Equals(TargetTypeName, ESearchCase::IgnoreCase);
                                    }
                                }
                            }
                            
                            if (bIsMatch)
                            {
                                if (UBlueprint* TargetBP = Cast<UBlueprint>(AssetData.GetAsset()))
                                {
                                    CastTargetClass = TargetBP->GeneratedClass;
                                    if (CastTargetClass)
                                    {
                                        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found Blueprint class '%s' (matched asset '%s')"), *CastTargetClass->GetName(), *AssetName);
                                        break;
                                    }
                                    else
                                    {
                                        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Blueprint '%s' has null GeneratedClass"), *AssetName);
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (CastTargetClass)
                {
                    CastNode->TargetType = CastTargetClass;
                    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Set cast target type to '%s'"), *CastTargetClass->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find target type '%s'"), *TargetTypeName);
                }
            }
        }
        
        CastNode->NodePosX = PositionX;
        CastNode->NodePosY = PositionY;
        CastNode->CreateNewGuid();
        EventGraph->AddNode(CastNode, true, true);
        CastNode->PostPlacedNewNode();
        CastNode->AllocateDefaultPins();
        NewNode = CastNode;
        NodeTitle = TEXT("Cast");
        NodeType = TEXT("UK2Node_DynamicCast");
    }
    // Handle standard event nodes (BeginPlay, Tick, etc.)
    else if (FunctionName.StartsWith(TEXT("Receive")) || 
             FunctionName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("EndPlay"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("BeginDestroy"), ESearchCase::IgnoreCase))
    {
        // Create standard event node
        UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);
        
        // Determine the correct event name and parent class
        FString EventName = FunctionName;
        FString ParentClassName = TEXT("/Script/Engine.Actor");
        
        // Map common event names to their proper "Receive" format
        if (FunctionName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveBeginPlay");
        }
        else if (FunctionName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveTick");
        }
        else if (FunctionName.Equals(TEXT("EndPlay"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveEndPlay");
        }
        else if (FunctionName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveActorBeginOverlap");
        }
        else if (FunctionName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveActorEndOverlap");
        }
        else if (FunctionName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveHit");
        }
        else if (FunctionName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveDestroyed");
        }
        else if (FunctionName.Equals(TEXT("BeginDestroy"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveBeginDestroy");
        }
        
        // Set up the EventReference structure
        EventNode->EventReference.SetExternalMember(*EventName, UClass::TryFindTypeSlow<UClass>(*ParentClassName));
        if (!EventNode->EventReference.GetMemberParentClass())
        {
            // Fallback to Actor class if the specific class wasn't found
            EventNode->EventReference.SetExternalMember(*EventName, AActor::StaticClass());
        }
        
        // Override function - this makes it a Blueprint implementable event
        EventNode->bOverrideFunction = true;
        
        EventNode->NodePosX = PositionX;
        EventNode->NodePosY = PositionY;
        EventNode->CreateNewGuid();
        EventGraph->AddNode(EventNode, true, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        
        NewNode = EventNode;
        NodeTitle = EventName;
        NodeType = TEXT("UK2Node_Event");
        
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Created event node '%s'"), *EventName);
    }
    // Handle macro functions using the Macro Discovery Service
    else if (FMacroDiscoveryService::IsMacroFunction(FunctionName))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Processing macro function '%s' using MacroDiscoveryService"), *FunctionName);
        
        // Use the macro discovery service to find the macro blueprint dynamically
        FString MacroGraphName = FMacroDiscoveryService::MapFunctionNameToMacroGraphName(FunctionName);
        UBlueprint* MacroBlueprint = FMacroDiscoveryService::FindMacroBlueprint(FunctionName);
        
        if (MacroBlueprint)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found macro blueprint for '%s' via discovery service"), *FunctionName);
            
            // Find the specific macro graph
            UEdGraph* TargetMacroGraph = FMacroDiscoveryService::FindMacroGraph(MacroBlueprint, MacroGraphName);
            
            if (TargetMacroGraph)
            {
                // Create macro instance
                UK2Node_MacroInstance* MacroInstance = NewObject<UK2Node_MacroInstance>(EventGraph);
                MacroInstance->SetMacroGraph(TargetMacroGraph);
                MacroInstance->NodePosX = PositionX;
                MacroInstance->NodePosY = PositionY;
                MacroInstance->CreateNewGuid();
                EventGraph->AddNode(MacroInstance, true, true);
                MacroInstance->PostPlacedNewNode();
                MacroInstance->AllocateDefaultPins();
                
                NewNode = MacroInstance;
                NodeTitle = FunctionName;
                NodeType = TEXT("UK2Node_MacroInstance");
                
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created macro instance for '%s' using discovery service"), *FunctionName);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName);
                return BuildNodeResult(false, FString::Printf(TEXT("Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not discover macro blueprint for '%s'"), *FunctionName);
            return BuildNodeResult(false, FString::Printf(TEXT("Could not discover macro blueprint for '%s'. Macro may not be available."), *FunctionName));
        }
    }
    // Variable getter/setter node creation
    else if (FunctionName.StartsWith(TEXT("Get ")) || FunctionName.StartsWith(TEXT("Set ")) ||
        FunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase) ||
        FunctionName.Equals(TEXT("UK2Node_VariableSet"), ESearchCase::IgnoreCase))
    {
        FString VarName = FunctionName;
        bool bIsGetter = false;
        if (VarName.StartsWith(TEXT("Get ")))
        {
            VarName = VarName.RightChop(4);
            bIsGetter = true;
        }
        else if (VarName.StartsWith(TEXT("Set ")))
        {
            VarName = VarName.RightChop(4);
        }
        
        // Try to find the variable or component in the Blueprint
        bool bFound = false;
        
        // First check user-defined variables
        for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
        {
            if (VarDesc.VarName.ToString().Equals(VarName, ESearchCase::IgnoreCase))
            {
                if (bIsGetter)
                {
                    UK2Node_VariableGet* GetterNode = NewObject<UK2Node_VariableGet>(EventGraph);
                    GetterNode->VariableReference.SetSelfMember(*VarName);
                    GetterNode->NodePosX = PositionX;
                    GetterNode->NodePosY = PositionY;
                    GetterNode->CreateNewGuid();
                    EventGraph->AddNode(GetterNode, true, true);
                    GetterNode->PostPlacedNewNode();
                    GetterNode->AllocateDefaultPins();
                    NewNode = GetterNode;
                    NodeTitle = FString::Printf(TEXT("Get %s"), *VarName);
                    NodeType = TEXT("UK2Node_VariableGet");
                }
                else
                {
                    UK2Node_VariableSet* SetterNode = NewObject<UK2Node_VariableSet>(EventGraph);
                    SetterNode->VariableReference.SetSelfMember(*VarName);
                    SetterNode->NodePosX = PositionX;
                    SetterNode->NodePosY = PositionY;
                    SetterNode->CreateNewGuid();
                    EventGraph->AddNode(SetterNode, true, true);
                    SetterNode->PostPlacedNewNode();
                    SetterNode->AllocateDefaultPins();
                    NewNode = SetterNode;
                    NodeTitle = FString::Printf(TEXT("Set %s"), *VarName);
                    NodeType = TEXT("UK2Node_VariableSet");
                }
                bFound = true;
                break;
            }
        }
        
        // If not found in variables, check components
        if (!bFound && bIsGetter && Blueprint->SimpleConstructionScript)
        {
            TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
            for (USCS_Node* Node : AllNodes)
            {
                if (Node && Node->GetVariableName().ToString().Equals(VarName, ESearchCase::IgnoreCase))
                {
                    // Create component reference node using variable get approach
                    UK2Node_VariableGet* ComponentGetterNode = NewObject<UK2Node_VariableGet>(EventGraph);
                    ComponentGetterNode->VariableReference.SetSelfMember(Node->GetVariableName());
                    ComponentGetterNode->NodePosX = PositionX;
                    ComponentGetterNode->NodePosY = PositionY;
                    ComponentGetterNode->CreateNewGuid();
                    EventGraph->AddNode(ComponentGetterNode, true, true);
                    ComponentGetterNode->PostPlacedNewNode();
                    ComponentGetterNode->AllocateDefaultPins();
                    NewNode = ComponentGetterNode;
                    NodeTitle = FString::Printf(TEXT("Get %s"), *VarName);
                    NodeType = TEXT("UK2Node_VariableGet");
                    bFound = true;
                    
                    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Created component reference for '%s'"), *VarName);
                    break;
                }
            }
        }
        
        if (!bFound)
        {
            return BuildNodeResult(false, FString::Printf(TEXT("Variable or component '%s' not found in Blueprint '%s'"), *VarName, *BlueprintName));
        }
    }
    // Special loop node types - these classes may not exist in all UE versions
    // Commented out for UE 5.6 compatibility - the Blueprint Action Database will handle these
    /*
    else if (FunctionName.Equals(TEXT("For Each Loop (Map)"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Map ForEach"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("K2Node_MapForEach"), ESearchCase::IgnoreCase))
    {
        UK2Node_MapForEach* MapForEachNode = NewObject<UK2Node_MapForEach>(EventGraph);
        MapForEachNode->NodePosX = PositionX;
        MapForEachNode->NodePosY = PositionY;
        MapForEachNode->CreateNewGuid();
        EventGraph->AddNode(MapForEachNode, true, true);
        MapForEachNode->PostPlacedNewNode();
        MapForEachNode->AllocateDefaultPins();
        NewNode = MapForEachNode;
        NodeTitle = TEXT("For Each Loop (Map)");
        NodeType = TEXT("UK2Node_MapForEach");
    }
    else if (FunctionName.Equals(TEXT("For Each Loop (Set)"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Set ForEach"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("K2Node_SetForEach"), ESearchCase::IgnoreCase))
    {
        UK2Node_SetForEach* SetForEachNode = NewObject<UK2Node_SetForEach>(EventGraph);
        SetForEachNode->NodePosX = PositionX;
        SetForEachNode->NodePosY = PositionY;
        SetForEachNode->CreateNewGuid();
        EventGraph->AddNode(SetForEachNode, true, true);
        SetForEachNode->PostPlacedNewNode();
        SetForEachNode->AllocateDefaultPins();
        NewNode = SetForEachNode;
        NodeTitle = TEXT("For Each Loop (Set)");
        NodeType = TEXT("UK2Node_SetForEach");
    }
    */
    // Universal dynamic node creation using Blueprint Action Database
    else if (TryCreateNodeUsingBlueprintActionDatabase(FunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created node '%s' using Blueprint Action Database"), *NodeTitle);
    }
    else
    {
        // Try to find the function and create a function call node
        UFunction* TargetFunction = nullptr;
        TargetClass = nullptr;
        
        // Find target class
        TargetClass = FindTargetClass(ClassName);
        if (TargetClass)
        {
            TargetFunction = TargetClass->FindFunctionByName(*FunctionName);
        }
        else
        {
            // Try to find the function in common math/utility classes
            TArray<UClass*> CommonClasses = {
                UKismetMathLibrary::StaticClass(),
                UKismetSystemLibrary::StaticClass(),
                UGameplayStatics::StaticClass()
            };
            
            for (UClass* TestClass : CommonClasses)
            {
                TargetFunction = TestClass->FindFunctionByName(*FunctionName);
                if (TargetFunction)
                {
                    TargetClass = TestClass;
                    break;
                }
            }
        }
        
        if (!TargetFunction)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Function '%s' not found"), *FunctionName);
            return BuildNodeResult(false, FString::Printf(TEXT("Function '%s' not found and not a recognized control flow node"), *FunctionName));
        }
        
        UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Found function '%s' in class '%s'"), *FunctionName, TargetClass ? *TargetClass->GetName() : TEXT("Unknown"));
        
        // Create the function call node
        UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
        FunctionNode->FunctionReference.SetExternalMember(TargetFunction->GetFName(), TargetClass);
        FunctionNode->NodePosX = PositionX;
        FunctionNode->NodePosY = PositionY;
        FunctionNode->CreateNewGuid();
        EventGraph->AddNode(FunctionNode, true, true);
        FunctionNode->PostPlacedNewNode();
        FunctionNode->AllocateDefaultPins();
        NewNode = FunctionNode;
        NodeTitle = FunctionName;
        NodeType = TEXT("UK2Node_CallFunction");
    }
    
    if (!NewNode)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to create node for '%s'"), *FunctionName);
        return BuildNodeResult(false, FString::Printf(TEXT("Failed to create node for '%s'"), *FunctionName));
    }
    
    UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Return success result
    return BuildNodeResult(true, FString::Printf(TEXT("Successfully created '%s' node (%s)"), *NodeTitle, *NodeType),
                          BlueprintName, FunctionName, NewNode, NodeTitle, NodeType, TargetClass, PositionX, PositionY);
}

bool FBlueprintNodeCreationService::ParseJsonParameters(const FString& JsonParams, TSharedPtr<FJsonObject>& OutParamsObject, TSharedPtr<FJsonObject>& OutResultObj)
{
    if (!JsonParams.IsEmpty())
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonParams);
        if (!FJsonSerializer::Deserialize(Reader, OutParamsObject) || !OutParamsObject.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to parse JSON parameters"));
            OutResultObj->SetBoolField(TEXT("success"), false);
            OutResultObj->SetStringField(TEXT("message"), TEXT("Invalid JSON parameters"));
            return false;
        }
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully parsed JSON parameters"));
    }
    return true;
}

void FBlueprintNodeCreationService::ParseNodePosition(const FString& NodePosition, int32& OutPositionX, int32& OutPositionY)
{
    OutPositionX = 0;
    OutPositionY = 0;
    
    if (!NodePosition.IsEmpty())
    {
        // Try to parse as JSON array [x, y] first (from Python)
        TSharedPtr<FJsonValue> JsonValue;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodePosition);
        
        if (FJsonSerializer::Deserialize(Reader, JsonValue) && JsonValue.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
            if (JsonValue->TryGetArray(JsonArray) && JsonArray->Num() >= 2)
            {
                OutPositionX = FMath::RoundToInt((*JsonArray)[0]->AsNumber());
                OutPositionY = FMath::RoundToInt((*JsonArray)[1]->AsNumber());
                return;
            }
        }
        
        // Fallback: parse as string format "[x, y]" or "x,y"
        FString CleanPosition = NodePosition;
        CleanPosition = CleanPosition.Replace(TEXT("["), TEXT(""));
        CleanPosition = CleanPosition.Replace(TEXT("]"), TEXT(""));
        
        TArray<FString> Coords;
        CleanPosition.ParseIntoArray(Coords, TEXT(","));
        
        if (Coords.Num() == 2)
        {
            OutPositionX = FCString::Atoi(*Coords[0].TrimStartAndEnd());
            OutPositionY = FCString::Atoi(*Coords[1].TrimStartAndEnd());
        }
    }
}

UClass* FBlueprintNodeCreationService::FindTargetClass(const FString& ClassName)
{
    if (ClassName.IsEmpty()) return nullptr;
    
    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(ClassName);
    if (TargetClass) return TargetClass;
    
    // Try with common prefixes
    FString TestClassName = ClassName;
    if (!TestClassName.StartsWith(TEXT("U")) && !TestClassName.StartsWith(TEXT("A")) && !TestClassName.StartsWith(TEXT("/Script/")))
    {
        TestClassName = TEXT("U") + ClassName;
        TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
        if (TargetClass) return TargetClass;
    }
    
    // Try with full path for common Unreal classes
    if (ClassName.Equals(TEXT("KismetMathLibrary"), ESearchCase::IgnoreCase))
    {
        return UKismetMathLibrary::StaticClass();
    }
    else if (ClassName.Equals(TEXT("KismetSystemLibrary"), ESearchCase::IgnoreCase))
    {
        return UKismetSystemLibrary::StaticClass();
    }
    else if (ClassName.Equals(TEXT("GameplayStatics"), ESearchCase::IgnoreCase))
    {
        return UGameplayStatics::StaticClass();
    }
    
    return nullptr;
}

UBlueprint* FBlueprintNodeCreationService::FindBlueprintByName(const FString& BlueprintName)
{
    // Find blueprint by searching for it in the asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> BlueprintAssets;
    AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
    
    for (const FAssetData& AssetData : BlueprintAssets)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Contains(BlueprintName) || BlueprintName.Contains(AssetName))
        {
            UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
            if (Blueprint)
                return Blueprint;
        }
    }
    
    return nullptr;
}

bool FBlueprintNodeCreationService::TryCreateNodeUsingBlueprintActionDatabase(const FString& FunctionName, UEdGraph* EventGraph, float PositionX, float PositionY, UEdGraphNode*& NewNode, FString& NodeTitle, FString& NodeType)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Attempting dynamic creation for '%s'"), *FunctionName);
    
    // Use Blueprint Action Database to find the appropriate spawner
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found %d action categories"), ActionRegistry.Num());
    
    // Search through spawners directly
    for (const auto& ActionPair : ActionRegistry)
    {
        for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
        {
            if (NodeSpawner && IsValid(NodeSpawner))
            {
                // Get template node to determine what type of node this is
                UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();
                if (!TemplateNode)
                {
                    continue;
                }
                
                // Try to match based on node type and function name
                FString NodeName = TEXT("");
                FString NodeClass = TemplateNode->GetClass()->GetName();
                
                // Get human-readable node name
                if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                {
                    NodeName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                    if (NodeName.IsEmpty())
                    {
                        NodeName = K2Node->GetClass()->GetName();
                    }
                    
                    // For function calls, get the function name
                    if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                    {
                        if (UFunction* Function = FunctionNode->GetTargetFunction())
                        {
                            NodeName = Function->GetName();
                        }
                    }
                }
                else
                {
                    NodeName = TemplateNode->GetClass()->GetName();
                }
                
                // Check if this matches our function name
                if (NodeName.Equals(FunctionName, ESearchCase::IgnoreCase))
                {
                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found matching spawner for '%s' (node class: %s)"), *NodeName, *NodeClass);
                    
                    // Create the node using the spawner
                    NewNode = NodeSpawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D(PositionX, PositionY));
                    if (NewNode)
                    {
                        NodeTitle = NodeName;
                        NodeType = NodeClass;
                        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);
                        return true;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: No matching spawner found for '%s'"), *FunctionName);
    return false;
}

FString FBlueprintNodeCreationService::BuildNodeResult(bool bSuccess, const FString& Message, const FString& BlueprintName, const FString& FunctionName, UEdGraphNode* NewNode, const FString& NodeTitle, const FString& NodeType, UClass* TargetClass, int32 PositionX, int32 PositionY)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSuccess);
    
    // Use the correct field name based on success/failure
    if (bSuccess)
    {
        ResultObj->SetStringField(TEXT("message"), Message);
    }
    else
    {
        ResultObj->SetStringField(TEXT("error"), Message);
    }
    
    if (bSuccess && NewNode)
    {
        ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
        ResultObj->SetStringField(TEXT("function_name"), FunctionName);
        ResultObj->SetStringField(TEXT("node_type"), NodeType);
        ResultObj->SetStringField(TEXT("class_name"), NodeType.Equals(TEXT("UK2Node_CallFunction")) ? (TargetClass ? TargetClass->GetName() : TEXT("")) : TEXT(""));
        ResultObj->SetStringField(TEXT("node_id"), NewNode->NodeGuid.ToString());
        ResultObj->SetStringField(TEXT("node_title"), NodeTitle);
        
        // Add position info
        TSharedPtr<FJsonObject> PositionObj = MakeShared<FJsonObject>();
        PositionObj->SetNumberField(TEXT("x"), PositionX);
        PositionObj->SetNumberField(TEXT("y"), PositionY);
        ResultObj->SetObjectField(TEXT("position"), PositionObj);
        
        // Add pin information
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : NewNode->Pins)
        {
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
            PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
            PinObj->SetBoolField(TEXT("is_execution"), Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
            PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        ResultObj->SetArrayField(TEXT("pins"), PinsArray);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    return OutputString;
}

void FBlueprintNodeCreationService::LogNodeCreationAttempt(const FString& FunctionName, const FString& BlueprintName, const FString& ClassName, int32 PositionX, int32 PositionY)
{
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintNodeCreationService: Creating node '%s' in blueprint '%s' with class '%s' at position [%d, %d]"), 
           *FunctionName, *BlueprintName, *ClassName, PositionX, PositionY);
} 