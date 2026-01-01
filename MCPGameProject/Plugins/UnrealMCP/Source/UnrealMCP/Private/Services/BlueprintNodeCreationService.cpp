#include "Services/BlueprintNodeCreationService.h"
#include "Services/NodeCreation/ControlFlowNodeCreator.h"
#include "Services/NodeCreation/EventAndVariableNodeCreator.h"
#include "Services/IBlueprintNodeService.h"  // For FBlueprintNodeConnectionParams
#include "Services/BlueprintNode/BlueprintNodeConnectionService.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Utils/UnrealMCPCommonUtils.h" // For utility blueprint finder
#include "Utils/GraphUtils.h" // For reliable node IDs

// Include refactored node creation helpers
#include "NodeCreation/ArithmeticNodeCreator.h"
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
    FString WarningMessage; // For warnings like WidgetBlueprintLibrary usage in non-Widget Blueprints
    
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
    // Try control flow node creators (Literal, Branch, Sequence, CustomEvent)
    if (FControlFlowNodeCreator::Get().TryCreateLiteralNode(EffectiveFunctionName, ParamsObject, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        // Literal node created successfully
    }
    else if (FControlFlowNodeCreator::Get().TryCreateBranchNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        // Branch node created successfully
    }
    else if (FControlFlowNodeCreator::Get().TryCreateSequenceNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        // Sequence node created successfully
    }
    else if (FControlFlowNodeCreator::Get().TryCreateCustomEventNode(EffectiveFunctionName, ParamsObject, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        // CustomEvent node created successfully
    }
    // Try cast node creation
    else if (FControlFlowNodeCreator::Get().TryCreateCastNode(EffectiveFunctionName, ParamsObject, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        // Cast node created successfully
    }
    // Try self reference node creation
    else if (FControlFlowNodeCreator::Get().TryCreateSelfNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
    {
        // Self node created successfully
    }
    // Try component bound event creation
    else
    {
        FString ErrorMessage;
        if (FEventAndVariableNodeCreator::Get().TryCreateComponentBoundEventNode(ParamsObject, Blueprint, BlueprintName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType, ErrorMessage))
        {
            // Component bound event handled (might have succeeded or failed with error)
            if (!ErrorMessage.IsEmpty())
            {
                // Failed with specific error
                return FNodeResultBuilder::BuildNodeResult(false, ErrorMessage);
            }
            // Otherwise succeeded
        }
        // Try standard event node creation
        else if (FEventAndVariableNodeCreator::Get().TryCreateStandardEventNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
        {
            // Standard event node created successfully
        }
        // Try macro node creation
        else if (FEventAndVariableNodeCreator::Get().TryCreateMacroNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType, ErrorMessage))
        {
            // Macro node handled (might have succeeded or failed with error)
            if (!ErrorMessage.IsEmpty())
            {
                // Failed with specific error
                return FNodeResultBuilder::BuildNodeResult(false, ErrorMessage);
            }
            // Otherwise succeeded
        }
        // Try variable node creation
        else if (FEventAndVariableNodeCreator::Get().TryCreateVariableNode(EffectiveFunctionName, ParamsObject, Blueprint, BlueprintName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType, ErrorMessage))
        {
            // Variable node handled (might have succeeded or failed with error)
            if (!ErrorMessage.IsEmpty())
            {
                // Failed with specific error
                return FNodeResultBuilder::BuildNodeResult(false, ErrorMessage);
            }
            // Otherwise succeeded
        }
        // Try struct node creation
        else if (FEventAndVariableNodeCreator::Get().TryCreateStructNode(EffectiveFunctionName, ParamsObject, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType, ErrorMessage))
        {
            // Struct node handled (might have succeeded or failed with error)
            if (!ErrorMessage.IsEmpty())
            {
                // Failed with specific error
                return FNodeResultBuilder::BuildNodeResult(false, ErrorMessage);
            }
            // Otherwise succeeded
        }
        // Try call parent function node creation (Parent: FunctionName)
        else if (FEventAndVariableNodeCreator::Get().TryCreateCallParentFunctionNode(EffectiveFunctionName, ParamsObject, Blueprint, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType, ErrorMessage))
        {
            // Call parent function node handled (might have succeeded or failed with error)
            if (!ErrorMessage.IsEmpty())
            {
                // Failed with specific error
                return FNodeResultBuilder::BuildNodeResult(false, ErrorMessage);
            }
            // Otherwise succeeded
        }
        // Try to create arithmetic or comparison operations directly
        else if (FArithmeticNodeCreator::TryCreateArithmeticOrComparisonNode(EffectiveFunctionName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType))
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created arithmetic/comparison node '%s'"), *NodeTitle);
        }
        // Universal dynamic node creation using Blueprint Action Database
        else
        {
            FString DatabaseErrorMessage;
            FString DatabaseWarningMessage;
            if (FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(EffectiveFunctionName, ClassName, EventGraph, PositionX, PositionY, NewNode, NodeTitle, NodeType, &DatabaseErrorMessage, &DatabaseWarningMessage))
            {
                UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Successfully created node '%s' using Blueprint Action Database"), *NodeTitle);
                // Store warning for later inclusion in result
                if (!DatabaseWarningMessage.IsEmpty())
                {
                    WarningMessage = DatabaseWarningMessage;
                }
            }
            else if (!DatabaseErrorMessage.IsEmpty())
            {
                // If we got a specific error message (e.g., duplicate functions), return it immediately
                // Don't try other methods as this is a user error that needs to be fixed
                return FNodeResultBuilder::BuildNodeResult(false, DatabaseErrorMessage);
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
        }
    }

    if (!NewNode)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to create node for '%s'"), *EffectiveFunctionName);
        return FNodeResultBuilder::BuildNodeResult(false, FString::Printf(TEXT("Failed to create node for '%s'"), *EffectiveFunctionName));
    }

    UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);

    // Collect warnings and connection results for enhanced response
    TArray<FString> Warnings;
    TArray<TSharedPtr<FJsonObject>> ConnectionResults;

    // Auto-set bPrintToLog=true for Print String nodes (for debugging via log file analysis)
    if (EffectiveFunctionName.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase) ||
        NodeTitle.Contains(TEXT("Print String")))
    {
        UEdGraphPin* PrintToLogPin = NewNode->FindPin(TEXT("bPrintToLog"));
        if (PrintToLogPin)
        {
            PrintToLogPin->DefaultValue = TEXT("true");
            UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Auto-set bPrintToLog=true for Print String node"));
        }
    }

    // Apply pin values if provided
    if (ParamsObject.IsValid() && ParamsObject->HasField(TEXT("pin_values")))
    {
        const TSharedPtr<FJsonObject>* PinValuesObject = nullptr;
        if (ParamsObject->TryGetObjectField(TEXT("pin_values"), PinValuesObject) && PinValuesObject->IsValid())
        {
            ApplyPinValues(NewNode, EventGraph, Blueprint, *PinValuesObject, Warnings);
        }
    }

    // Apply connections if provided
    if (ParamsObject.IsValid() && ParamsObject->HasField(TEXT("connections")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray = nullptr;
        if (ParamsObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
        {
            ApplyConnections(NewNode, EventGraph, Blueprint, *ConnectionsArray, Warnings, ConnectionResults);
        }
    }

    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Combine all warnings into the warning message
    if (Warnings.Num() > 0)
    {
        if (!WarningMessage.IsEmpty())
        {
            WarningMessage += TEXT("; ");
        }
        WarningMessage += FString::Join(Warnings, TEXT("; "));
    }

    // Return success result (include warning if present)
    // TODO: Enhance BuildNodeResult to include connection_results array
    return FNodeResultBuilder::BuildNodeResult(true, FString::Printf(TEXT("Successfully created '%s' node (%s)"), *NodeTitle, *NodeType),
                          BlueprintName, EffectiveFunctionName, NewNode, NodeTitle, NodeType, TargetClass, PositionX, PositionY, WarningMessage);
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

void FBlueprintNodeCreationService::ApplyPinValues(UEdGraphNode* Node, UEdGraph* Graph, UBlueprint* Blueprint,
                                                   const TSharedPtr<FJsonObject>& PinValuesObject, TArray<FString>& OutWarnings)
{
    if (!Node || !Graph || !Blueprint || !PinValuesObject.IsValid())
    {
        return;
    }

    const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
    if (!K2Schema)
    {
        OutWarnings.Add(TEXT("Graph schema is not K2 (Blueprint) schema - pin values not applied"));
        return;
    }

    // Iterate over all pin values
    for (const auto& PinValuePair : PinValuesObject->Values)
    {
        const FString& PinName = PinValuePair.Key;
        FString Value;

        // Get value as string (handles string, number, bool)
        if (PinValuePair.Value->Type == EJson::String)
        {
            Value = PinValuePair.Value->AsString();
        }
        else if (PinValuePair.Value->Type == EJson::Number)
        {
            Value = FString::SanitizeFloat(PinValuePair.Value->AsNumber());
        }
        else if (PinValuePair.Value->Type == EJson::Boolean)
        {
            Value = PinValuePair.Value->AsBool() ? TEXT("true") : TEXT("false");
        }
        else
        {
            OutWarnings.Add(FString::Printf(TEXT("Unsupported value type for pin '%s'"), *PinName));
            continue;
        }

        // Find the pin on the node
        UEdGraphPin* TargetPin = nullptr;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin && (Pin->GetName() == PinName || Pin->PinFriendlyName.ToString() == PinName))
            {
                TargetPin = Pin;
                break;
            }
        }

        if (!TargetPin)
        {
            // Pin not found - add warning but continue with other pins
            OutWarnings.Add(FString::Printf(TEXT("Pin '%s' not found on node - value not set"), *PinName));
            continue;
        }

        // Set the pin value based on its type (adapted from SetNodePinValueCommand logic)
        if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
        {
            // Handle class reference pins
            UClass* ClassToSet = nullptr;

            if (Value.StartsWith(TEXT("/Script/")))
            {
                ClassToSet = FindObject<UClass>(nullptr, *Value);
            }
            else if (Value.StartsWith(TEXT("/Game/")))
            {
                FString ClassPath = Value;
                if (!ClassPath.EndsWith(TEXT("_C")))
                {
                    FString BaseName = FPaths::GetBaseFilename(Value);
                    ClassPath = FString::Printf(TEXT("%s.%s_C"), *Value, *BaseName);
                }
                ClassToSet = LoadObject<UClass>(nullptr, *ClassPath);

                if (!ClassToSet)
                {
                    UObject* Asset = LoadObject<UObject>(nullptr, *Value);
                    if (UBlueprint* BP = Cast<UBlueprint>(Asset))
                    {
                        ClassToSet = BP->GeneratedClass;
                    }
                }
            }
            else
            {
                // Try widget class first, then engine classes
                ClassToSet = FUnrealMCPCommonUtils::FindWidgetClass(Value);
                if (!ClassToSet)
                {
                    FString FullPath = FString::Printf(TEXT("/Script/Engine.%s"), *Value);
                    ClassToSet = FindObject<UClass>(nullptr, *FullPath);
                    if (!ClassToSet)
                    {
                        ClassToSet = FindFirstObject<UClass>(*Value, EFindFirstObjectOptions::NativeFirst);
                    }
                }
            }

            if (ClassToSet)
            {
                K2Schema->TrySetDefaultObject(*TargetPin, ClassToSet);
                UE_LOG(LogTemp, Display, TEXT("Set class pin '%s' to '%s'"), *PinName, *Value);
            }
            else
            {
                OutWarnings.Add(FString::Printf(TEXT("Class '%s' not found for pin '%s'"), *Value, *PinName));
            }
        }
        else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && TargetPin->PinType.PinSubCategoryObject.IsValid())
        {
            // Handle enum pins
            UEnum* EnumType = Cast<UEnum>(TargetPin->PinType.PinSubCategoryObject.Get());
            if (EnumType)
            {
                int64 EnumValue = EnumType->GetValueByNameString(Value);
                if (EnumValue == INDEX_NONE)
                {
                    // Try short name match
                    for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i)
                    {
                        FString EnumName = EnumType->GetNameStringByIndex(i);
                        FString ShortName = EnumName;
                        int32 ColonPos;
                        if (EnumName.FindLastChar(':', ColonPos))
                        {
                            ShortName = EnumName.RightChop(ColonPos + 1);
                        }
                        if (ShortName.Equals(Value, ESearchCase::IgnoreCase))
                        {
                            EnumValue = i;
                            TargetPin->DefaultValue = EnumName;
                            break;
                        }
                    }
                }
                else
                {
                    TargetPin->DefaultValue = EnumType->GetNameStringByValue(EnumValue);
                }

                if (EnumValue == INDEX_NONE)
                {
                    OutWarnings.Add(FString::Printf(TEXT("Enum value '%s' not found for pin '%s'"), *Value, *PinName));
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("Set enum pin '%s' to '%s'"), *PinName, *TargetPin->DefaultValue);
                }
            }
        }
        else
        {
            // For basic types (int, float, bool, string), just set the default value
            TargetPin->DefaultValue = Value;
            UE_LOG(LogTemp, Display, TEXT("Set pin '%s' to '%s'"), *PinName, *Value);
        }
    }

    // Reconstruct the node to apply changes
    Node->ReconstructNode();
}

void FBlueprintNodeCreationService::ApplyConnections(UEdGraphNode* Node, UEdGraph* Graph, UBlueprint* Blueprint,
                                                     const TArray<TSharedPtr<FJsonValue>>& ConnectionsArray,
                                                     TArray<FString>& OutWarnings, TArray<TSharedPtr<FJsonObject>>& OutConnectionResults)
{
    if (!Node || !Graph || !Blueprint || ConnectionsArray.Num() == 0)
    {
        return;
    }

    // Get the new node's ID for placeholder replacement
    FString NewNodeId = FGraphUtils::GetReliableNodeId(Node);

    // Build connection params array
    TArray<FBlueprintNodeConnectionParams> ConnectionParams;

    for (const TSharedPtr<FJsonValue>& ConnectionValue : ConnectionsArray)
    {
        const TSharedPtr<FJsonObject>* ConnectionObj = nullptr;
        if (!ConnectionValue->TryGetObject(ConnectionObj) || !ConnectionObj->IsValid())
        {
            OutWarnings.Add(TEXT("Invalid connection object in connections array"));
            continue;
        }

        FBlueprintNodeConnectionParams Params;

        // Get source and target info
        FString SourceNodeId, SourcePin, TargetNodeId, TargetPin;
        (*ConnectionObj)->TryGetStringField(TEXT("source_node_id"), SourceNodeId);
        (*ConnectionObj)->TryGetStringField(TEXT("source_pin"), SourcePin);
        (*ConnectionObj)->TryGetStringField(TEXT("target_node_id"), TargetNodeId);
        (*ConnectionObj)->TryGetStringField(TEXT("target_pin"), TargetPin);

        // Replace "$new" placeholder with actual new node ID
        if (SourceNodeId == TEXT("$new") || SourceNodeId == TEXT("$NEW"))
        {
            SourceNodeId = NewNodeId;
        }
        if (TargetNodeId == TEXT("$new") || TargetNodeId == TEXT("$NEW"))
        {
            TargetNodeId = NewNodeId;
        }

        if (SourceNodeId.IsEmpty() || SourcePin.IsEmpty() || TargetNodeId.IsEmpty() || TargetPin.IsEmpty())
        {
            OutWarnings.Add(TEXT("Connection missing required fields (source_node_id, source_pin, target_node_id, target_pin)"));
            continue;
        }

        Params.SourceNodeId = SourceNodeId;
        Params.SourcePin = SourcePin;
        Params.TargetNodeId = TargetNodeId;
        Params.TargetPin = TargetPin;

        ConnectionParams.Add(Params);
    }

    if (ConnectionParams.Num() == 0)
    {
        return;
    }

    // Use the connection service
    TArray<FConnectionResultInfo> Results;
    FBlueprintNodeConnectionService::Get().ConnectBlueprintNodesEnhanced(Blueprint, ConnectionParams, Graph->GetName(), Results);

    // Process results
    for (int32 i = 0; i < Results.Num(); i++)
    {
        const FConnectionResultInfo& Result = Results[i];
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), Result.bSuccess);
        ResultObj->SetStringField(TEXT("source_node_id"), ConnectionParams[i].SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), ConnectionParams[i].TargetNodeId);

        if (!Result.bSuccess)
        {
            OutWarnings.Add(FString::Printf(TEXT("Connection failed: %s -> %s: %s"),
                *ConnectionParams[i].SourcePin, *ConnectionParams[i].TargetPin,
                Result.ErrorMessage.IsEmpty() ? TEXT("Unknown error") : *Result.ErrorMessage));
            ResultObj->SetStringField(TEXT("error"), Result.ErrorMessage);
        }

        OutConnectionResults.Add(ResultObj);
    }
} 
