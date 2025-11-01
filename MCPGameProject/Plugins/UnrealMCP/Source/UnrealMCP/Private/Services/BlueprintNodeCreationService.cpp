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

// Forward declaration of native property helper
static bool TryCreateNativePropertyNode(const FString& VarName, bool bIsGetter, UEdGraph* EventGraph, int32 PositionX, int32 PositionY, UEdGraphNode*& OutNode, FString& OutTitle, FString& OutNodeType);

// Helper function to create arithmetic and comparison operations directly
static bool TryCreateArithmeticOrComparisonNode(const FString& OperationName, UEdGraph* EventGraph, int32 PositionX, int32 PositionY, UEdGraphNode*& OutNode, FString& OutTitle, FString& OutNodeType);
// Utility to convert property names into friendly display strings
static FString ConvertPropertyNameToDisplayLocal(const FString& InPropName)
{
    FString Name = InPropName;
    // Strip leading 'b' for bool properties
    if (Name.StartsWith(TEXT("b")) && Name.Len() > 1 && FChar::IsUpper(Name[1]))
    {
        Name = Name.RightChop(1);
    }

    FString Out;
    Out.Reserve(Name.Len()*2);
    for (int32 Index = 0; Index < Name.Len(); ++Index)
    {
        const TCHAR Ch = Name[Index];
        if (Index > 0 && FChar::IsUpper(Ch) && !FChar::IsUpper(Name[Index-1]))
        {
            Out += TEXT(" ");
        }
        Out.AppendChar(Ch);
    }
    return Out;
}

// Utility to convert CamelCase function names to Title Case (e.g., "GetActorLocation" -> "Get Actor Location")
static FString ConvertCamelCaseToTitleCase(const FString& InFunctionName)
{
    if (InFunctionName.IsEmpty())
    {
        return InFunctionName;
    }

    FString Out;
    Out.Reserve(InFunctionName.Len() * 2);
    
    for (int32 Index = 0; Index < InFunctionName.Len(); ++Index)
    {
        const TCHAR Ch = InFunctionName[Index];
        
        // Add space before uppercase letters (except the first character)
        if (Index > 0 && FChar::IsUpper(Ch) && !FChar::IsUpper(InFunctionName[Index-1]))
        {
            // Don't add space if the previous character was already a space
            if (Out.Len() > 0 && Out[Out.Len()-1] != TEXT(' '))
            {
                Out += TEXT(" ");
            }
        }
        
        Out.AppendChar(Ch);
    }
    
    return Out;
}

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
        return BuildNodeResult(false, FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
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
        return BuildNodeResult(false, FString::Printf(TEXT("Could not find or create target graph '%s'"), *TargetGraphName));
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
            return BuildNodeResult(false, TEXT("Could not find SelectFloat function for literal creation"));
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
                return BuildNodeResult(false, FString::Printf(TEXT("Could not find macro graph '%s' in discovered macro blueprint"), *MacroGraphName));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Could not discover macro blueprint for '%s'"), *EffectiveFunctionName);
            return BuildNodeResult(false, FString::Printf(TEXT("Could not discover macro blueprint for '%s'. Macro may not be available."), *EffectiveFunctionName));
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
            bSpawned = TryCreateNodeUsingBlueprintActionDatabase(EffectiveFunctionName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);

            if (!bSpawned)
            {
                // Try trimmed variable name (without Get/Set prefix)
                bSpawned = TryCreateNodeUsingBlueprintActionDatabase(VarName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }

            if (!bSpawned && bIsGetter)
            {
                // Prepend "Get " to the node name in case the BAD entry includes it
                FString GetterName = FString::Printf(TEXT("Get %s"), *VarName);
                bSpawned = TryCreateNodeUsingBlueprintActionDatabase(GetterName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }
            else if (!bSpawned && !bIsGetter)
            {
                FString SetterName = FString::Printf(TEXT("Set %s"), *VarName);
                bSpawned = TryCreateNodeUsingBlueprintActionDatabase(SetterName, TEXT(""), EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }

            if (!bSpawned)
            {
                // Final attempt: directly construct native property node by class search
                bSpawned = TryCreateNativePropertyNode(VarName, bIsGetter, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType);
            }

            if (!bSpawned)
            {
                return BuildNodeResult(false, FString::Printf(TEXT("Variable or component '%s' not found in Blueprint '%s' and no matching Blueprint Action Database entry"), *VarName, *BlueprintName));
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
            return BuildNodeResult(false, FString::Printf(TEXT("struct_type parameter is required for %s operations"), bIsBreakStruct ? TEXT("BreakStruct") : TEXT("MakeStruct")));
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
            return BuildNodeResult(false, FString::Printf(TEXT("Struct type not found: %s"), *StructTypeName));
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
    else if (TryCreateArithmeticOrComparisonNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created arithmetic/comparison node '%s'"), *NodeTitle);
    }
    // Universal dynamic node creation using Blueprint Action Database
    else if (TryCreateNodeUsingBlueprintActionDatabase(EffectiveFunctionName, ClassName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
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
            return BuildNodeResult(false, FString::Printf(TEXT("Function '%s' not found and not a recognized control flow node"), *EffectiveFunctionName));
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
        return BuildNodeResult(false, FString::Printf(TEXT("Failed to create node for '%s'"), *EffectiveFunctionName));
    }
    
    UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);
    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Return success result
    return BuildNodeResult(true, FString::Printf(TEXT("Successfully created '%s' node (%s)"), *NodeTitle, *NodeType),
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

bool FBlueprintNodeCreationService::TryCreateNodeUsingBlueprintActionDatabase(const FString& FunctionName, const FString& ClassName, UEdGraph* EventGraph, float PositionX, float PositionY, UEdGraphNode*& NewNode, FString& NodeTitle, FString& NodeType)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Attempting dynamic creation for '%s' with class '%s'"), *FunctionName, *ClassName);
    
    // Special handling for Enhanced Input Actions
    // When ClassName is "EnhancedInputAction", we search the Asset Registry for Input Actions
    // and create UK2Node_EnhancedInputAction nodes
    if (ClassName.Equals(TEXT("EnhancedInputAction"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Enhanced Input Action requested for '%s'"), *FunctionName);
        
        // Search for the Input Action asset
        IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        TArray<FAssetData> ActionAssets;
        AssetRegistry.GetAssetsByClass(UInputAction::StaticClass()->GetClassPathName(), ActionAssets, true);
        
        for (const FAssetData& ActionAsset : ActionAssets)
        {
            FString ActionName = ActionAsset.AssetName.ToString();
            
            // Check if this is the action we're looking for
            if (ActionName.Equals(FunctionName, ESearchCase::IgnoreCase))
            {
                if (const UInputAction* Action = Cast<const UInputAction>(ActionAsset.GetAsset()))
                {
                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found Enhanced Input Action '%s', creating node"), *ActionName);
                    
                    // Create Enhanced Input Action node manually
                    // Note: We can't use the spawner directly because UK2Node_EnhancedInputAction
                    // is created dynamically based on available Input Actions
                    UK2Node_EnhancedInputAction* InputActionNode = NewObject<UK2Node_EnhancedInputAction>(EventGraph);
                    if (InputActionNode)
                    {
                        InputActionNode->InputAction = const_cast<UInputAction*>(Action);
                        InputActionNode->NodePosX = PositionX;
                        InputActionNode->NodePosY = PositionY;
                        InputActionNode->CreateNewGuid();
                        EventGraph->AddNode(InputActionNode, true, true);
                        InputActionNode->PostPlacedNewNode();
                        InputActionNode->AllocateDefaultPins();
                        
                        NewNode = InputActionNode;
                        NodeTitle = FString::Printf(TEXT("EnhancedInputAction %s"), *ActionName);
                        NodeType = TEXT("K2Node_EnhancedInputAction");
                        
                        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Successfully created Enhanced Input Action node for '%s'"), *ActionName);
                        return true;
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Failed to create UK2Node_EnhancedInputAction for '%s'"), *ActionName);
                        return false;
                    }
                }
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Enhanced Input Action '%s' not found in asset registry"), *FunctionName);
        return false;
    }
    
    // Create a map of common operation aliases to their actual function names
    TMap<FString, TArray<FString>> OperationAliases;
    
    // Arithmetic operations
    OperationAliases.Add(TEXT("Add"), {TEXT("Add_FloatFloat"), TEXT("Add_IntInt"), TEXT("Add_DoubleDouble"), TEXT("Add_VectorVector"), TEXT("Add")});
    OperationAliases.Add(TEXT("Subtract"), {TEXT("Subtract_FloatFloat"), TEXT("Subtract_IntInt"), TEXT("Subtract_DoubleDouble"), TEXT("Subtract_VectorVector"), TEXT("Subtract")});
    OperationAliases.Add(TEXT("Multiply"), {TEXT("Multiply_FloatFloat"), TEXT("Multiply_IntInt"), TEXT("Multiply_DoubleDouble"), TEXT("Multiply_VectorFloat"), TEXT("Multiply")});
    OperationAliases.Add(TEXT("Divide"), {TEXT("Divide_FloatFloat"), TEXT("Divide_IntInt"), TEXT("Divide_DoubleDouble"), TEXT("Divide_VectorFloat"), TEXT("Divide")});
    OperationAliases.Add(TEXT("Modulo"), {TEXT("Percent_IntInt"), TEXT("Percent_FloatFloat"), TEXT("FMod"), TEXT("Modulo")});
    OperationAliases.Add(TEXT("Power"), {TEXT("MultiplyMultiply_FloatFloat"), TEXT("Power")});
    
    // Comparison operations
    OperationAliases.Add(TEXT("Equal"), {TEXT("EqualEqual_FloatFloat"), TEXT("EqualEqual_IntInt"), TEXT("EqualEqual_BoolBool"), TEXT("EqualEqual_ObjectObject"), TEXT("Equal")});
    OperationAliases.Add(TEXT("NotEqual"), {TEXT("NotEqual_FloatFloat"), TEXT("NotEqual_IntInt"), TEXT("NotEqual_BoolBool"), TEXT("NotEqual_ObjectObject"), TEXT("NotEqual")});
    OperationAliases.Add(TEXT("Greater"), {TEXT("Greater_FloatFloat"), TEXT("Greater_IntInt"), TEXT("Greater_DoubleDouble"), TEXT("Greater")});
    OperationAliases.Add(TEXT("GreaterEqual"), {TEXT("GreaterEqual_FloatFloat"), TEXT("GreaterEqual_IntInt"), TEXT("GreaterEqual_DoubleDouble"), TEXT("GreaterEqual")});
    OperationAliases.Add(TEXT("Less"), {TEXT("Less_FloatFloat"), TEXT("Less_IntInt"), TEXT("Less_DoubleDouble"), TEXT("Less")});
    OperationAliases.Add(TEXT("LessEqual"), {TEXT("LessEqual_FloatFloat"), TEXT("LessEqual_IntInt"), TEXT("LessEqual_DoubleDouble"), TEXT("LessEqual")});
    
    // Logical operations
    OperationAliases.Add(TEXT("And"), {TEXT("BooleanAND"), TEXT("And")});
    OperationAliases.Add(TEXT("Or"), {TEXT("BooleanOR"), TEXT("Or")});
    OperationAliases.Add(TEXT("Not"), {TEXT("BooleanNOT"), TEXT("Not")});
    
    // Build list of function names to search for
    TArray<FString> SearchNames;
    SearchNames.Add(FunctionName);
    
    // Add CamelCase to Title Case conversion (e.g., "GetActorLocation" -> "Get Actor Location")
    FString TitleCaseVersion = ConvertCamelCaseToTitleCase(FunctionName);
    if (!TitleCaseVersion.Equals(FunctionName, ESearchCase::IgnoreCase))
    {
        SearchNames.Add(TitleCaseVersion);
        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Added Title Case variation: '%s' -> '%s'"), *FunctionName, *TitleCaseVersion);
    }
    
    // Add aliases if this is a known operation
    if (OperationAliases.Contains(FunctionName))
    {
        SearchNames.Append(OperationAliases[FunctionName]);
    }
    
    // Also try some common variations
    SearchNames.Add(FString::Printf(TEXT("%s_FloatFloat"), *FunctionName));
    SearchNames.Add(FString::Printf(TEXT("%s_IntInt"), *FunctionName));
    SearchNames.Add(FString::Printf(TEXT("%s_DoubleDouble"), *FunctionName));
    
    // Use Blueprint Action Database to find the appropriate spawner
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found %d action categories, searching for %d name variations"), ActionRegistry.Num(), SearchNames.Num());
    
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
                FString FunctionNameFromNode = TEXT("");
                
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
                            FunctionNameFromNode = Function->GetName();
                            // Use function name as node name if it's more descriptive
                            if (NodeName.IsEmpty() || NodeName == NodeClass)
                            {
                                NodeName = FunctionNameFromNode;
                            }
                        }
                    }
                }
                else
                {
                    NodeName = TemplateNode->GetClass()->GetName();
                }
                
                // Check if any of our search names match
                bool bFoundMatch = false;
                bool bExactMatch = false;  // Track if we found an exact match
                FString MatchedName;
                
                for (const FString& SearchName : SearchNames)
                {
                    // Try exact match on node title
                    if (NodeName.Equals(SearchName, ESearchCase::IgnoreCase))
                    {
                        bFoundMatch = true;
                        bExactMatch = true;
                        MatchedName = SearchName;
                        break;
                    }
                    
                    // Try exact match on function name
                    if (!FunctionNameFromNode.IsEmpty() && FunctionNameFromNode.Equals(SearchName, ESearchCase::IgnoreCase))
                    {
                        bFoundMatch = true;
                        bExactMatch = true;
                        MatchedName = SearchName;
                        break;
                    }
                    
                    // Try partial match (for operations like "+" which might show as "Add (float)")
                    // But DON'T break here - continue searching for exact match
                    if (!bFoundMatch && NodeName.Contains(SearchName, ESearchCase::IgnoreCase))
                    {
                        bFoundMatch = true;
                        bExactMatch = false;
                        MatchedName = SearchName;
                        // DON'T break - keep searching for exact match
                    }
                }
                
                if (bFoundMatch)
                {
                    // CRITICAL FIX for Problem #3/#5: Check class name if specified
                    // This ensures we get the correct function when multiple functions have the same name
                    bool bClassMatches = true;
                    
                    // CRITICAL FIX: When ClassName is NOT specified, prefer exact function name matches
                    // to avoid getting "GetAllActorsOfClassMatchingTagQuery" when we want "GetAllActorsOfClass"
                    bool bPreferExactMatch = ClassName.IsEmpty() && !bExactMatch;
                    if (bPreferExactMatch)
                    {
                        // Check if the actual function name is EXACTLY what we're looking for
                        // This prevents partial matches from being accepted when an exact match might exist later
                        bool bIsExactFunctionMatch = false;
                        if (!FunctionNameFromNode.IsEmpty())
                        {
                            for (const FString& SearchName : SearchNames)
                            {
                                if (FunctionNameFromNode.Equals(SearchName, ESearchCase::IgnoreCase))
                                {
                                    bIsExactFunctionMatch = true;
                                    break;
                                }
                            }
                        }
                        
                        // If this is NOT an exact function name match, skip it and continue searching
                        // This allows us to find the exact match (e.g., "GetAllActorsOfClass" vs "GetAllActorsOfClassMatchingTagQuery")
                        if (!bIsExactFunctionMatch)
                        {
                            UE_LOG(LogTemp, Verbose, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Skipping partial match '%s' (function: '%s') - searching for exact match"), 
                                   *NodeName, *FunctionNameFromNode);
                            continue; // Skip this spawner, continue to next one
                        }
                    }
                    
                    if (!ClassName.IsEmpty())
                    {
                        bClassMatches = false; // Start with false, must match to be true
                        
                        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Class filtering enabled, wanted class: '%s'"), *ClassName);
                        
                        // For function call nodes, check the owner class
                        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                        {
                            if (UFunction* Function = FunctionNode->GetTargetFunction())
                            {
                                UClass* OwnerClass = Function->GetOwnerClass();
                                if (OwnerClass)
                                {
                                    FString OwnerClassName = OwnerClass->GetName();
                                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found function '%s' in class '%s'"), *FunctionNameFromNode, *OwnerClassName);
                                    
                                    // Try exact match
                                    if (OwnerClassName.Equals(ClassName, ESearchCase::IgnoreCase))
                                    {
                                        bClassMatches = true;
                                    }
                                    // Try without U/A prefix
                                    else if (OwnerClassName.Len() > 1 && 
                                            (OwnerClassName[0] == 'U' || OwnerClassName[0] == 'A') &&
                                            OwnerClassName.Mid(1).Equals(ClassName, ESearchCase::IgnoreCase))
                                    {
                                        bClassMatches = true;
                                    }
                                    // Try with U prefix added to ClassName
                                    else if (!ClassName.StartsWith(TEXT("U")) && !ClassName.StartsWith(TEXT("A")) &&
                                            OwnerClassName.Equals(FString::Printf(TEXT("U%s"), *ClassName), ESearchCase::IgnoreCase))
                                    {
                                        bClassMatches = true;
                                    }
                                    // Try common library name mappings
                                    else if (ClassName.Equals(TEXT("GameplayStatics"), ESearchCase::IgnoreCase) && 
                                            OwnerClassName.Equals(TEXT("UGameplayStatics"), ESearchCase::IgnoreCase))
                                    {
                                        bClassMatches = true;
                                    }
                                    else if (ClassName.Equals(TEXT("KismetMathLibrary"), ESearchCase::IgnoreCase) && 
                                            OwnerClassName.Equals(TEXT("UKismetMathLibrary"), ESearchCase::IgnoreCase))
                                    {
                                        bClassMatches = true;
                                    }
                                    else if (ClassName.Equals(TEXT("KismetSystemLibrary"), ESearchCase::IgnoreCase) && 
                                            OwnerClassName.Equals(TEXT("UKismetSystemLibrary"), ESearchCase::IgnoreCase))
                                    {
                                        bClassMatches = true;
                                    }
                                    
                                    if (!bClassMatches)
                                    {
                                        UE_LOG(LogTemp, Verbose, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Skipping '%s' - class mismatch (wanted '%s', got '%s')"), 
                                               *NodeName, *ClassName, *OwnerClassName);
                                    }
                                }
                            }
                        }
                        // For other node types, if class is specified but can't be checked, skip
                        else
                        {
                            // Non-function nodes don't have a class, so if ClassName is specified, this isn't a match
                            bClassMatches = false;
                            UE_LOG(LogTemp, Verbose, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Skipping '%s' - not a function call node but class '%s' was specified"), 
                                   *NodeName, *ClassName);
                        }
                    }
                    
                    // Only create node if class matches (or no class was specified)
                    // AND if we have an exact function name match (when ClassName is specified)
                    // If class doesn't match, CONTINUE searching for other nodes with the same name
                    if (bClassMatches)
                    {
                        // If class name is specified, require exact function name match
                        // This prevents creating "GetAllActorsOfClassWithTag" when we want "GetAllActorsOfClass"
                        bool bFunctionNameMatches = true;
                        if (!ClassName.IsEmpty() && !FunctionNameFromNode.IsEmpty())
                        {
                            // Check if the actual function name exactly matches what we're looking for
                            bFunctionNameMatches = false;
                            for (const FString& SearchName : SearchNames)
                            {
                                if (FunctionNameFromNode.Equals(SearchName, ESearchCase::IgnoreCase))
                                {
                                    bFunctionNameMatches = true;
                                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Exact function name match: '%s' == '%s'"), *FunctionNameFromNode, *SearchName);
                                    break;
                                }
                            }
                            
                            if (!bFunctionNameMatches)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Skipping '%s' - function name mismatch (wanted exact match for '%s', got '%s')"), 
                                       *NodeName, *FunctionName, *FunctionNameFromNode);
                            }
                        }
                        
                        if (bFunctionNameMatches)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found matching spawner for '%s' -> '%s' (node class: %s, function: %s)"), 
                                   *FunctionName, *MatchedName, *NodeClass, *FunctionNameFromNode);
                            
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
                    // If class doesn't match, DON'T return - continue to next spawner
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: No matching spawner found for '%s' (tried %d variations)"), *FunctionName, SearchNames.Num());
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
        
        // Check if this is a pure function (no execution pins)
        bool bHasExecutionPins = false;
        for (UEdGraphPin* Pin : NewNode->Pins)
        {
            if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                bHasExecutionPins = true;
                break;
            }
        }
        bool bIsPure = !bHasExecutionPins;
        
        // CRITICAL: Add execution pin information for AI assistants
        ResultObj->SetBoolField(TEXT("is_pure_function"), bIsPure);
        ResultObj->SetBoolField(TEXT("requires_execution_flow"), !bIsPure);
        
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

void FBlueprintNodeCreationService::LogNodeCreationAttempt(const FString& FunctionName, const FString& BlueprintName, const FString& ClassName, int32 PositionX, int32 PositionY) const
{
    UE_LOG(LogTemp, Warning, TEXT("FBlueprintNodeCreationService: Creating node '%s' in blueprint '%s' with class '%s' at position [%d, %d]"), 
           *FunctionName, *BlueprintName, *ClassName, PositionX, PositionY);
} 

// Helper function to create arithmetic and comparison operations directly
static bool TryCreateArithmeticOrComparisonNode(const FString& OperationName, UEdGraph* EventGraph, int32 PositionX, int32 PositionY, UEdGraphNode*& OutNode, FString& OutTitle, FString& OutNodeType)
{
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Attempting to create '%s' using TypePromotion system"), *OperationName);
    
    // Map common operation names to TypePromotion operator names
    TMap<FString, FName> OperationMappings;
    
    // Arithmetic operations
    OperationMappings.Add(TEXT("Add"), TEXT("Add"));
    OperationMappings.Add(TEXT("Subtract"), TEXT("Subtract"));
    OperationMappings.Add(TEXT("Multiply"), TEXT("Multiply"));
    OperationMappings.Add(TEXT("Divide"), TEXT("Divide"));
    OperationMappings.Add(TEXT("Modulo"), TEXT("Percent"));
    OperationMappings.Add(TEXT("Power"), TEXT("MultiplyMultiply"));
    
    // Comparison operations
    OperationMappings.Add(TEXT("Equal"), TEXT("EqualEqual"));
    OperationMappings.Add(TEXT("NotEqual"), TEXT("NotEqual"));
    OperationMappings.Add(TEXT("Greater"), TEXT("Greater"));
    OperationMappings.Add(TEXT("GreaterEqual"), TEXT("GreaterEqual"));
    OperationMappings.Add(TEXT("Less"), TEXT("Less"));
    OperationMappings.Add(TEXT("LessEqual"), TEXT("LessEqual"));
    
    // Logical operations
    OperationMappings.Add(TEXT("And"), TEXT("BooleanAND"));
    OperationMappings.Add(TEXT("Or"), TEXT("BooleanOR"));
    OperationMappings.Add(TEXT("Not"), TEXT("BooleanNOT"));
    
    // Symbol-based operations
    OperationMappings.Add(TEXT("+"), TEXT("Add"));
    OperationMappings.Add(TEXT("-"), TEXT("Subtract"));
    OperationMappings.Add(TEXT("*"), TEXT("Multiply"));
    OperationMappings.Add(TEXT("/"), TEXT("Divide"));
    OperationMappings.Add(TEXT("=="), TEXT("EqualEqual"));
    OperationMappings.Add(TEXT("!="), TEXT("NotEqual"));
    OperationMappings.Add(TEXT(">"), TEXT("Greater"));
    OperationMappings.Add(TEXT(">="), TEXT("GreaterEqual"));
    OperationMappings.Add(TEXT("<"), TEXT("Less"));
    OperationMappings.Add(TEXT("<="), TEXT("LessEqual"));
    
    // Check if we have a mapping for this operation
    const FName* OpName = OperationMappings.Find(OperationName);
    if (!OpName)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: No TypePromotion mapping found for '%s'"), *OperationName);
        return false;
    }
    
    // APPROACH 1: Try to create K2Node_PromotableOperator directly
    UE_LOG(LogTemp, Warning, TEXT("Attempting DIRECT K2Node_PromotableOperator creation for operator '%s'"), *OpName->ToString());
    
    // Check if this operator is supported by TypePromotion system
    const TSet<FName>& AllOperators = FTypePromotion::GetAllOpNames();
    if (AllOperators.Contains(*OpName))
    {
        UE_LOG(LogTemp, Warning, TEXT("Operator '%s' is registered in TypePromotion system"), *OpName->ToString());
        
        // Create K2Node_PromotableOperator directly and set its function
        UK2Node_PromotableOperator* PromotableOpNode = NewObject<UK2Node_PromotableOperator>(EventGraph);
        if (PromotableOpNode)
        {
            // Find the appropriate function for this operator
            TArray<UFunction*> OpFunctions;
            FTypePromotion::GetAllFuncsForOp(*OpName, OpFunctions);
            
            if (OpFunctions.Num() > 0)
            {
                // Use the first available function (wildcard operators will adapt)
                UFunction* BaseFunction = OpFunctions[0];
                PromotableOpNode->SetFromFunction(BaseFunction);
                
                // Set position and add to graph
                PromotableOpNode->NodePosX = PositionX;
                PromotableOpNode->NodePosY = PositionY;
                PromotableOpNode->CreateNewGuid();
                EventGraph->AddNode(PromotableOpNode, true, true);
                PromotableOpNode->PostPlacedNewNode();
                PromotableOpNode->AllocateDefaultPins();
                
                // CRITICAL FIX: Don't break natural PromotableOperator behavior!
                // SetFromFunction already sets up proper wildcard adaptation
                // Just finalize with reconstruction and visualization
                PromotableOpNode->ReconstructNode();
                
                if (const UEdGraphSchema* Schema = EventGraph->GetSchema())
                {
                    Schema->ForceVisualizationCacheClear();
                }
                EventGraph->NotifyGraphChanged();
                
                OutNode = PromotableOpNode;
                FText UserFacingName = FTypePromotion::GetUserFacingOperatorName(*OpName);
                OutTitle = UserFacingName.IsEmpty() ? OperationName : UserFacingName.ToString();
                OutNodeType = TEXT("K2Node_PromotableOperator");
                
                UE_LOG(LogTemp, Warning, TEXT("Successfully created DIRECT K2Node_PromotableOperator for '%s'"), *OutTitle);
                return true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No functions found for operator '%s'"), *OpName->ToString());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Operator '%s' not found in TypePromotion system"), *OpName->ToString());
    }
    
    // APPROACH 2: Try to get the operator spawner from TypePromotion system (original approach)
    UBlueprintFunctionNodeSpawner* OperatorSpawner = FTypePromotion::GetOperatorSpawner(*OpName);
    if (OperatorSpawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Found TypePromotion spawner for operation '%s' -> '%s'"), *OperationName, *OpName->ToString());
        
        // Create the node using the TypePromotion spawner
        OutNode = OperatorSpawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D(PositionX, PositionY));
        if (OutNode)
        {
            FText UserFacingName = FTypePromotion::GetUserFacingOperatorName(*OpName);
            OutTitle = UserFacingName.IsEmpty() ? OperationName : UserFacingName.ToString();
            OutNodeType = OutNode->GetClass()->GetName();
            
            // CRITICAL FIX: Properly initialize PromotableOperator wildcard pins
            if (OutNodeType.Contains(TEXT("PromotableOperator")))
            {
                // Cast to PromotableOperator to access ResetNodeToWildcard
                if (UK2Node_PromotableOperator* PromotableOp = Cast<UK2Node_PromotableOperator>(OutNode))
                {
                    // Don't interfere with natural PromotableOperator adaptation
                    // TypePromotion spawner already sets up proper behavior
                    PromotableOp->ReconstructNode();
                    
                    if (const UEdGraphSchema* Schema = EventGraph->GetSchema())
                    {
                        Schema->ForceVisualizationCacheClear();
                    }
                    EventGraph->NotifyGraphChanged();
                    
                    UE_LOG(LogTemp, Warning, TEXT("Applied PromotableOperator wildcard pin fix with ResetNodeToWildcard for node: %s"), *OutTitle);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to cast PromotableOperator node for ResetNodeToWildcard"));
                }
            }
            
            UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Successfully created dynamic '%s' operator node using TypePromotion"), *OutTitle);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TryCreateArithmeticOrComparisonNode: TypePromotion spawner failed to create node for '%s'"), *OpName->ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: No TypePromotion spawner found for operator '%s'"), *OpName->ToString());
        
        // Fallback to old system for non-promotable operations
        TMap<FString, TArray<FString>> LegacyMappings;
        LegacyMappings.Add(TEXT("And"), {TEXT("BooleanAND")});
        LegacyMappings.Add(TEXT("Or"), {TEXT("BooleanOR")});
        LegacyMappings.Add(TEXT("Not"), {TEXT("BooleanNOT")});
        
        const TArray<FString>* FunctionNames = LegacyMappings.Find(OperationName);
        if (FunctionNames)
        {
            for (const FString& FunctionName : *FunctionNames)
            {
                UFunction* TargetFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(*FunctionName);
                if (TargetFunction)
                {
                    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
                    FunctionNode->FunctionReference.SetExternalMember(TargetFunction->GetFName(), UKismetMathLibrary::StaticClass());
                    FunctionNode->NodePosX = PositionX;
                    FunctionNode->NodePosY = PositionY;
                    FunctionNode->CreateNewGuid();
                    EventGraph->AddNode(FunctionNode, true, true);
                    FunctionNode->PostPlacedNewNode();
                    FunctionNode->AllocateDefaultPins();
                    
                    OutNode = FunctionNode;
                    OutTitle = OperationName;
                    OutNodeType = TEXT("UK2Node_CallFunction");
                    
                    UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Created legacy function node '%s'"), *FunctionName);
                    return true;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateArithmeticOrComparisonNode: Failed to create node for operation '%s'"), *OperationName);
    return false;
}

static bool TryCreateNativePropertyNode(const FString& VarName, bool bIsGetter, UEdGraph* EventGraph, int32 PositionX, int32 PositionY, UEdGraphNode*& OutNode, FString& OutTitle, FString& OutNodeType)
{
    // Helper lambdas -------------------------------------------------------
    auto StripBoolPrefix = [](const FString& InName) -> FString
    {
        // Strip a leading 'b' only when it is followed by an uppercase character (UE bool convention)
        if (InName.StartsWith(TEXT("b")) && InName.Len() > 1 && FChar::IsUpper(InName[1]))
        {
            return InName.Mid(1);
        }
        return InName;
    };

    auto IsPropertyWritable = [bIsGetter](FProperty* Property) -> bool
    {
        const bool bConstParm = Property->HasAnyPropertyFlags(CPF_ConstParm);
        const bool bReadOnlyMeta = Property->HasMetaData(TEXT("BlueprintReadOnly"));
        // For setter requests we allow write when the property is NOT const and NOT marked BlueprintReadOnly.
        if (bIsGetter)
        {
            return true; // Getter can always be created for BlueprintVisible properties
        }

        return !bConstParm && !bReadOnlyMeta;
    };

    // ---------------------------------------------------------------------
    // Build search candidates (space-stripped variants with/without bool prefix)
    TArray<FString> Candidates;
    const FString NoSpace = VarName.Replace(TEXT(" "), TEXT(""));
    Candidates.Add(NoSpace);
    Candidates.Add(FString(TEXT("b")) + NoSpace); // bool prefix variant

    // Structure to hold all matching properties so we can choose deterministically
    struct FPropMatch
    {
        UClass*  Class;
        FProperty* Property;
    };
    TArray<FPropMatch> Matches;

    // Iterate over all loaded classes once and gather potential matches
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass* TargetClass = *ClassIt;
        if (!TargetClass || TargetClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }

        for (TFieldIterator<FProperty> PropIt(TargetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
            {
                continue; // Not visible to Blueprints – skip
            }

            // For setter requests ensure the property is writable
            if (!bIsGetter && !IsPropertyWritable(Property))
            {
                continue; // Not writable – skip for setters
            }

            const FString PropName    = Property->GetName();
            const FString StrippedProp = StripBoolPrefix(PropName);

            // Build list of potential property names for matching
            TArray<FString> NameOptions;
            NameOptions.Add(PropName);
            if (StrippedProp != PropName)
            {
                NameOptions.Add(StrippedProp);
            }

            FString DisplayNameMeta = Property->GetMetaData(TEXT("DisplayName"));
            if (DisplayNameMeta.IsEmpty())
            {
                DisplayNameMeta = ConvertPropertyNameToDisplayLocal(PropName);
            }
            NameOptions.Add(DisplayNameMeta.Replace(TEXT(" "), TEXT("")));

            // Perform case-insensitive comparison against all candidates
            bool bMatch = false;
            for (const FString& Candidate : Candidates)
            {
                for (const FString& Option : NameOptions)
                {
                    if (Option.Equals(Candidate, ESearchCase::IgnoreCase))
                    {
                        bMatch = true;
                        break;
                    }
                }
                if (bMatch) break;
            }

            if (bMatch)
            {
                Matches.Add({ TargetClass, Property });
            }
        }
    }

    if (Matches.Num() == 0)
    {
        return false; // Nothing matched
    }

    // ---------------------------------------------------------------------
    // Choose a deterministic match: sort by class name, then property name for stability
    Matches.Sort([](const FPropMatch& A, const FPropMatch& B)
    {
        const int32 ClassCmp = A.Class->GetName().Compare(B.Class->GetName());
        if (ClassCmp == 0)
        {
            return A.Property->GetName().Compare(B.Property->GetName()) < 0;
        }
        return ClassCmp < 0;
    });

    const FPropMatch& Chosen = Matches[0];
    UClass*   TargetClass = Chosen.Class;
    FProperty* Property   = Chosen.Property;
    const FString PropName = Property->GetName();

    // Create the appropriate node ------------------------------------------------
    if (bIsGetter)
    {
        UK2Node_VariableGet* GetterNode = NewObject<UK2Node_VariableGet>(EventGraph);
        GetterNode->VariableReference.SetExternalMember(*PropName, TargetClass);
        GetterNode->NodePosX = PositionX;
        GetterNode->NodePosY = PositionY;
        GetterNode->CreateNewGuid();
        EventGraph->AddNode(GetterNode, true, true);
        GetterNode->PostPlacedNewNode();
        GetterNode->AllocateDefaultPins();
        OutNode = GetterNode;
        OutTitle = FString::Printf(TEXT("Get %s"), *VarName);
        OutNodeType = TEXT("UK2Node_VariableGet");
    }
    else
    {
        UK2Node_VariableSet* SetterNode = NewObject<UK2Node_VariableSet>(EventGraph);
        SetterNode->VariableReference.SetExternalMember(*PropName, TargetClass);
        SetterNode->NodePosX = PositionX;
        SetterNode->NodePosY = PositionY;
        SetterNode->CreateNewGuid();
        EventGraph->AddNode(SetterNode, true, true);
        SetterNode->PostPlacedNewNode();
        SetterNode->AllocateDefaultPins();
        OutNode = SetterNode;
        OutTitle = FString::Printf(TEXT("Set %s"), *VarName);
        OutNodeType = TEXT("UK2Node_VariableSet");
    }

    return true;
} 
