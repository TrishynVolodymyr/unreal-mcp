#include "Services/BlueprintNodeCreationService.h"
#include "Services/MacroDiscoveryService.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "BlueprintNodeBinder.h"
#include "BlueprintTypePromotion.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_PromotableOperator.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Framework/Commands/UIAction.h"
#include "Engine/Engine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "Utils/UnrealMCPCommonUtils.h" // For utility blueprint finder

// Include refactored node creation helpers
#include "NodeCreation/NodeCreationHelpers.h"
#include "NodeCreation/ArithmeticNodeCreator.h"
#include "NodeCreation/NativePropertyNodeCreator.h"
#include "NodeCreation/BlueprintActionDatabaseNodeCreator.h"
#include "NodeCreation/NodeResultBuilder.h"

FBlueprintNodeCreationService::FBlueprintNodeCreationService()
{
}

FString FBlueprintNodeCreationService::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintNodeCreationService::CreateNodeByActionName ENTRY: Blueprint='%s', Function='%s', ClassName='%s'"), *BlueprintName, *FunctionName, *ClassName);
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    // Create a map for function name aliases
    TMap<FString, FString> FunctionNameAliases;
    FunctionNameAliases.Add(TEXT("ForEachLoop"), TEXT("For Each Loop"));
    FunctionNameAliases.Add(TEXT("ForEachLoopWithBreak"), TEXT("For Each Loop With Break"));
    FunctionNameAliases.Add(TEXT("ForEachLoopMap"), TEXT("For Each Loop (Map)"));
    FunctionNameAliases.Add(TEXT("ForEachLoopSet"), TEXT("For Each Loop (Set)"));

    FString EffectiveFunctionName = FunctionName;
    if (FunctionNameAliases.Contains(FunctionName))
    {
        EffectiveFunctionName = FunctionNameAliases[FunctionName];
    }
    
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
    
    // --- DIAGNOSTIC LOGGING: Function entry ---
    FString ParamsJsonStr = TEXT("<null>");
    if (ParamsObject.IsValid())
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsJsonStr);
        FJsonSerializer::Serialize(ParamsObject.ToSharedRef(), Writer);
    }
    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName ENTRY: FunctionName='%s', Blueprint='%s', Params=%s"), *EffectiveFunctionName, *BlueprintName, *ParamsJsonStr);
    
    // Find the blueprint
    // Use the common utility that searches both UBlueprint and UWidgetBlueprint assets
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
    }
    
    // Get the event graph
    // Determine which graph we should place the node in. By default we still use the main
    // EventGraph, but callers can specify a custom graph name (e.g., a function graph)
    // through the optional "target_graph" field either in the top-level parameters or
    // inside the JsonParams object.  This lets external tools create nodes inside
    // Blueprint functions like "GetLine" / "GetSpeaker" rather than being limited to
    // the EventGraph.

    FString TargetGraphName = TEXT("EventGraph");

    // Check if the parameter was provided at the top level (maintaining backward compat)
    if (ParamsObject.IsValid())
    {
        FString TempGraphName;
        if (ParamsObject->TryGetStringField(TEXT("target_graph"), TempGraphName) && !TempGraphName.IsEmpty())
        {
            TargetGraphName = TempGraphName;
        }
    }

    UEdGraph* EventGraph = nullptr;

    // First search function graphs (these hold user-defined functions)
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph && Graph->GetName().Equals(TargetGraphName, ESearchCase::IgnoreCase))
        {
            EventGraph = Graph;
            break;
        }
    }

    // Fallback: search across *all* graphs that belong to this blueprint (includes Macros, AnimGraph, etc.)
    if (!EventGraph)
    {
        TArray<UEdGraph*> AllGraphs;
        Blueprint->GetAllGraphs(AllGraphs);
        for (UEdGraph* Graph : AllGraphs)
        {
            if (Graph && Graph->GetName().Equals(TargetGraphName, ESearchCase::IgnoreCase))
            {
                EventGraph = Graph;
                break;
            }
        }
    }

    // If still not found, create a new graph with that name (function graph by default)
    if (!EventGraph)
    {
        UE_LOG(LogTemp, Display, TEXT("Target graph '%s' not found – creating new graph"), *TargetGraphName);

        // Create function graph when target is not EventGraph; otherwise create EventGraph
        if (TargetGraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
        {
            EventGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(*TargetGraphName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
            FBlueprintEditorUtils::AddUbergraphPage(Blueprint, EventGraph);
        }
        else
        {
            EventGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(*TargetGraphName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
            FBlueprintEditorUtils::AddFunctionGraph<UFunction>(Blueprint, EventGraph, /*bIsUserCreated=*/true, nullptr);
        }
    }

    if (!EventGraph)
    {
        return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Could not find or create target graph '%s'"), *TargetGraphName));
    }

    UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Using graph '%s' for node placement"), *EventGraph->GetName());
    
    // Parse node position
    int32 PositionX, PositionY;
    ParseNodePosition(NodePosition, PositionX, PositionY);
    
    // Log the creation attempt
    LogNodeCreationAttempt(EffectiveFunctionName, BlueprintName, ClassName, PositionX, PositionY);
    
    UEdGraphNode* NewNode = nullptr;
    FString NodeTitle = TEXT("Unknown");
    FString NodeType = TEXT("Unknown");
    UClass* TargetClass = nullptr;
    
    // After parameter parsing and before any node type handling
    // --- PATCH: Rewrite 'Get'/'Set' with variable_name before any node type handling ---
    if ((EffectiveFunctionName.Equals(TEXT("Get"), ESearchCase::IgnoreCase) ||
         EffectiveFunctionName.Equals(TEXT("Set"), ESearchCase::IgnoreCase)) &&
        ParamsObject.IsValid())
    {
        FString ParamVarName;
        // Check at root level
        if (ParamsObject->TryGetStringField(TEXT("variable_name"), ParamVarName) && !ParamVarName.IsEmpty())
        {
            EffectiveFunctionName = FString::Printf(TEXT("%s %s"), *EffectiveFunctionName, *ParamVarName);
            UE_LOG(LogTemp, Warning, TEXT("[PATCH] Rewrote function name to '%s' using variable_name payload"), *EffectiveFunctionName);
        }
        // Optionally: check inside 'kwargs' if not found at root
        else if (ParamsObject->HasField(TEXT("kwargs")))
        {
            TSharedPtr<FJsonObject> KwargsObj = ParamsObject->GetObjectField(TEXT("kwargs"));
            if (KwargsObj.IsValid() && KwargsObj->TryGetStringField(TEXT("variable_name"), ParamVarName) && !ParamVarName.IsEmpty())
            {
                EffectiveFunctionName = FString::Printf(TEXT("%s %s"), *EffectiveFunctionName, *ParamVarName);
                UE_LOG(LogTemp, Warning, TEXT("[PATCH] Rewrote function name to '%s' using variable_name in kwargs"), *EffectiveFunctionName);
            }
        }
    }
    // Check for literal/constant value creation
    if (EffectiveFunctionName.Equals(TEXT("Float"), ESearchCase::IgnoreCase) ||
        EffectiveFunctionName.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) ||
        EffectiveFunctionName.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase) ||
        EffectiveFunctionName.StartsWith(TEXT("Literal"), ESearchCase::IgnoreCase))
    {
        // For literal values, we create a simple variable get node with a default value
        // This is the most straightforward way to create constants in Blueprint
        UK2Node_VariableGet* LiteralNode = NewObject<UK2Node_VariableGet>(EventGraph);
        
        // Set up a temporary variable reference for the literal
        FString LiteralVarName = TEXT("LiteralValue");
        if (ParamsObject.IsValid())
        {
            FString ParamValue;
            if (ParamsObject->TryGetStringField(TEXT("value"), ParamValue) && !ParamValue.IsEmpty())
            {
                LiteralVarName = FString::Printf(TEXT("Literal_%s"), *ParamValue);
            }
        }
        
        // For now, create a simple math operation that returns the constant
        // This is a workaround since direct literal nodes are complex in UE
        UK2Node_CallFunction* MathNode = NewObject<UK2Node_CallFunction>(EventGraph);
        
        // Use SelectFloat from KismetMathLibrary to create a constant
        UFunction* SelectFloatFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("SelectFloat"));
        if (SelectFloatFunc)
        {
            MathNode->SetFromFunction(SelectFloatFunc);
            MathNode->NodePosX = PositionX;
            MathNode->NodePosY = PositionY;
            MathNode->CreateNewGuid();
            EventGraph->AddNode(MathNode, true, true);
            MathNode->PostPlacedNewNode();
            MathNode->AllocateDefaultPins();
            
            // Set default values on pins to create the constant
            if (ParamsObject.IsValid())
            {
                FString ParamValue;
                if (ParamsObject->TryGetStringField(TEXT("value"), ParamValue) && !ParamValue.IsEmpty())
                {
                    // Find the A and B pins and set them to the same value
                    for (UEdGraphPin* Pin : MathNode->Pins)
                    {
                        if (Pin && (Pin->PinName == TEXT("A") || Pin->PinName == TEXT("B")))
                        {
                            Pin->DefaultValue = ParamValue;
                        }
                        // Set the Index pin to false (0) so it always returns A
                        else if (Pin && Pin->PinName == TEXT("Index"))
                        {
                            Pin->DefaultValue = TEXT("false");
                        }
                    }
                }
            }
            
            NewNode = MathNode;
            NodeTitle = FString::Printf(TEXT("Constant %s"), *EffectiveFunctionName);
            NodeType = TEXT("K2Node_CallFunction");
        }
        else
        {
            return FNodeResultBuilder::BuildNodeResult(false, TEXT("Could not find SelectFloat function for literal creation"));
        }
    }
    // Check if this is a control flow node request
    else if (EffectiveFunctionName.Equals(TEXT("Branch"), ESearchCase::IgnoreCase) || 
        EffectiveFunctionName.Equals(TEXT("IfThenElse"), ESearchCase::IgnoreCase) ||
        EffectiveFunctionName.Equals(TEXT("UK2Node_IfThenElse"), ESearchCase::IgnoreCase))
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
    else if (EffectiveFunctionName.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("ExecutionSequence"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("UK2Node_ExecutionSequence"), ESearchCase::IgnoreCase))
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
    else if (EffectiveFunctionName.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("Custom Event"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("UK2Node_CustomEvent"), ESearchCase::IgnoreCase))
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
    else if (EffectiveFunctionName.Equals(TEXT("Cast"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("UK2Node_DynamicCast"), ESearchCase::IgnoreCase))
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
                                        FString GeneratedClassName = TestBP->GeneratedClass->GetName();
                                        // Remove common Blueprint prefixes for comparison
                                        if (GeneratedClassName.StartsWith(TEXT("BP_")))
                                        {
                                            GeneratedClassName = GeneratedClassName.Mid(3);
                                        }
                                        bIsMatch = GeneratedClassName.Equals(TargetTypeName, ESearchCase::IgnoreCase);
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
    else if (EffectiveFunctionName.StartsWith(TEXT("Receive")) || 
             EffectiveFunctionName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("EndPlay"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("BeginDestroy"), ESearchCase::IgnoreCase))
    {
        // Create standard event node
        UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);
        
        // Determine the correct event name and parent class
        FString EventName = EffectiveFunctionName;
        FString ParentClassName = TEXT("/Script/Engine.Actor");
        
        // Map common event names to their proper "Receive" format
        if (EffectiveFunctionName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveBeginPlay");
        }
        else if (EffectiveFunctionName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveTick");
        }
        else if (EffectiveFunctionName.Equals(TEXT("EndPlay"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveEndPlay");
        }
        else if (EffectiveFunctionName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveActorBeginOverlap");
        }
        else if (EffectiveFunctionName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveActorEndOverlap");
        }
        else if (EffectiveFunctionName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveHit");
        }
        else if (EffectiveFunctionName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase))
        {
            EventName = TEXT("ReceiveDestroyed");
        }
        else if (EffectiveFunctionName.Equals(TEXT("BeginDestroy"), ESearchCase::IgnoreCase))
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
    else if (FMacroDiscoveryService::IsMacroFunction(EffectiveFunctionName))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Processing macro function '%s' using MacroDiscoveryService"), *EffectiveFunctionName);
        
        // Use the macro discovery service to find the macro blueprint dynamically
        FString MacroGraphName = FMacroDiscoveryService::MapFunctionNameToMacroGraphName(EffectiveFunctionName);
        UBlueprint* MacroBlueprint = FMacroDiscoveryService::FindMacroBlueprint(EffectiveFunctionName);
        
        if (MacroBlueprint)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found macro blueprint for '%s' via discovery service"), *EffectiveFunctionName);
            
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
                NodeTitle = EffectiveFunctionName;
                NodeType = TEXT("UK2Node_MacroInstance");
                
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created macro instance for '%s' using discovery service"), *EffectiveFunctionName);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName);
                return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not discover macro blueprint for '%s'"), *EffectiveFunctionName);
            return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Could not discover macro blueprint for '%s'. Macro may not be available."), *EffectiveFunctionName));
        }
    }
    // Variable getter/setter node creation
    else if (EffectiveFunctionName.StartsWith(TEXT("Get ")) || EffectiveFunctionName.StartsWith(TEXT("Set ")) ||
        EffectiveFunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase) ||
        EffectiveFunctionName.Equals(TEXT("UK2Node_VariableSet"), ESearchCase::IgnoreCase))
    {
        FString VarName = EffectiveFunctionName;
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
        // Handle explicit node class names without "Get " or "Set " prefix
        else if (EffectiveFunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase) ||
                 EffectiveFunctionName.Equals(TEXT("UK2Node_VariableSet"), ESearchCase::IgnoreCase))
        {
            // Determine getter vs setter based on node class requested
            bIsGetter = EffectiveFunctionName.Equals(TEXT("UK2Node_VariableGet"), ESearchCase::IgnoreCase);

            // Attempt to pull the actual variable name from JSON parameters ("variable_name") if provided
            if (ParamsObject.IsValid())
            {
                FString ParamVarName;
                // First check at root level
                if (ParamsObject->TryGetStringField(TEXT("variable_name"), ParamVarName) && !ParamVarName.IsEmpty())
                {
                    VarName = ParamVarName;
                }
                else
                {
                    // Then check nested under a "kwargs" object (for backward compatibility with Python tool)
                    const TSharedPtr<FJsonObject>* KwargsObject;
                    if (ParamsObject->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
                    {
                        if ((*KwargsObject)->TryGetStringField(TEXT("variable_name"), ParamVarName) && !ParamVarName.IsEmpty())
                        {
                            VarName = ParamVarName;
                        }
                    }
                }
            }
        }
        
        // Log getter/setter detection and variable name
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: EffectiveFunctionName='%s', bIsGetter=%d, bIsSetter=%d, VarName='%s'"), *EffectiveFunctionName, (int)bIsGetter, (int)bIsGetter, *VarName);

        // Try to find the variable or component in the Blueprint
        bool bFound = false;
        
        // Diagnostic logging: List all user variables and what we're searching for
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Blueprint '%s' has %d user variables. Looking for '%s'."), *Blueprint->GetName(), Blueprint->NewVariables.Num(), *VarName);
        for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found variable '%s' (type: %s)"), *VarDesc.VarName.ToString(), *VarDesc.VarType.PinCategory.ToString());
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
            // Variable not found directly in the Blueprint – it might be a native property on another class.
            // Attempt to spawn it via the Blueprint Action Database using multiple name variants so users can still
            // create property nodes like "Get Show Mouse Cursor" on a PlayerController reference.

            bool bSpawned = false;
            bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(EffectiveFunctionName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);

            if (!bSpawned)
            {
                // Try trimmed variable name (without Get/Set prefix)
                bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(VarName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }

            if (!bSpawned && bIsGetter)
            {
                // Prepend "Get " to the node name in case the BAD entry includes it
                FString GetterName = FString::Printf(TEXT("Get %s"), *VarName);
                bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(GetterName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }
            else if (!bSpawned && !bIsGetter)
            {
                FString SetterName = FString::Printf(TEXT("Set %s"), *VarName);
                bSpawned = FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(SetterName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }

            if (!bSpawned)
            {
                // Final attempt: directly construct native property node by class search
                bSpawned = FNativePropertyNodeCreator::TryCreateNativePropertyNode(VarName, bIsGetter, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }

            if (!bSpawned)
            {
                return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Variable or component '%s' not found in Blueprint '%s' and no matching Blueprint Action Database entry"), *VarName, *BlueprintName));
            }
        }
    }
    else if (EffectiveFunctionName.Equals(TEXT("BreakStruct"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("Break Struct"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("MakeStruct"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("Make Struct"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("UK2Node_BreakStruct"), ESearchCase::IgnoreCase) ||
             EffectiveFunctionName.Equals(TEXT("UK2Node_MakeStruct"), ESearchCase::IgnoreCase))
    {
        bool bIsBreakStruct = EffectiveFunctionName.Contains(TEXT("Break"), ESearchCase::IgnoreCase);
        
        // Extract struct type from parameters
        FString StructTypeName;
        bool bFoundStructType = false;
        
        if (ParamsObject.IsValid())
        {
            // First check at root level
            if (ParamsObject->TryGetStringField(TEXT("struct_type"), StructTypeName) && !StructTypeName.IsEmpty())
            {
                bFoundStructType = true;
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found struct_type '%s' at root level"), *StructTypeName);
            }
            else
            {
                // Then check nested under kwargs object
                const TSharedPtr<FJsonObject>* KwargsObject;
                if (ParamsObject->TryGetObjectField(TEXT("kwargs"), KwargsObject) && KwargsObject->IsValid())
                {
                    if ((*KwargsObject)->TryGetStringField(TEXT("struct_type"), StructTypeName) && !StructTypeName.IsEmpty())
                    {
                        bFoundStructType = true;
                        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found struct_type '%s' in kwargs"), *StructTypeName);
                    }
                }
            }
        }
        
        if (!bFoundStructType)
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: struct_type parameter is required for %s operations"), bIsBreakStruct ? TEXT("BreakStruct") : TEXT("MakeStruct"));
            return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("struct_type parameter is required for %s operations"), bIsBreakStruct ? TEXT("BreakStruct") : TEXT("MakeStruct")));
        }
        
        // Find the struct type
        UScriptStruct* StructType = nullptr;
        
        // Try multiple variations of struct name resolution
        TArray<FString> StructNameVariations = {
            StructTypeName,
            FString::Printf(TEXT("F%s"), *StructTypeName),
            FString::Printf(TEXT("/Script/Engine.%s"), *StructTypeName),
            FString::Printf(TEXT("/Script/Engine.F%s"), *StructTypeName),
            FString::Printf(TEXT("/Script/CoreUObject.%s"), *StructTypeName),
            FString::Printf(TEXT("/Script/CoreUObject.F%s"), *StructTypeName)
        };
        
        for (const FString& StructName : StructNameVariations)
        {
            StructType = FindObject<UScriptStruct>(nullptr, *StructName);
            if (StructType)
            {
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Found struct type '%s' using name '%s'"), *StructType->GetName(), *StructName);
                break;
            }
        }
        
        // If not found by FindObject, try using LoadObject for asset paths
        if (!StructType && StructTypeName.StartsWith(TEXT("/Game/")))
        {
            StructType = LoadObject<UScriptStruct>(nullptr, *StructTypeName);
            if (StructType)
            {
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Loaded struct type '%s' using LoadObject"), *StructType->GetName());
            }
        }
        
        if (!StructType)
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not find struct type '%s'"), *StructTypeName);
            return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Struct type not found: %s"), *StructTypeName));
        }
        
        // Create the appropriate struct node
        if (bIsBreakStruct)
        {
            UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(EventGraph);
            BreakNode->StructType = StructType;
            BreakNode->NodePosX = PositionX;
            BreakNode->NodePosY = PositionY;
            BreakNode->CreateNewGuid();
            EventGraph->AddNode(BreakNode, true, true);
            BreakNode->PostPlacedNewNode();
            BreakNode->AllocateDefaultPins();
            
            NewNode = BreakNode;
            NodeTitle = FString::Printf(TEXT("Break %s"), *StructType->GetDisplayNameText().ToString());
            NodeType = TEXT("UK2Node_BreakStruct");
            
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created BreakStruct node for '%s'"), *StructType->GetName());
        }
        else
        {
            UK2Node_MakeStruct* MakeNode = NewObject<UK2Node_MakeStruct>(EventGraph);
            MakeNode->StructType = StructType;
            MakeNode->NodePosX = PositionX;
            MakeNode->NodePosY = PositionY;
            MakeNode->CreateNewGuid();
            EventGraph->AddNode(MakeNode, true, true);
            MakeNode->PostPlacedNewNode();
            MakeNode->AllocateDefaultPins();
            
            NewNode = MakeNode;
            NodeTitle = FString::Printf(TEXT("Make %s"), *StructType->GetDisplayNameText().ToString());
            NodeType = TEXT("UK2Node_MakeStruct");
            
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created MakeStruct node for '%s'"), *StructType->GetName());
        }
    }
    // Try to create arithmetic or comparison operations directly
    else if (FArithmeticNodeCreator::TryCreateArithmeticOrComparisonNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created arithmetic/comparison node '%s'"), *NodeTitle);
    }
    // Universal dynamic node creation using Blueprint Action Database
    else if (FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(EffectiveFunctionName, ClassName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created node '%s' using Blueprint Action Database"), *NodeTitle);
    }
    else
    {
        // Try to find the function and create a function call node
        UFunction* TargetFunction = nullptr;
        TargetClass = nullptr;
        
        // CRITICAL FIX: Add alternative function name mappings for common issues
        TMap<FString, FString> FunctionMappings;
        FunctionMappings.Add(TEXT("Vector Length"), TEXT("VSize"));
        FunctionMappings.Add(TEXT("VectorLength"), TEXT("VSize"));
        FunctionMappings.Add(TEXT("Distance"), TEXT("Vector_Distance"));
        FunctionMappings.Add(TEXT("Vector Distance"), TEXT("Vector_Distance"));
        FunctionMappings.Add(TEXT("GetPlayerPawn"), TEXT("GetPlayerPawn"));
        FunctionMappings.Add(TEXT("Get Player Pawn"), TEXT("GetPlayerPawn"));
        
        FString ActualFunctionName = EffectiveFunctionName;
        if (FString* MappedName = FunctionMappings.Find(EffectiveFunctionName))
        {
            ActualFunctionName = *MappedName;
            UE_LOG(LogTemp, Warning, TEXT("Mapped function name '%s' -> '%s'"), *EffectiveFunctionName, *ActualFunctionName);
        }
        
        // Find target class
        TargetClass = FindTargetClass(ClassName);
        if (TargetClass)
        {
            TargetFunction = TargetClass->FindFunctionByName(*ActualFunctionName);
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
                TargetFunction = TestClass->FindFunctionByName(*ActualFunctionName);
                if (TargetFunction)
                {
                    TargetClass = TestClass;
                    break;
                }
            }
        }
        
        if (!TargetFunction)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Function '%s' not found"), *EffectiveFunctionName);
            return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Function '%s' not found and not a recognized control flow node"), *EffectiveFunctionName));
        }
        
        UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Found function '%s' in class '%s'"), *EffectiveFunctionName, TargetClass ? *TargetClass->GetName() : TEXT("Unknown"));
        
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
        NodeTitle = EffectiveFunctionName;
        NodeType = TEXT("UK2Node_CallFunction");
    }
    
    if (!NewNode)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to create node for '%s'"), *EffectiveFunctionName);
        return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Failed to create node for '%s'"), *EffectiveFunctionName));
    }
    
    UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Return success result
    return FNodeResultBuilder::BuildNodeResult(true, FString::Printf(TEXT("Successfully created '%s' node (%s)"), *NodeTitle, *NodeType),
                          BlueprintName, EffectiveFunctionName, NewNode, NodeTitle, NodeType, TargetClass, PositionX, PositionY);
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

void FBlueprintNodeCreationService::LogNodeCreationAttempt(const FString& FunctionName, const FString& BlueprintName, const FString& ClassName, int32 PositionX, int32 PositionY) const
{
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintNodeCreationService: Creating node '%s' in blueprint '%s' with class '%s' at position [%d, %d]"), 
           *FunctionName, *BlueprintName, *ClassName, PositionX, PositionY);
} 
