#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataTable.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/InputSettings.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/SceneComponent.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_BreakStruct.h"
#include "Engine/UserDefinedStruct.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInput/Public/InputAction.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "K2Node_DynamicCast.h"
#include "EditorAssetLibrary.h"
#include "K2Node_ConstructObjectFromClass.h"

// Forward declaration
static UEdGraphNode* CreateSpecialNodeByName(const FString& NodeType, UEdGraph* Graph);

// No longer needed as we're using LogTemp
// DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCP, Log, All);

FUnrealMCPBlueprintNodeCommands::FUnrealMCPBlueprintNodeCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("connect_blueprint_nodes"))
    {
        return HandleConnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("add_blueprint_event_node"))
    {
        return HandleAddBlueprintEvent(Params);
    }
    else if (CommandType == TEXT("add_blueprint_function_node"))
    {
        return HandleAddBlueprintFunctionCall(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable"))
    {
        return HandleAddBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("add_blueprint_input_action_node"))
    {
        return HandleAddBlueprintInputActionNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_self_reference"))
    {
        return HandleAddBlueprintSelfReference(Params);
    }
    else if (CommandType == TEXT("find_blueprint_nodes"))
    {
        return HandleFindBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("add_blueprint_custom_event_node"))
    {
        return HandleAddBlueprintCustomEventNode(Params);
    }
    else if (CommandType == TEXT("get_variable_info"))
    {
        return HandleGetVariableInfo(Params);
    }
    else if (CommandType == TEXT("add_enhanced_input_action_node"))
    {
        return HandleAddEnhancedInputActionNode(Params);
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint node command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString TargetNodeId;
    if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin"), SourcePinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the nodes
    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId)
        {
            SourceNode = Node;
        }
        else if (Node->NodeGuid.ToString() == TargetNodeId)
        {
            TargetNode = Node;
        }
    }

    if (!SourceNode || !TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    // Connect the nodes
    if (FUnrealMCPCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        // Mark the blueprint as modified
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect nodes"));
}


// Helper function to extract pin information from a node
static TSharedPtr<FJsonObject> CreateNodeWithPinInfo(UEdGraphNode* Node)
{
    if (!Node)
    {
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> NodeInfo = MakeShared<FJsonObject>();
    NodeInfo->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
    NodeInfo->SetStringField(TEXT("node_type"), Node->GetClass()->GetName());
    
    // Get node title/name
    FText NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView);
    NodeInfo->SetStringField(TEXT("node_title"), NodeTitle.ToString());
    
    // Get all pins with detailed information
    TArray<TSharedPtr<FJsonValue>> PinsArray;
    TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
    TArray<TSharedPtr<FJsonValue>> InputPinsArray;
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin)
        {
            TSharedPtr<FJsonObject> PinInfo = MakeShared<FJsonObject>();
            PinInfo->SetStringField(TEXT("pin_id"), Pin->PinId.ToString());
            PinInfo->SetStringField(TEXT("pin_name"), Pin->PinName.ToString());
            PinInfo->SetStringField(TEXT("pin_type"), Pin->PinType.PinCategory.ToString());
            PinInfo->SetBoolField(TEXT("is_input"), Pin->Direction == EGPD_Input);
            PinInfo->SetBoolField(TEXT("is_output"), Pin->Direction == EGPD_Output);
            PinInfo->SetStringField(TEXT("default_value"), Pin->DefaultValue);
            
            // Add subcategory if available
            if (Pin->PinType.PinSubCategoryObject.IsValid())
            {
                PinInfo->SetStringField(TEXT("pin_subcategory"), Pin->PinType.PinSubCategoryObject->GetName());
            }
            
            // Add to main pins array
            PinsArray.Add(MakeShared<FJsonValueObject>(PinInfo));
            
            // Also categorize by direction for easy access
            if (Pin->Direction == EGPD_Output)
            {
                OutputPinsArray.Add(MakeShared<FJsonValueObject>(PinInfo));
            }
            else if (Pin->Direction == EGPD_Input)
            {
                InputPinsArray.Add(MakeShared<FJsonValueObject>(PinInfo));
            }
        }
    }
    
    NodeInfo->SetArrayField(TEXT("pins"), PinsArray);
    NodeInfo->SetArrayField(TEXT("output_pins"), OutputPinsArray);
    NodeInfo->SetArrayField(TEXT("input_pins"), InputPinsArray);
    
    return NodeInfo;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the event node
    UK2Node_Event* EventNode = FUnrealMCPCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    if (!EventNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Return detailed node information including all pins
    return CreateNodeWithPinInfo(EventNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Check for target parameter (optional)
    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Special case: Create Widget node - MUST BE BEFORE CreateSpecialNodeByName
    if (FunctionName.Equals(TEXT("Create Widget"), ESearchCase::IgnoreCase) || 
        FunctionName.Equals(TEXT("CreateWidget"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Display, TEXT("Creating Create Widget node with enhanced UE5.6 compatibility"));
        
        // Find the CreateWidget function in UWidgetBlueprintLibrary
        UClass* WidgetBlueprintLibraryClass = UWidgetBlueprintLibrary::StaticClass();
        UFunction* CreateWidgetFunction = nullptr;
        
        // UE5.6 standard: Try "Create" first as it's the standard name
        CreateWidgetFunction = WidgetBlueprintLibraryClass->FindFunctionByName(TEXT("Create"));
        
        if (!CreateWidgetFunction)
        {
            // Fallback to other possible names for older versions
            TArray<FString> FallbackNames = {
                TEXT("CreateWidget"),
                TEXT("CreateWidgetInstance"),
                TEXT("CreateWidgetOfClass")
            };
            
            for (const FString& FuncName : FallbackNames)
            {
                CreateWidgetFunction = WidgetBlueprintLibraryClass->FindFunctionByName(*FuncName);
                if (CreateWidgetFunction)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found CreateWidget function with fallback name: %s"), *FuncName);
                    break;
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("Found UE5.6 standard Create function"));
        }
        
        if (!CreateWidgetFunction)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find CreateWidget function in UWidgetBlueprintLibrary. Ensure UMG module is loaded."));
        }
        
        // Create the function call node
        UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
        FunctionNode->FunctionReference.SetExternalMember(FName(*CreateWidgetFunction->GetName()), UWidgetBlueprintLibrary::StaticClass());
        FunctionNode->NodePosX = NodePosition.X;
        FunctionNode->NodePosY = NodePosition.Y;
        
        EventGraph->AddNode(FunctionNode, true);
        FunctionNode->CreateNewGuid();
        FunctionNode->PostPlacedNewNode();
        FunctionNode->AllocateDefaultPins();
        
        UE_LOG(LogTemp, Display, TEXT("Created Create Widget node with function: %s"), *CreateWidgetFunction->GetName());
        
        // Find the widget class parameter pin - try multiple possible names for compatibility
        UEdGraphPin* WidgetTypePin = nullptr;
        TArray<FString> PossiblePinNames = {
            TEXT("WidgetType"),  // UE5.6 primary
            TEXT("Class"),       // Common alternative
            TEXT("WidgetClass")  // Legacy support
        };
        
        for (const FString& PinName : PossiblePinNames)
        {
            WidgetTypePin = FUnrealMCPCommonUtils::FindPin(FunctionNode, PinName, EGPD_Input);
            if (WidgetTypePin)
            {
                UE_LOG(LogTemp, Display, TEXT("Found widget class pin: %s"), *PinName);
                break;
            }
        }
        
        if (!WidgetTypePin)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not find widget class pin in Create Widget node"));
            // Debug: List all available input pins
            for (UEdGraphPin* Pin : FunctionNode->Pins)
            {
                if (Pin->Direction == EGPD_Input)
                {
                    UE_LOG(LogTemp, Display, TEXT("  Available input pin: %s (Category: %s)"), 
                           *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString());
                }
            }
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not find widget class pin in Create Widget node. This may indicate a UE version compatibility issue."));
        }
        
        // Set parameters if provided
        if (Params->HasField(TEXT("params")))
        {
            const TSharedPtr<FJsonObject>* ParamsObj;
            if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
            {
                // Process parameters
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
                {
                    const FString& ParamName = Param.Key;
                    const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                    
                    // Handle Class parameter with enhanced logic
                    if ((ParamName.Equals(TEXT("Class"), ESearchCase::IgnoreCase) || 
                         ParamName.Equals(TEXT("WidgetType"), ESearchCase::IgnoreCase) ||
                         ParamName.Equals(TEXT("WidgetClass"), ESearchCase::IgnoreCase)) && 
                        ParamValue->Type == EJson::String)
                    {
                        FString WidgetClassPath = ParamValue->AsString();
                        UE_LOG(LogTemp, Display, TEXT("Processing widget class parameter: %s"), *WidgetClassPath);
                        
                        // Use the new enhanced asset discovery utilities
                        UClass* WidgetClass = FUnrealMCPCommonUtils::FindWidgetClass(WidgetClassPath);
                        
                        // Validate the found class
                        if (!WidgetClass)
                        {
                            UE_LOG(LogTemp, Error, TEXT("Failed to find widget class: %s"), *WidgetClassPath);
                            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                                TEXT("Failed to find widget class: %s. Please ensure the widget blueprint exists in your project. Common locations searched include Widgets/, UI/, Blueprints/, and other standard directories."), 
                                *WidgetClassPath));
                        }
                        
                        if (!WidgetClass->IsChildOf(UUserWidget::StaticClass()))
                        {
                            UE_LOG(LogTemp, Error, TEXT("Class '%s' is not a UserWidget subclass"), *WidgetClass->GetName());
                            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                                TEXT("Class '%s' is not a UserWidget subclass. Only UserWidget-derived classes can be used with Create Widget."), 
                                *WidgetClass->GetName()));
                        }
                        
                        // Set the class on the pin using the most compatible method
                        const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                        if (K2Schema)
                        {
                            K2Schema->TrySetDefaultObject(*WidgetTypePin, WidgetClass);
                            UE_LOG(LogTemp, Display, TEXT("Set widget class using K2Schema: %s"), *WidgetClass->GetName());
                        }
                        else
                        {
                            // Fallback: direct assignment
                            WidgetTypePin->DefaultObject = WidgetClass;
                            UE_LOG(LogTemp, Display, TEXT("Set widget class using direct assignment: %s"), *WidgetClass->GetName());
                        }
                        
                        // Force node reconstruction to update the UI
                        FunctionNode->ReconstructNode();
                    }
                    else
                    {
                        // Handle other parameters (OwningPlayer, etc.)
                        UEdGraphPin* ParamPin = FUnrealMCPCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
                        if (ParamPin)
                        {
                            if (ParamValue->Type == EJson::String)
                            {
                                ParamPin->DefaultValue = ParamValue->AsString();
                            }
                            else if (ParamValue->Type == EJson::Number)
                            {
                                ParamPin->DefaultValue = FString::SanitizeFloat(ParamValue->AsNumber());
                            }
                            else if (ParamValue->Type == EJson::Boolean)
                            {
                                ParamPin->DefaultValue = ParamValue->AsBool() ? TEXT("true") : TEXT("false");
                            }
                            UE_LOG(LogTemp, Display, TEXT("Set parameter '%s' to '%s'"), *ParamName, *ParamPin->DefaultValue);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Parameter pin '%s' not found in Create Widget node"), *ParamName);
                        }
                    }
                }
            }
        }
        
        // Mark the blueprint as modified
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        
        TSharedPtr<FJsonObject> ResultObj = CreateNodeWithPinInfo(FunctionNode);
        return ResultObj;
    }

    // Generic special node creation
    UEdGraphNode* SpecialNode = CreateSpecialNodeByName(FunctionName, EventGraph);
    if (SpecialNode)
    {
        SpecialNode->NodePosX = NodePosition.X;
        SpecialNode->NodePosY = NodePosition.Y;
        // If this is a comment node, set text and size if provided
        
        EventGraph->AddNode(SpecialNode, true);
        SpecialNode->CreateNewGuid();
        SpecialNode->PostPlacedNewNode();
        SpecialNode->AllocateDefaultPins();
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        return CreateNodeWithPinInfo(SpecialNode);
    }

    // Special case: Break Struct node
    if (FunctionName.Equals(TEXT("BreakStruct"), ESearchCase::IgnoreCase) && Params->HasField(TEXT("struct_type")))
    {
        FString StructTypeName = Params->GetStringField(TEXT("struct_type"));
        UScriptStruct* StructType = nullptr;
        // Try to find the struct by name (try F prefix and without)
        FString TryNames[] = { StructTypeName, FString::Printf(TEXT("F%s"), *StructTypeName) };
        for (const FString& Name : TryNames)
        {
            StructType = FindObject<UScriptStruct>(nullptr, *Name);
            if (StructType) break;
        }
        if (!StructType)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct type not found: %s"), *StructTypeName));
        }
        UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(EventGraph);
        BreakNode->StructType = StructType;
        BreakNode->NodePosX = NodePosition.X;
        BreakNode->NodePosY = NodePosition.Y;
        EventGraph->AddNode(BreakNode, true);
        BreakNode->CreateNewGuid();
        BreakNode->PostPlacedNewNode();
        BreakNode->AllocateDefaultPins();
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        return CreateNodeWithPinInfo(BreakNode);
    }

    // Find the function
    UFunction* Function = nullptr;
    UK2Node_CallFunction* FunctionNode = nullptr;
    
    // Add extensive logging for debugging
    UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in target '%s'"), 
           *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target);
    
    // Check if we have a target class specified
    if (!Target.IsEmpty())
    {
        // Use enhanced asset discovery instead of hardcoded paths
        UClass* TargetClass = nullptr;
        
        // Strategy 1: Try engine and core paths for built-in classes
        TArray<FString> BuiltInPaths;
        BuiltInPaths.Add(FUnrealMCPCommonUtils::BuildEnginePath(Target));
        BuiltInPaths.Add(FUnrealMCPCommonUtils::BuildCorePath(Target));
        
        for (const FString& ClassPath : BuiltInPaths)
        {
            TargetClass = LoadObject<UClass>(nullptr, *ClassPath);
            if (TargetClass)
            {
                UE_LOG(LogTemp, Display, TEXT("Found target class via built-in path: %s"), *TargetClass->GetName());
                break;
            }
        }
        
        // Strategy 2: Use enhanced asset discovery for game classes
        if (!TargetClass)
        {
            TArray<FString> SearchPaths = FUnrealMCPCommonUtils::GetCommonAssetSearchPaths(Target);
            for (const FString& SearchPath : SearchPaths)
            {
                // Try to load as blueprint class with _C suffix
                FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *SearchPath, *FPaths::GetBaseFilename(SearchPath));
                TargetClass = LoadObject<UClass>(nullptr, *ClassPath);
                if (TargetClass)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found target class via enhanced search: %s"), *TargetClass->GetName());
                    break;
                }
            }
        }

        // Strategy 3: Try with U prefix if not found (for engine classes)
        if (!TargetClass)
        {
            FString TargetWithPrefix = FString::Printf(TEXT("U%s"), *Target);
            TArray<FString> PrefixedPaths;
            PrefixedPaths.Add(FUnrealMCPCommonUtils::BuildEnginePath(TargetWithPrefix));
            PrefixedPaths.Add(FUnrealMCPCommonUtils::BuildCorePath(TargetWithPrefix));
            
            for (const FString& Path : PrefixedPaths)
            {
                TargetClass = LoadObject<UClass>(nullptr, *Path);
                if (TargetClass)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found target class with U prefix: %s"), *TargetClass->GetName());
                    break;
                }
            }
        }

        // For UGameplayStatics specific lookup
        if (!TargetClass && Target.Equals(TEXT("GameplayStatics")))
        {
            UE_LOG(LogTemp, Display, TEXT("Using special case handling for GameplayStatics::GetActorOfClass"));
            
            // Create the function node directly
            FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
            if (FunctionNode)
            {
                // Direct setup for known function
                FunctionNode->FunctionReference.SetExternalMember(
                    FName(TEXT("GetActorOfClass")), 
                    TargetClass
                );
                
                FunctionNode->NodePosX = NodePosition.X;
                FunctionNode->NodePosY = NodePosition.Y;
                EventGraph->AddNode(FunctionNode);
                FunctionNode->CreateNewGuid();
                FunctionNode->PostPlacedNewNode();
                FunctionNode->AllocateDefaultPins();
                
                UE_LOG(LogTemp, Display, TEXT("Created GetActorOfClass node directly"));
                
                // List all pins
                for (UEdGraphPin* Pin : FunctionNode->Pins)
                {
                    UE_LOG(LogTemp, Display, TEXT("  - Pin: %s, Direction: %d, Category: %s"), 
                           *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
                }
            }
        }
        
        // If we found a target class, look for the function there
        if (TargetClass)
        {
            UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in class '%s'"), 
                   *FunctionName, *TargetClass->GetName());
                   
            // First try exact name
            Function = TargetClass->FindFunctionByName(*FunctionName);
            
            // If not found, try class hierarchy
            UClass* CurrentClass = TargetClass;
            while (!Function && CurrentClass)
            {
                UE_LOG(LogTemp, Display, TEXT("Searching in class: %s"), *CurrentClass->GetName());
                
                // Try exact match
                Function = CurrentClass->FindFunctionByName(*FunctionName);
                
                // Try case-insensitive match
                if (!Function)
                {
                    for (TFieldIterator<UFunction> FuncIt(CurrentClass); FuncIt; ++FuncIt)
                    {
                        UFunction* AvailableFunc = *FuncIt;
                        UE_LOG(LogTemp, Display, TEXT("  - Available function: %s"), *AvailableFunc->GetName());
                        
                        if (AvailableFunc->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive match: %s"), *AvailableFunc->GetName());
                            Function = AvailableFunc;
                            break;
                        }
                    }
                }
                
                // Move to parent class
                CurrentClass = CurrentClass->GetSuperClass();
            }
            
            // Special handling for known functions
            if (!Function)
            {
                if (TargetClass->GetName() == TEXT("GameplayStatics") && 
                    (FunctionName == TEXT("GetActorOfClass") || FunctionName.Equals(TEXT("GetActorOfClass"), ESearchCase::IgnoreCase)))
                {
                    UE_LOG(LogTemp, Display, TEXT("Using special case handling for GameplayStatics::GetActorOfClass"));
                    
                    // Create the function node directly
                    FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
                    if (FunctionNode)
                    {
                        // Direct setup for known function
                        FunctionNode->FunctionReference.SetExternalMember(
                            FName(TEXT("GetActorOfClass")), 
                            TargetClass
                        );
                        
                        FunctionNode->NodePosX = NodePosition.X;
                        FunctionNode->NodePosY = NodePosition.Y;
                        EventGraph->AddNode(FunctionNode);
                        FunctionNode->CreateNewGuid();
                        FunctionNode->PostPlacedNewNode();
                        FunctionNode->AllocateDefaultPins();
                        
                        UE_LOG(LogTemp, Display, TEXT("Created GetActorOfClass node directly"));
                        
                        // List all pins
                        for (UEdGraphPin* Pin : FunctionNode->Pins)
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Pin: %s, Direction: %d, Category: %s"), 
                                   *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
                        }
                    }
                }
            }
        }
    }
    
    // If we still haven't found the function, try in the blueprint's class
    if (!Function && !FunctionNode)
    {
        UE_LOG(LogTemp, Display, TEXT("Trying to find function in blueprint class"));
        Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
    }
    
    // Create the function call node if we found the function
    if (Function && !FunctionNode)
    {
        FunctionNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
    }
    
    if (!FunctionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target));
    }

    // Set parameters if provided
    if (Params->HasField(TEXT("params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj;
        if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
        {
            // Process parameters
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
            {
                const FString& ParamName = Param.Key;
                const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                
                // Find the parameter pin
                UEdGraphPin* ParamPin = FUnrealMCPCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
                if (ParamPin)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found parameter pin '%s' of category '%s'"), 
                           *ParamName, *ParamPin->PinType.PinCategory.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  Current default value: '%s'"), *ParamPin->DefaultValue);
                    if (ParamPin->PinType.PinSubCategoryObject.IsValid())
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Pin subcategory: '%s'"), 
                               *ParamPin->PinType.PinSubCategoryObject->GetName());
                    }
                    
                    // Set parameter based on type
                    if (ParamValue->Type == EJson::String)
                    {
                        FString StringVal = ParamValue->AsString();
                        UE_LOG(LogTemp, Display, TEXT("  Setting string parameter '%s' to: '%s'"), 
                               *ParamName, *StringVal);
                        
                        // Handle class reference parameters (e.g., ActorClass in GetActorOfClass)
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                        {
                            // For class references, we require the exact class name with proper prefix
                            // - Actor classes must start with 'A' (e.g., ACameraActor)
                            // - Non-actor classes must start with 'U' (e.g., UObject)
                            const FString& ClassName = StringVal;
                            
                            // TODO: This likely won't work in UE5.5+, so don't rely on it.
                            UClass* Class = LoadObject<UClass>(nullptr, *ClassName);

                            if (!Class)
                            {
                                // Try with Engine module path
                                FString EngineClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
                                Class = LoadObject<UClass>(nullptr, *EngineClassName);
                                UE_LOG(LogTemp, Display, TEXT("Trying Engine module path: %s"), *EngineClassName);
                            }
                            
                            if (!Class)
                            {
                                UE_LOG(LogTemp, Error, TEXT("Failed to find class '%s'. Make sure to use the exact class name with proper prefix (A for actors, U for non-actors)"), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to find class '%s'"), *ClassName));
                            }

                            const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                            if (!K2Schema)
                            {
                                UE_LOG(LogTemp, Error, TEXT("Failed to get K2Schema"));
                                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get K2Schema"));
                            }

                            K2Schema->TrySetDefaultObject(*ParamPin, Class);
                            if (ParamPin->DefaultObject != Class)
                            {
                                UE_LOG(LogTemp, Error, TEXT("Failed to set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to set class reference for pin '%s'"), *ParamPin->PinName.ToString()));
                            }

                            UE_LOG(LogTemp, Log, TEXT("Successfully set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                            continue;
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            bool BoolValue = ParamValue->AsBool();
                            ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                            UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                                   *ParamName, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
                        {
                            // Handle array parameters - like Vector parameters
                            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                            if (ParamValue->TryGetArray(ArrayValue))
                            {
                                // Check if this could be a vector (array of 3 numbers)
                                if (ArrayValue->Num() == 3)
                                {
                                    // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                    float X = (*ArrayValue)[0]->AsNumber();
                                    float Y = (*ArrayValue)[1]->AsNumber();
                                    float Z = (*ArrayValue)[2]->AsNumber();
                                    
                                    FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                    ParamPin->DefaultValue = VectorString;
                                    
                                    UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                           *ParamName, *VectorString);
                                    UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                           *ParamPin->DefaultValue);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                                }
                            }
                        }
                    }
                    else if (ParamValue->Type == EJson::Number)
                    {
                        // Handle integer vs float parameters correctly
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                    }
                    else if (ParamValue->Type == EJson::Boolean)
                    {
                        bool BoolValue = ParamValue->AsBool();
                        ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                        UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                               *ParamName, *ParamPin->DefaultValue);
                    }
                    else if (ParamValue->Type == EJson::Array)
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Processing array parameter '%s'"), *ParamName);
                        // Handle array parameters - like Vector parameters
                        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                        if (ParamValue->TryGetArray(ArrayValue))
                        {
                            // Check if this could be a vector (array of 3 numbers)
                            if (ArrayValue->Num() == 3 && 
                                (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct) &&
                                (ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()))
                            {
                                // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                float X = (*ArrayValue)[0]->AsNumber();
                                float Y = (*ArrayValue)[1]->AsNumber();
                                float Z = (*ArrayValue)[2]->AsNumber();
                                
                                FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                ParamPin->DefaultValue = VectorString;
                                
                                UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                       *ParamName, *VectorString);
                                UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                       *ParamPin->DefaultValue);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                            }
                        }
                    }
                    // Add handling for other types as needed
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Parameter pin '%s' not found"), *ParamName);
                }
            }
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = CreateNodeWithPinInfo(FunctionNode);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    // Get optional parameters
    bool IsExposed = false;
    if (Params->HasField(TEXT("is_exposed")))
    {
        IsExposed = Params->GetBoolField(TEXT("is_exposed"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create variable based on type
    FEdGraphPinType PinType;
    bool bTypeResolved = false;

    auto SetPinTypeForCategory = [&](auto Category, UObject* SubCategoryObject = nullptr) {
        if constexpr (std::is_same_v<std::decay_t<decltype(Category)>, FName>) {
            PinType.PinCategory = Category;
        } else {
            PinType.PinCategory = FName(Category);
        }
        PinType.PinSubCategoryObject = SubCategoryObject;
        bTypeResolved = true;
    };

    FString TypeStr = VariableType;
    if (TypeStr.StartsWith(TEXT("/")))
    {
        TypeStr.RemoveFromStart(TEXT("/"));
    }
    TypeStr.TrimStartAndEndInline();

    // Handle array, set, map containers
    if (TypeStr.EndsWith(TEXT("[]"))) {
        // Array type
        FString InnerType = TypeStr.LeftChop(2);
        InnerType.TrimStartAndEndInline();
        // Recursively resolve inner type
        FEdGraphPinType InnerPinType;
        bool bInnerResolved = false;
        // Use a lambda for recursion
        auto ResolveType = [&](const FString& InType, FEdGraphPinType& OutPinType, bool& bResolved) {
            // Built-in types
            if (InType.Equals(TEXT("Float"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Float;
                bResolved = true;
            } else if (InType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
                bResolved = true;
            } else if (InType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) || InType.Equals(TEXT("Int"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
                bResolved = true;
            } else if (InType.Equals(TEXT("String"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
                bResolved = true;
            } else if (InType.Equals(TEXT("Name"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
                bResolved = true;
            } else if (InType.Equals(TEXT("Text"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
                bResolved = true;
            } else if (InType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
                bResolved = true;
            } else if (InType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
                bResolved = true;
            } else if (InType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
                bResolved = true;
            } else if (InType.Equals(TEXT("Color"), ESearchCase::IgnoreCase)) {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
                bResolved = true;
            } else {
                // Use enhanced struct discovery instead of inline searching
                UScriptStruct* FoundStruct = FUnrealMCPCommonUtils::FindStructType(InType);
                if (FoundStruct) {
                    OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                    OutPinType.PinSubCategoryObject = FoundStruct;
                    bResolved = true;
                } else {
                    // Try enum
                    UEnum* FoundEnum = nullptr;
                    TArray<FString> EnumNameVariations;
                    EnumNameVariations.Add(InType);
                    EnumNameVariations.Add(FString::Printf(TEXT("E%s"), *InType));
                    EnumNameVariations.Add(FUnrealMCPCommonUtils::BuildEnginePath(InType));
                    
                    for (const FString& EnumVariation : EnumNameVariations) {
                        FoundEnum = LoadObject<UEnum>(nullptr, *EnumVariation);
                        if (FoundEnum) break;
                    }
                    
                    if (FoundEnum) {
                        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
                        OutPinType.PinSubCategoryObject = FoundEnum;
                        bResolved = true;
                    } else {
                        // Try object/class using enhanced asset discovery
                        UClass* FoundClass = nullptr;
                        
                        // Use enhanced asset discovery for class search
                        TArray<FString> ClassSearchPaths = FUnrealMCPCommonUtils::GetCommonAssetSearchPaths(InType);
                        for (const FString& SearchPath : ClassSearchPaths)
                        {
                            // Try to load as blueprint class with _C suffix
                            FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *SearchPath, *FPaths::GetBaseFilename(SearchPath));
                            FoundClass = LoadClass<UObject>(nullptr, *ClassPath);
                            if (FoundClass)
                            {
                                UE_LOG(LogTemp, Display, TEXT("Found class via enhanced search: %s"), *FoundClass->GetName());
                                break;
                            }
                        }
                        
                        // Fallback: try engine path
                        if (!FoundClass)
                        {
                            FString EngineClassName = FUnrealMCPCommonUtils::BuildEnginePath(InType);
                            FoundClass = LoadObject<UClass>(nullptr, *EngineClassName);
                            if (FoundClass)
                            {
                                UE_LOG(LogTemp, Display, TEXT("Found class via engine path: %s"), *FoundClass->GetName());
                            }
                        }
                        
                        if (FoundClass)
                        {
                            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                            OutPinType.PinSubCategoryObject = FoundClass;
                            bResolved = true;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Could not find type for array element: %s"), *InType);
                            bResolved = false;
                        }
                    }
                }
            }
            
            // Always return success, the bResolved flag will indicate if the type was actually resolved
            return TSharedPtr<FJsonObject>();
        };

        ResolveType(InnerType, InnerPinType, bInnerResolved);
        if (bInnerResolved) {
            PinType = InnerPinType;
            PinType.ContainerType = EPinContainerType::Array;
            bTypeResolved = true;
        }
    } else {
        // Built-in types
        if (TypeStr.Equals(TEXT("Float"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Float);
        } else if (TypeStr.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Boolean);
        } else if (TypeStr.Equals(TEXT("Integer"), ESearchCase::IgnoreCase) || TypeStr.Equals(TEXT("Int"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Int);
        } else if (TypeStr.Equals(TEXT("String"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_String);
        } else if (TypeStr.Equals(TEXT("Name"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Name);
        } else if (TypeStr.Equals(TEXT("Text"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Text);
        } else if (TypeStr.Equals(TEXT("Vector"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
        } else if (TypeStr.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FRotator>::Get());
        } else if (TypeStr.Equals(TEXT("Transform"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FTransform>::Get());
        } else if (TypeStr.Equals(TEXT("Color"), ESearchCase::IgnoreCase)) {
            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FLinearColor>::Get());
        } else if (TypeStr.StartsWith(TEXT("Class<")) && TypeStr.EndsWith(TEXT(">"))) {
            // Handle class reference types like "Class<WBP_DialogueWidget>" or "Class<UserWidget>"
            FString InnerType = TypeStr.Mid(6, TypeStr.Len() - 7); // Remove "Class<" and ">"
            InnerType.TrimStartAndEndInline();
            
            UClass* TargetClass = nullptr;
            
            // Handle common base classes
            if (InnerType.Equals(TEXT("UserWidget"), ESearchCase::IgnoreCase)) {
                TargetClass = UUserWidget::StaticClass();
            } else if (InnerType.Equals(TEXT("Actor"), ESearchCase::IgnoreCase)) {
                TargetClass = AActor::StaticClass();
            } else if (InnerType.Equals(TEXT("Pawn"), ESearchCase::IgnoreCase)) {
                TargetClass = APawn::StaticClass();
            } else {
                // Try to find specific widget blueprint class
                FString CleanPath = InnerType;
                
                // Use enhanced asset discovery instead of hardcoded paths
                UClass* FoundClass = FUnrealMCPCommonUtils::FindWidgetClass(CleanPath);
                
                if (FoundClass)
                {
                    TargetClass = FoundClass;
                }
                else
                {
                    // Fallback: try to load as blueprint using common asset search paths
                    TArray<FString> SearchPaths = FUnrealMCPCommonUtils::GetCommonAssetSearchPaths(CleanPath);
                    for (const FString& SearchPath : SearchPaths)
                    {
                        // Try to load the class
                        FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *SearchPath, *FPaths::GetBaseFilename(SearchPath));
                        TargetClass = LoadClass<UObject>(nullptr, *ClassPath);
                        if (TargetClass)
                        {
                            break;
                        }
                    }
                    
                    if (!TargetClass) {
                        UE_LOG(LogTemp, Warning, TEXT("Could not find class for class reference: %s"), *InnerType);
                        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not find class for class reference: %s"), *InnerType));
                    }
                }
            }
            
            if (TargetClass) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Class, TargetClass);
            }
        } else if (TypeStr.StartsWith(TEXT("WBP_")) && TypeStr.EndsWith(TEXT("Class"))) {
            // Handle "WBP_DialogueWidgetClass" style naming
            FString WidgetName = TypeStr.LeftChop(5); // Remove "Class"
            
            // Use enhanced asset discovery instead of hardcoded path
            UClass* WidgetClass = FUnrealMCPCommonUtils::FindWidgetClass(WidgetName);
            
            if (WidgetClass) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Class, WidgetClass);
            } else {
                UE_LOG(LogTemp, Warning, TEXT("Could not find widget class: %s"), *WidgetName);
                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not find widget class: %s"), *WidgetName));
            }
        } else {
            // Try struct
            UScriptStruct* FoundStruct = nullptr;
            TArray<FString> StructNameVariations;
            StructNameVariations.Add(TypeStr);
            StructNameVariations.Add(FString::Printf(TEXT("F%s"), *TypeStr));
            
            // Use enhanced asset discovery for struct search paths instead of hardcoded paths
            TArray<FString> StructDirectories = {
                TEXT("/Game/DataStructures/"),
                TEXT("/Game/Data/"),
                TEXT("/Game/Blueprints/DataStructures/"),
                TEXT("/Game/Blueprints/"),
                TEXT("/Game/")
            };

            for (const FString& StructDir : StructDirectories)
            {
                StructNameVariations.Add(FString::Printf(TEXT("%s%s.%s"), *StructDir, *TypeStr, *TypeStr));
                StructNameVariations.Add(FString::Printf(TEXT("%sF%s.F%s"), *StructDir, *TypeStr, *TypeStr));
            }
            
            // Add engine paths as fallback
            StructNameVariations.Add(FUnrealMCPCommonUtils::BuildEnginePath(TypeStr));
            
            for (const FString& StructVariation : StructNameVariations) {
                FoundStruct = LoadObject<UScriptStruct>(nullptr, *StructVariation);
                if (FoundStruct) {
                    UE_LOG(LogTemp, Display, TEXT("Found struct via enhanced search: %s"), *FoundStruct->GetName());
                    break;
                }
            }
            
            if (FoundStruct) {
                SetPinTypeForCategory(UEdGraphSchema_K2::PC_Struct, FoundStruct);
            } else {
                // Try object/class
                UClass* FoundClass = nullptr;
                
                // Clean up the path by removing any redundant slashes
                FString CleanPath = TypeStr;
                if (CleanPath.StartsWith(TEXT("/")))
                {
                    CleanPath.RemoveFromStart(TEXT("/"));
                }
                CleanPath.TrimStartAndEndInline();
                
                // Handle Game prefix variations
                if (!CleanPath.StartsWith(TEXT("/")))
                {
                    if (CleanPath.StartsWith(TEXT("Game/")))
                    {
                        CleanPath = FString::Printf(TEXT("/%s"), *CleanPath);
                    }
                    else
                    {
                        CleanPath = FString::Printf(TEXT("/Game/%s"), *CleanPath);
                    }
                }
                
                // Try direct path first
                FString DirectPath = FString::Printf(TEXT("%s.%s_C"), *CleanPath, *FPaths::GetBaseFilename(CleanPath));
                UE_LOG(LogTemp, Display, TEXT("Trying direct path: %s"), *DirectPath);
                FoundClass = LoadClass<UObject>(nullptr, *DirectPath);
                
                // If not found, try standard class loading
                if (!FoundClass)
                {
                    FoundClass = LoadObject<UClass>(nullptr, *CleanPath);
                }
                
                // Try engine path if still not found
                if (!FoundClass)
                {
                    FString EngineClassName = FUnrealMCPCommonUtils::BuildEnginePath(CleanPath);
                    FoundClass = LoadObject<UClass>(nullptr, *EngineClassName);
                }
                
                if (FoundClass)
                {
                    SetPinTypeForCategory(UEdGraphSchema_K2::PC_Object, FoundClass);
                }
                else
                {
                    // Special handling for DataTable references
                    if (TypeStr.Contains(TEXT("Game/")))
                    {
                        // Ensure path starts with a forward slash
                        FString DataTablePath = TypeStr;
                        if (!DataTablePath.StartsWith(TEXT("/")))
                        {
                            DataTablePath = TEXT("/") + DataTablePath;
                        }
                        
                        // Try to load the DataTable asset
                        UDataTable* DataTableAsset = LoadObject<UDataTable>(nullptr, *DataTablePath);
                        if (DataTableAsset)
                        {
                            // For DataTables, we use Object reference with DataTable class
                            UClass* DataTableClass = UDataTable::StaticClass();
                            SetPinTypeForCategory(UEdGraphSchema_K2::PC_Object, DataTableClass);
                        }
                        else
                        {
                            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not find DataTable asset: %s"), *DataTablePath));
                        }
                    }
                    else
                    {
                        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not find class for type: %s"), *TypeStr));
                    }
                }
            }
        }
    }
    if (!bTypeResolved) {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not resolve variable type: %s"), *VariableType));
    }

    // Create the variable
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Set variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // Set exposure in editor
        if (IsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit;
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the input action node
    UK2Node_InputAction* InputActionNode = FUnrealMCPCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the self node
    UK2Node_Self* SelfNode = FUnrealMCPCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get optional node_type parameter
    FString NodeType;
    if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
    {
        NodeType = TEXT("All"); // Default to finding all nodes
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create a JSON array for detailed node information
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    
    UE_LOG(LogTemp, Display, TEXT("Searching for nodes of type '%s' in blueprint '%s'"), *NodeType, *BlueprintName);
    UE_LOG(LogTemp, Display, TEXT("Total nodes in graph: %d"), EventGraph->Nodes.Num());
    
    // Helper function to create detailed node info
    auto CreateNodeInfo = [](UEdGraphNode* Node) -> TSharedPtr<FJsonObject> {
        TSharedPtr<FJsonObject> NodeInfo = MakeShared<FJsonObject>();
        NodeInfo->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        NodeInfo->SetStringField(TEXT("node_type"), Node->GetClass()->GetName());
        
        // Get node title/name
        FText NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView);
        NodeInfo->SetStringField(TEXT("node_title"), NodeTitle.ToString());
        
        // Get all pins
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin)
            {
                TSharedPtr<FJsonObject> PinInfo = MakeShared<FJsonObject>();
                PinInfo->SetStringField(TEXT("pin_name"), Pin->PinName.ToString());
                PinInfo->SetStringField(TEXT("pin_type"), Pin->PinType.PinCategory.ToString());
                PinInfo->SetBoolField(TEXT("is_input"), Pin->Direction == EGPD_Input);
                PinInfo->SetBoolField(TEXT("is_output"), Pin->Direction == EGPD_Output);
                PinInfo->SetStringField(TEXT("default_value"), Pin->DefaultValue);
                
                // Add subcategory if available
                if (Pin->PinType.PinSubCategoryObject.IsValid())
                {
                    PinInfo->SetStringField(TEXT("pin_subcategory"), Pin->PinType.PinSubCategoryObject->GetName());
                }
                
                PinsArray.Add(MakeShared<FJsonValueObject>(PinInfo));
            }
        }
        NodeInfo->SetArrayField(TEXT("pins"), PinsArray);
        
        return NodeInfo;
    };
    
    // Filter nodes by the requested type
    if (NodeType.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
    {
        // Get optional event_name parameter for specific event filtering
        FString EventName;
        bool bHasSpecificEvent = Params->TryGetStringField(TEXT("event_name"), EventName);
        
        UE_LOG(LogTemp, Display, TEXT("Looking for Event nodes%s"), 
               bHasSpecificEvent ? *FString::Printf(TEXT(" with name '%s'"), *EventName) : TEXT(" (all events)"));
        
        // Look for event nodes
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode)
            {
                FString EventNodeName = EventNode->EventReference.GetMemberName().ToString();
                UE_LOG(LogTemp, Display, TEXT("Found event node: %s"), *EventNodeName);
                
                // If specific event name is provided, filter by it
                if (bHasSpecificEvent)
                {
                    if (EventNodeName.Equals(EventName, ESearchCase::IgnoreCase))
                    {
                        UE_LOG(LogTemp, Display, TEXT("Event node matches filter: %s"), *EventNode->NodeGuid.ToString());
                        NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(EventNode)));
                    }
                }
                else
                {
                    // Add all event nodes
                    UE_LOG(LogTemp, Display, TEXT("Adding event node: %s"), *EventNode->NodeGuid.ToString());
                    NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(EventNode)));
                }
            }
        }
    }
    else if (NodeType.Equals(TEXT("Function"), ESearchCase::IgnoreCase))
    {
        // Look for function call nodes
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node);
            if (FunctionNode)
            {
                UE_LOG(LogTemp, Display, TEXT("Found function node: %s"), *FunctionNode->NodeGuid.ToString());
                NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(FunctionNode)));
            }
        }
    }
    else if (NodeType.Equals(TEXT("Variable"), ESearchCase::IgnoreCase))
    {
        // Look for variable nodes (get/set)
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node);
            if (VarGetNode)
            {
                UE_LOG(LogTemp, Display, TEXT("Found variable get node: %s"), *VarGetNode->NodeGuid.ToString());
                NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(VarGetNode)));
                continue;
            }
            
            UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node);
            if (VarSetNode)
            {
                UE_LOG(LogTemp, Display, TEXT("Found variable set node: %s"), *VarSetNode->NodeGuid.ToString());
                NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(VarSetNode)));
            }
        }
    }
    else if (NodeType.Equals(TEXT("All"), ESearchCase::IgnoreCase))
    {
        // Return all nodes with detailed information
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node)
            {
                UE_LOG(LogTemp, Display, TEXT("Found node: %s (Type: %s)"), 
                       *Node->NodeGuid.ToString(), 
                       *Node->GetClass()->GetName());
                NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(Node)));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Unsupported node type: %s"), *NodeType);
        // For unsupported types, we'll still search by class name
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node && Node->GetClass()->GetName().Contains(NodeType))
            {
                UE_LOG(LogTemp, Display, TEXT("Found node matching type '%s': %s"), *NodeType, *Node->NodeGuid.ToString());
                NodesArray.Add(MakeShared<FJsonValueObject>(CreateNodeInfo(Node)));
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("Found %d matching nodes"), NodesArray.Num());
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
    
    // Also include the old format for backwards compatibility
    TArray<TSharedPtr<FJsonValue>> NodeGuidArray;
    for (const TSharedPtr<FJsonValue>& NodeValue : NodesArray)
    {
        const TSharedPtr<FJsonObject>& NodeObj = NodeValue->AsObject();
        if (NodeObj.IsValid())
        {
            FString NodeId = NodeObj->GetStringField(TEXT("node_id"));
            NodeGuidArray.Add(MakeShared<FJsonValueString>(NodeId));
        }
    }
    ResultObj->SetArrayField(TEXT("node_guids"), NodeGuidArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintCustomEventNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));

    // Check for existing custom event node
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(Node);
        if (CustomEventNode && CustomEventNode->CustomFunctionName == FName(*EventName))
        {
            return CreateNodeWithPinInfo(CustomEventNode);
        }
    }

    // Create new custom event node
    UK2Node_CustomEvent* NewEventNode = NewObject<UK2Node_CustomEvent>(EventGraph);
    NewEventNode->CustomFunctionName = FName(*EventName);
    NewEventNode->NodePosX = NodePosition.X;
    NewEventNode->NodePosY = NodePosition.Y;
    EventGraph->AddNode(NewEventNode, true);
    NewEventNode->CreateNewGuid();
    NewEventNode->PostPlacedNewNode();
    NewEventNode->AllocateDefaultPins();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    return CreateNodeWithPinInfo(NewEventNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetVariableInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }
    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    FString TypeString = TEXT("Unknown");
    for (const FBPVariableDescription& Var : Blueprint->NewVariables)
    {
        if (Var.VarName == FName(*VariableName))
        {
            if (Var.VarType.PinCategory == UEdGraphSchema_K2::PC_Struct && Var.VarType.PinSubCategoryObject.IsValid())
            {
                // User-defined struct: return full asset path
                UScriptStruct* StructObj = Cast<UScriptStruct>(Var.VarType.PinSubCategoryObject.Get());
                if (StructObj)
                {
                    FString StructPath = StructObj->GetPathName();
                    // Remove dot notation if present (e.g., /Game/Blueprints/DialogueData.DialogueData -> /Game/Blueprints/DialogueData)
                    int32 DotIndex;
                    if (StructPath.FindChar('.', DotIndex))
                    {
                        StructPath = StructPath.Left(DotIndex);
                    }
                    TypeString = StructPath;
                }
            }
            else if (!Var.VarType.PinCategory.IsNone())
            {
                TypeString = Var.VarType.PinCategory.ToString();
            }
            else
            {
                TypeString = Var.FriendlyName;
            }
            break;
        }
    }
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_type"), TypeString);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddEnhancedInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActionPath;
    if (!Params->TryGetStringField(TEXT("action_path"), ActionPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_path' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Load the Input Action asset
    UInputAction* InputAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(ActionPath));
    if (!InputAction)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load Input Action: %s"), *ActionPath));
    }

    // For Enhanced Input in UE5, we need to create a UK2Node_EnhancedInputAction
    // First check if we have the Enhanced Input plugin classes available
    UClass* EnhancedInputActionNodeClass = FindObject<UClass>(ANY_PACKAGE, TEXT("K2Node_EnhancedInputAction"));
    if (!EnhancedInputActionNodeClass)
    {
        // Fallback: Try to find it through the class registry
        EnhancedInputActionNodeClass = UClass::TryFindTypeSlow<UClass>(TEXT("K2Node_EnhancedInputAction"));
    }
    
    if (!EnhancedInputActionNodeClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Enhanced Input node class not found. Make sure Enhanced Input plugin is enabled."));
    }

    // Check for existing Enhanced Input Action node for this action
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->GetClass() == EnhancedInputActionNodeClass)
        {
            // Check if this node is for our specific Input Action
            // We'll use reflection to check the InputAction property
            if (UObject** InputActionProperty = (UObject**)Node->GetClass()->FindPropertyByName(FName("InputAction")))
            {
                if (*InputActionProperty == InputAction)
                {
                    return CreateNodeWithPinInfo(Node);
                }
            }
        }
    }

    // Create new Enhanced Input Action node
    UEdGraphNode* NewInputNode = NewObject<UEdGraphNode>(EventGraph, EnhancedInputActionNodeClass);
    if (!NewInputNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Enhanced Input Action node"));
    }

    // Set the Input Action property using reflection
    FProperty* InputActionProperty = NewInputNode->GetClass()->FindPropertyByName(FName("InputAction"));
    if (InputActionProperty)
    {
        FObjectProperty* ObjectProperty = CastField<FObjectProperty>(InputActionProperty);
        if (ObjectProperty)
        {
            ObjectProperty->SetObjectPropertyValue_InContainer(NewInputNode, InputAction);
        }
    }

    // Set position
    NewInputNode->NodePosX = NodePosition.X;
    NewInputNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(NewInputNode, true);
    NewInputNode->CreateNewGuid();
    NewInputNode->PostPlacedNewNode();
    NewInputNode->AllocateDefaultPins();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    return CreateNodeWithPinInfo(NewInputNode);
}

// Helper function to create special Blueprint nodes by name
static UEdGraphNode* CreateSpecialNodeByName(const FString& NodeType, UEdGraph* Graph) {
    if (NodeType.Equals(TEXT("Branch"), ESearchCase::IgnoreCase)) {
        return NewObject<UK2Node_IfThenElse>(Graph);
    }
    if (NodeType.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase)) {
        return NewObject<UK2Node_ExecutionSequence>(Graph);
    }
    if (NodeType.Equals(TEXT("Print String"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* PrintStringNode = NewObject<UK2Node_CallFunction>(Graph);
        PrintStringNode->FunctionReference.SetExternalMember(
            FName(TEXT("PrintString")),
            UKismetSystemLibrary::StaticClass()
        );
        return PrintStringNode;
    }
    if (NodeType.Equals(TEXT("Is Valid"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* IsValidNode = NewObject<UK2Node_CallFunction>(Graph);
        IsValidNode->FunctionReference.SetExternalMember(
            FName(TEXT("IsValid")),
            UKismetSystemLibrary::StaticClass()
        );
        return IsValidNode;
    }
    if (NodeType.Equals(TEXT("Add to Viewport"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* AddToViewportNode = NewObject<UK2Node_CallFunction>(Graph);
        AddToViewportNode->FunctionReference.SetExternalMember(
            FName(TEXT("AddToViewport")),
            UUserWidget::StaticClass()
        );
        return AddToViewportNode;
    }
    if (NodeType.Equals(TEXT("Get Class"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* GetClassNode = NewObject<UK2Node_CallFunction>(Graph);
        GetClassNode->FunctionReference.SetExternalMember(
            FName(TEXT("GetClass")),
            UObject::StaticClass()
        );
        return GetClassNode;
    }
    if (NodeType.Equals(TEXT("Get Player Controller"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* GetPlayerControllerNode = NewObject<UK2Node_CallFunction>(Graph);
        GetPlayerControllerNode->FunctionReference.SetExternalMember(
            FName(TEXT("GetPlayerController")),
            UGameplayStatics::StaticClass()
        );
        return GetPlayerControllerNode;
    }
    if (NodeType.Equals(TEXT("Get Player Pawn"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* GetPlayerPawnNode = NewObject<UK2Node_CallFunction>(Graph);
        GetPlayerPawnNode->FunctionReference.SetExternalMember(
            FName(TEXT("GetPlayerPawn")),
            UGameplayStatics::StaticClass()
        );
        return GetPlayerPawnNode;
    }
    if (NodeType.Equals(TEXT("Get Player State"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* GetPlayerStateNode = NewObject<UK2Node_CallFunction>(Graph);
        GetPlayerStateNode->FunctionReference.SetExternalMember(
            FName(TEXT("GetPlayerState")),
            UGameplayStatics::StaticClass()
        );
        return GetPlayerStateNode;
    }
    if (NodeType.Equals(TEXT("Get Game Mode"), ESearchCase::IgnoreCase)) {
        UK2Node_CallFunction* GetGameModeNode = NewObject<UK2Node_CallFunction>(Graph);
        GetGameModeNode->FunctionReference.SetExternalMember(
            FName(TEXT("GetGameMode")),
            UGameplayStatics::StaticClass()
        );
        return GetGameModeNode;
    }
    if (NodeType.Equals(TEXT("Cast to PlayerController"), ESearchCase::IgnoreCase)) {
        UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(Graph);
        CastNode->SetPurity(false);
        CastNode->TargetType = APlayerController::StaticClass();
        return CastNode;
    }
    // Check for specific widget constructor patterns like "Create WBP_DialogueWidget Widget" or "Create WBP_DialogueWidget"
    if (NodeType.StartsWith(TEXT("Create "), ESearchCase::IgnoreCase)) {
        FString WidgetTypeName = NodeType.Mid(7); // Remove "Create " prefix
        
        // Remove " Widget" suffix if present
        if (WidgetTypeName.EndsWith(TEXT(" Widget"), ESearchCase::IgnoreCase)) {
            WidgetTypeName = WidgetTypeName.Left(WidgetTypeName.Len() - 7);
        }
        
        // Simply create a basic ConstructObjectFromClass node without trying to set the class
        // The user can set the class manually in the UI
        UK2Node_ConstructObjectFromClass* ConstructorNode = NewObject<UK2Node_ConstructObjectFromClass>(Graph);
        
        if (ConstructorNode) {
            UE_LOG(LogTemp, Display, TEXT("Created basic constructor node for: %s"), *WidgetTypeName);
            return ConstructorNode;
        }
    }
    return nullptr;
}