#include "Commands/UnrealMCPBlueprintActionCommands.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/Engine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"

// Blueprint Action Database includes
#include "K2Node.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_MapForEach.h"
#include "K2Node_SetForEach.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet/KismetMathLibrary.h"

// For pin type analysis
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"

// Additional includes for node creation
#include "Commands/UnrealMCPCommonUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

// Helper: Add Blueprint-local custom function actions
void AddBlueprintCustomFunctionActions(UBlueprint* Blueprint, const FString& SearchFilter, TArray<TSharedPtr<FJsonValue>>& OutActions)
{
    if (!Blueprint) 
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Blueprint is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Processing Blueprint '%s' with %d custom functions"), 
           *Blueprint->GetName(), Blueprint->FunctionGraphs.Num());
    
    FString SearchLower = SearchFilter.ToLower();
    int32 AddedActions = 0;
    
    for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
    {
        if (!FunctionGraph)
        {
            continue;
        }
        
        FString FunctionName = FunctionGraph->GetName();
        FString FunctionNameLower = FunctionName.ToLower();
        
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Checking function '%s'"), *FunctionName);
        
        if (!SearchFilter.IsEmpty() && !FunctionNameLower.Contains(SearchLower))
        {
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Function '%s' doesn't match search filter '%s'"), *FunctionName, *SearchFilter);
            continue;
        }
        
        // Look for function entry node to get input/output parameters
        UK2Node_FunctionEntry* FunctionEntry = nullptr;
        UK2Node_FunctionResult* FunctionResult = nullptr;
        
        for (UEdGraphNode* Node : FunctionGraph->Nodes)
        {
            if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
            {
                FunctionEntry = EntryNode;
            }
            else if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
            {
                FunctionResult = ResultNode;
            }
        }
        
        // Create function call action
        TSharedPtr<FJsonObject> FunctionObj = MakeShared<FJsonObject>();
        FunctionObj->SetStringField(TEXT("title"), FunctionName);
        FunctionObj->SetStringField(TEXT("tooltip"), FString::Printf(TEXT("Call custom function %s"), *FunctionName));
        FunctionObj->SetStringField(TEXT("category"), TEXT("Custom Functions"));
        FunctionObj->SetStringField(TEXT("keywords"), FString::Printf(TEXT("function call custom %s local blueprint"), *FunctionName));
        FunctionObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_CallFunction"));
        FunctionObj->SetStringField(TEXT("function_name"), FunctionName);
        FunctionObj->SetBoolField(TEXT("is_blueprint_function"), true);
        
        // Add parameter information if available
        if (FunctionEntry)
        {
            TArray<TSharedPtr<FJsonValue>> InputParams;
            for (UEdGraphPin* Pin : FunctionEntry->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && Pin->PinName != UEdGraphSchema_K2::PN_Then)
                {
                    TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
                    ParamObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                    ParamObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                    InputParams.Add(MakeShared<FJsonValueObject>(ParamObj));
                }
            }
            FunctionObj->SetArrayField(TEXT("input_params"), InputParams);
        }
        
        if (FunctionResult)
        {
            TArray<TSharedPtr<FJsonValue>> OutputParams;
            for (UEdGraphPin* Pin : FunctionResult->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Execute)
                {
                    TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
                    ParamObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                    ParamObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                    OutputParams.Add(MakeShared<FJsonValueObject>(ParamObj));
                }
            }
            FunctionObj->SetArrayField(TEXT("output_params"), OutputParams);
        }
        
        OutActions.Add(MakeShared<FJsonValueObject>(FunctionObj));
        AddedActions++;
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Added custom function '%s'"), *FunctionName);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Added %d custom function actions total"), AddedActions);
}

// Helper: Add Blueprint-local variable getter/setter actions
void AddBlueprintVariableActions(UBlueprint* Blueprint, const FString& SearchFilter, TArray<TSharedPtr<FJsonValue>>& OutActions)
{
    if (!Blueprint) 
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Blueprint is null"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Processing Blueprint '%s' with %d variables"), 
           *Blueprint->GetName(), Blueprint->NewVariables.Num());
    
    FString SearchLower = SearchFilter.ToLower();
    int32 AddedActions = 0;
    
    for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
    {
        FString VarName = VarDesc.VarName.ToString();
        FString VarNameLower = VarName.ToLower();
        
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Checking variable '%s'"), *VarName);
        
        if (!SearchFilter.IsEmpty() && !VarNameLower.Contains(SearchLower))
        {
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Variable '%s' doesn't match search filter '%s'"), *VarName, *SearchFilter);
            continue;
        }
        
        // Getter
        {
            TSharedPtr<FJsonObject> GetterObj = MakeShared<FJsonObject>();
            GetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Get %s"), *VarName));
            GetterObj->SetStringField(TEXT("tooltip"), FString::Printf(TEXT("Get the value of variable %s"), *VarName));
            GetterObj->SetStringField(TEXT("category"), TEXT("Variables"));
            GetterObj->SetStringField(TEXT("keywords"), FString::Printf(TEXT("variable get %s local blueprint"), *VarName));
            GetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableGet"));
            GetterObj->SetStringField(TEXT("variable_name"), VarName);
            GetterObj->SetStringField(TEXT("pin_type"), VarDesc.VarType.PinCategory.ToString());
            GetterObj->SetStringField(TEXT("function_name"), FString::Printf(TEXT("Get %s"), *VarName));
            GetterObj->SetBoolField(TEXT("is_blueprint_variable"), true);
            OutActions.Add(MakeShared<FJsonValueObject>(GetterObj));
            AddedActions++;
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Added getter for '%s'"), *VarName);
        }
        
        // Setter (if not const)
        if (!VarDesc.VarType.bIsConst)
        {
            TSharedPtr<FJsonObject> SetterObj = MakeShared<FJsonObject>();
            SetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Set %s"), *VarName));
            SetterObj->SetStringField(TEXT("tooltip"), FString::Printf(TEXT("Set the value of variable %s"), *VarName));
            SetterObj->SetStringField(TEXT("category"), TEXT("Variables"));
            SetterObj->SetStringField(TEXT("keywords"), FString::Printf(TEXT("variable set %s local blueprint"), *VarName));
            SetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableSet"));
            SetterObj->SetStringField(TEXT("variable_name"), VarName);
            SetterObj->SetStringField(TEXT("pin_type"), VarDesc.VarType.PinCategory.ToString());
            SetterObj->SetStringField(TEXT("function_name"), FString::Printf(TEXT("Set %s"), *VarName));
            SetterObj->SetBoolField(TEXT("is_blueprint_variable"), true);
            OutActions.Add(MakeShared<FJsonValueObject>(SetterObj));
            AddedActions++;
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Added setter for '%s'"), *VarName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Variable '%s' is const, skipping setter"), *VarName);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Added %d actions total"), AddedActions);
}

FString UUnrealMCPBlueprintActionCommands::GetActionsForPin(const FString& PinType, const FString& PinSubCategory, const FString& SearchFilter, int32 MaxResults)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    
    // Get the blueprint action database
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    UE_LOG(LogTemp, Warning, TEXT("GetActionsForPin: Searching for pin type '%s' with subcategory '%s'"), *PinType, *PinSubCategory);
    UE_LOG(LogTemp, Warning, TEXT("Total actions in database: %d"), ActionRegistry.Num());
    
    // Find matching actions based on pin type
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
                
                bool bRelevant = false;
                
                // Check for control flow nodes that are always relevant
                if (TemplateNode->IsA<UK2Node_IfThenElse>() ||
                    TemplateNode->IsA<UK2Node_ExecutionSequence>() ||
                    TemplateNode->IsA<UK2Node_CustomEvent>() ||
                    TemplateNode->IsA<UK2Node_DynamicCast>() ||
                    TemplateNode->IsA<UK2Node_BreakStruct>() ||
                    TemplateNode->IsA<UK2Node_MakeStruct>() ||
                    TemplateNode->IsA<UK2Node_ConstructObjectFromClass>() ||
                    TemplateNode->IsA<UK2Node_MacroInstance>() ||
                    TemplateNode->IsA<UK2Node_InputAction>() ||
                    TemplateNode->IsA<UK2Node_Self>() ||
                    TemplateNode->IsA<UK2Node_Event>() ||
                    TemplateNode->IsA<UK2Node_VariableGet>() ||
                    TemplateNode->IsA<UK2Node_VariableSet>())
                {
                    bRelevant = true;
                }
                
                // For mathematical operators, check if it's from UKismetMathLibrary
                if (!bRelevant && (PinType.Equals(TEXT("float"), ESearchCase::IgnoreCase) || 
                    PinType.Equals(TEXT("int"), ESearchCase::IgnoreCase) || 
                    PinType.Equals(TEXT("integer"), ESearchCase::IgnoreCase) ||
                    PinType.Equals(TEXT("real"), ESearchCase::IgnoreCase)))
                {
                    if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                    {
                        if (UFunction* Function = FunctionNode->GetTargetFunction())
                        {
                            UClass* OwnerClass = Function->GetOwnerClass();
                            if (OwnerClass == UKismetMathLibrary::StaticClass() ||
                                OwnerClass == UKismetSystemLibrary::StaticClass())
                            {
                                // Also check if function has float/int inputs or outputs
                                for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
                                {
                                    FProperty* Property = *PropIt;
                                    if (Property->IsA<FFloatProperty>() || Property->IsA<FIntProperty>() || Property->IsA<FDoubleProperty>())
                                    {
                                        bRelevant = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                
                // For object types, check class compatibility
                if (!bRelevant && PinType.Equals(TEXT("object"), ESearchCase::IgnoreCase) && !PinSubCategory.IsEmpty())
                {
                    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(PinSubCategory);
                    if (TargetClass)
                    {
                        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                        {
                            if (UFunction* Function = FunctionNode->GetTargetFunction())
                            {
                                if (Function->GetOwnerClass()->IsChildOf(TargetClass) || TargetClass->IsChildOf(Function->GetOwnerClass()))
                                {
                                    bRelevant = true;
                                }
                            }
                        }
                    }
                }
                
                // Default case - include more basic actions for wildcard/empty pin types
                if (!bRelevant && (PinType.Equals(TEXT("wildcard"), ESearchCase::IgnoreCase) || PinType.IsEmpty()))
                {
                    if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                    {
                        if (UFunction* Function = FunctionNode->GetTargetFunction())
                        {
                            UClass* OwnerClass = Function->GetOwnerClass();
                            if (OwnerClass == UKismetMathLibrary::StaticClass() ||
                                OwnerClass == UKismetSystemLibrary::StaticClass() ||
                                OwnerClass == UGameplayStatics::StaticClass())
                            {
                                bRelevant = true;
                            }
                        }
                    }
                    // Also include control flow nodes for wildcard searches
                    else
                    {
                        bRelevant = true; // Include all non-function nodes for wildcard
                    }
                }
                
                if (bRelevant)
                {
                    TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
                    
                    // Get what information we can from the node spawner
                    FString ActionName = TEXT("Unknown Action");
                    FString Category = TEXT("Unknown");
                    FString Tooltip = TEXT("");
                    FString Keywords = TEXT("");
                    FString NodeType = TEXT("Unknown");
                    
                    // Determine node type and get better naming
                    if (TemplateNode->IsA<UK2Node_IfThenElse>())
                    {
                        ActionName = TEXT("Branch");
                        Category = TEXT("Flow Control");
                        NodeType = TEXT("Branch");
                        Tooltip = TEXT("Conditional execution based on boolean input");
                        Keywords = TEXT("if then else conditional branch");
                        ActionObj->SetStringField(TEXT("node_class"), TEXT("UK2Node_IfThenElse"));
                    }
                    else if (TemplateNode->IsA<UK2Node_ExecutionSequence>())
                    {
                        ActionName = TEXT("Sequence");
                        Category = TEXT("Flow Control");
                        NodeType = TEXT("Sequence");
                        Tooltip = TEXT("Execute multiple outputs in order");
                        Keywords = TEXT("sequence multiple execution order");
                        ActionObj->SetStringField(TEXT("node_class"), TEXT("UK2Node_ExecutionSequence"));
                    }
                    else if (TemplateNode->IsA<UK2Node_DynamicCast>())
                    {
                        ActionName = TEXT("Cast");
                        Category = TEXT("Utilities");
                        NodeType = TEXT("Cast");
                        Tooltip = TEXT("Cast object to different type");
                        Keywords = TEXT("cast convert type object");
                        ActionObj->SetStringField(TEXT("node_class"), TEXT("UK2Node_DynamicCast"));
                    }
                    else if (TemplateNode->IsA<UK2Node_CustomEvent>())
                    {
                        ActionName = TEXT("Custom Event");
                        Category = TEXT("Events");
                        NodeType = TEXT("CustomEvent");
                        Tooltip = TEXT("Create custom event that can be called");
                        Keywords = TEXT("custom event call");
                        ActionObj->SetStringField(TEXT("node_class"), TEXT("UK2Node_CustomEvent"));
                    }
                    else if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                    {
                        ActionName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                        if (ActionName.IsEmpty())
                        {
                            ActionName = K2Node->GetClass()->GetName();
                        }
                        NodeType = K2Node->GetClass()->GetName();
                        ActionObj->SetStringField(TEXT("node_class"), NodeType);
                        
                        // Try to get function information if it's a function call
                        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                        {
                            if (UFunction* Function = FunctionNode->GetTargetFunction())
                            {
                                ActionName = Function->GetName();
                                Category = Function->GetOwnerClass()->GetName();
                                
                                // Mark math functions
                                if (Function->GetOwnerClass() == UKismetMathLibrary::StaticClass())
                                {
                                    Category = TEXT("Math");
                                    ActionObj->SetBoolField(TEXT("is_math_function"), true);
                                }
                                
                                ActionObj->SetStringField(TEXT("function_name"), Function->GetName());
                                ActionObj->SetStringField(TEXT("class_name"), Function->GetOwnerClass()->GetName());
                            }
                        }
                    }
                    else
                    {
                        ActionName = TemplateNode->GetClass()->GetName();
                        NodeType = ActionName;
                        ActionObj->SetStringField(TEXT("node_class"), NodeType);
                    }
                    
                    ActionObj->SetStringField(TEXT("title"), ActionName);
                    ActionObj->SetStringField(TEXT("tooltip"), Tooltip);
                    ActionObj->SetStringField(TEXT("category"), Category);
                    ActionObj->SetStringField(TEXT("keywords"), Keywords);
                    ActionObj->SetStringField(TEXT("node_type"), NodeType);
                    
                    // Apply search filter if provided
                    bool bPassesFilter = true;
                    if (!SearchFilter.IsEmpty())
                    {
                        FString SearchLower = SearchFilter.ToLower();
                        FString ActionNameLower = ActionName.ToLower();
                        FString CategoryLower = Category.ToLower();
                        FString TooltipLower = Tooltip.ToLower();
                        FString KeywordsLower = Keywords.ToLower();
                        
                        bPassesFilter = ActionNameLower.Contains(SearchLower) ||
                                       CategoryLower.Contains(SearchLower) ||
                                       TooltipLower.Contains(SearchLower) ||
                                       KeywordsLower.Contains(SearchLower);
                    }
                    
                    if (bPassesFilter)
                    {
                        ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
                    }
                    
                    // Limit results to avoid overwhelming output
                    if (ActionsArray.Num() >= MaxResults)
                    {
                        break;
                    }
                }
            }
        }
        
        if (ActionsArray.Num() >= MaxResults)
        {
            break;
        }
    }
    
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("pin_type"), PinType);
    ResultObj->SetStringField(TEXT("pin_subcategory"), PinSubCategory);
    ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
    ResultObj->SetNumberField(TEXT("action_count"), ActionsArray.Num());
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d actions for pin type '%s'"), ActionsArray.Num(), *PinType));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UUnrealMCPBlueprintActionCommands::GetActionsForClass(const FString& ClassName, const FString& SearchFilter, int32 MaxResults)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    
    // Find the class by name
    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(ClassName);
    if (!TargetClass)
    {
        // Try with different common class names
        FString TestClassName = ClassName;
        if (!TestClassName.StartsWith(TEXT("U")) && !TestClassName.StartsWith(TEXT("A")) && !TestClassName.StartsWith(TEXT("F")))
        {
            // Try with A prefix for Actor classes
            TestClassName = TEXT("A") + ClassName;
            TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
            
            if (!TargetClass)
            {
                // Try with U prefix for UObject classes
                TestClassName = TEXT("U") + ClassName;
                TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
            }
        }
    }
    
    if (TargetClass)
    {
        // Get the blueprint action database
        FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
        FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
        
        // Find actions relevant to this class
        for (const auto& ActionPair : ActionRegistry)
        {
            for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
            {
                if (NodeSpawner && IsValid(NodeSpawner))
                {
                    bool bRelevant = false;
                    
                    if (UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode())
                    {
                        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                        {
                            if (UFunction* Function = FunctionNode->GetTargetFunction())
                            {
                                if (Function->GetOwnerClass() == TargetClass || 
                                    Function->GetOwnerClass()->IsChildOf(TargetClass) || 
                                    TargetClass->IsChildOf(Function->GetOwnerClass()))
                                {
                                    bRelevant = true;
                                }
                            }
                        }
                    }
                    
                    if (bRelevant)
                    {
                        TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
                        
                        FString ActionName = TEXT("Unknown Action");
                        FString Category = TargetClass->GetName();
                        FString Tooltip = TEXT("");
                        FString Keywords = TEXT("");
                        
                        if (UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode())
                        {
                            if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                            {
                                ActionName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                                if (ActionName.IsEmpty())
                                {
                                    ActionName = K2Node->GetClass()->GetName();
                                }
                                
                                if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                                {
                                    if (UFunction* Function = FunctionNode->GetTargetFunction())
                                    {
                                        ActionName = Function->GetName();
                                        ActionObj->SetStringField(TEXT("function_name"), Function->GetName());
                                        ActionObj->SetStringField(TEXT("class_name"), Function->GetOwnerClass()->GetName());
                                        
                                        // Check if it's a math function
                                        if (Function->GetOwnerClass() == UKismetMathLibrary::StaticClass())
                                        {
                                            ActionObj->SetBoolField(TEXT("is_math_function"), true);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                ActionName = TemplateNode->GetClass()->GetName();
                            }
                        }
                        
                        ActionObj->SetStringField(TEXT("title"), ActionName);
                        ActionObj->SetStringField(TEXT("tooltip"), Tooltip);
                        ActionObj->SetStringField(TEXT("category"), Category);
                        ActionObj->SetStringField(TEXT("keywords"), Keywords);
                        
                        // Apply search filter if provided
                        bool bPassesFilter = true;
                        if (!SearchFilter.IsEmpty())
                        {
                            FString SearchLower = SearchFilter.ToLower();
                            FString ActionNameLower = ActionName.ToLower();
                            FString CategoryLower = Category.ToLower();
                            FString TooltipLower = Tooltip.ToLower();
                            FString KeywordsLower = Keywords.ToLower();
                            
                            bPassesFilter = ActionNameLower.Contains(SearchLower) ||
                                           CategoryLower.Contains(SearchLower) ||
                                           TooltipLower.Contains(SearchLower) ||
                                           KeywordsLower.Contains(SearchLower);
                        }
                        
                        if (bPassesFilter)
                        {
                            ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
                        }
                        
                        // Limit results
                        if (ActionsArray.Num() >= MaxResults)
                        {
                            break;
                        }
                    }
                }
            }
            
            if (ActionsArray.Num() >= MaxResults)
            {
                break;
            }
        }
        
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("class_name"), ClassName);
        ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
        ResultObj->SetNumberField(TEXT("action_count"), ActionsArray.Num());
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d actions for class '%s'"), ActionsArray.Num(), *ClassName));
    }
    else
    {
        // Class not found
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("class_name"), ClassName);
        ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
        ResultObj->SetNumberField(TEXT("action_count"), 0);
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Class '%s' not found"), *ClassName));
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UUnrealMCPBlueprintActionCommands::GetActionsForClassHierarchy(const FString& ClassName, const FString& SearchFilter, int32 MaxResults)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    TArray<TSharedPtr<FJsonValue>> HierarchyArray;
    TMap<FString, int32> CategoryCounts;
    
    // Find the class by name
    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(ClassName);
    if (!TargetClass)
    {
        // Try with different common class names
        FString TestClassName = ClassName;
        if (!TestClassName.StartsWith(TEXT("U")) && !TestClassName.StartsWith(TEXT("A")) && !TestClassName.StartsWith(TEXT("F")))
        {
            // Try with A prefix for Actor classes
            TestClassName = TEXT("A") + ClassName;
            TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
            
            if (!TargetClass)
            {
                // Try with U prefix for UObject classes
                TestClassName = TEXT("U") + ClassName;
                TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
            }
        }
    }
    
    if (TargetClass)
    {
        // Build class hierarchy
        TArray<UClass*> ClassHierarchy;
        UClass* CurrentClass = TargetClass;
        while (CurrentClass)
        {
            ClassHierarchy.Add(CurrentClass);
            HierarchyArray.Add(MakeShared<FJsonValueString>(CurrentClass->GetName()));
            CurrentClass = CurrentClass->GetSuperClass();
        }
        
        // Get the blueprint action database
        FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
        FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
        
        // Find actions relevant to this class hierarchy
        TSet<FString> UniqueActionNames; // To avoid duplicates
        for (const auto& ActionPair : ActionRegistry)
        {
            for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
            {
                if (NodeSpawner && IsValid(NodeSpawner))
                {
                    bool bRelevant = false;
                    
                    if (UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode())
                    {
                        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                        {
                            if (UFunction* Function = FunctionNode->GetTargetFunction())
                            {
                                for (UClass* HierarchyClass : ClassHierarchy)
                                {
                                    if (Function->GetOwnerClass() == HierarchyClass || 
                                        Function->GetOwnerClass()->IsChildOf(HierarchyClass) || 
                                        HierarchyClass->IsChildOf(Function->GetOwnerClass()))
                                    {
                                        bRelevant = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (bRelevant)
                    {
                        FString ActionName = TEXT("Unknown Action");
                        if (UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode())
                        {
                            if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                            {
                                ActionName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                                if (ActionName.IsEmpty())
                                {
                                    ActionName = K2Node->GetClass()->GetName();
                                }
                                
                                if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                                {
                                    if (UFunction* Function = FunctionNode->GetTargetFunction())
                                    {
                                        ActionName = Function->GetName();
                                    }
                                }
                            }
                            else
                            {
                                ActionName = TemplateNode->GetClass()->GetName();
                            }
                        }
                        
                        // Skip if we've already seen this action
                        if (UniqueActionNames.Contains(ActionName))
                        {
                            continue;
                        }
                        UniqueActionNames.Add(ActionName);
                        
                        TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
                        
                        FString CategoryName = TargetClass->GetName();
                        CategoryCounts.FindOrAdd(CategoryName)++;
                        
                        ActionObj->SetStringField(TEXT("title"), ActionName);
                        ActionObj->SetStringField(TEXT("tooltip"), TEXT(""));
                        ActionObj->SetStringField(TEXT("category"), CategoryName);
                        ActionObj->SetStringField(TEXT("keywords"), TEXT(""));
                        
                        if (UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode())
                        {
                            if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                            {
                                if (UFunction* Function = FunctionNode->GetTargetFunction())
                                {
                                    ActionObj->SetStringField(TEXT("function_name"), Function->GetName());
                                    ActionObj->SetStringField(TEXT("class_name"), Function->GetOwnerClass()->GetName());
                                    
                                    // Check if it's a math function
                                    if (Function->GetOwnerClass() == UKismetMathLibrary::StaticClass())
                                    {
                                        ActionObj->SetBoolField(TEXT("is_math_function"), true);
                                    }
                                }
                            }
                        }
                        
                        // Apply search filter if provided
                        bool bPassesFilter = true;
                        if (!SearchFilter.IsEmpty())
                        {
                            FString SearchLower = SearchFilter.ToLower();
                            FString ActionNameLower = ActionName.ToLower();
                            FString CategoryLower = CategoryName.ToLower();
                            
                            bPassesFilter = ActionNameLower.Contains(SearchLower) ||
                                           CategoryLower.Contains(SearchLower);
                        }
                        
                        if (bPassesFilter)
                        {
                            ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
                        }
                        
                        // Limit results
                        if (ActionsArray.Num() >= MaxResults)
                        {
                            break;
                        }
                    }
                }
            }
            
            if (ActionsArray.Num() >= MaxResults)
            {
                break;
            }
        }
        
        // Build category counts object
        TSharedPtr<FJsonObject> CategoryCountsObj = MakeShared<FJsonObject>();
        for (const auto& CountPair : CategoryCounts)
        {
            CategoryCountsObj->SetNumberField(CountPair.Key, CountPair.Value);
        }
        
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("class_name"), ClassName);
        ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
        ResultObj->SetArrayField(TEXT("class_hierarchy"), HierarchyArray);
        ResultObj->SetObjectField(TEXT("category_counts"), CategoryCountsObj);
        ResultObj->SetNumberField(TEXT("action_count"), ActionsArray.Num());
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d actions for class hierarchy of '%s'"), ActionsArray.Num(), *ClassName));
    }
    else
    {
        // Class not found
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("class_name"), ClassName);
        ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
        ResultObj->SetArrayField(TEXT("class_hierarchy"), HierarchyArray);
        ResultObj->SetObjectField(TEXT("category_counts"), MakeShared<FJsonObject>());
        ResultObj->SetNumberField(TEXT("action_count"), 0);
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Class '%s' not found"), *ClassName));
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UUnrealMCPBlueprintActionCommands::SearchBlueprintActions(const FString& SearchQuery, const FString& Category, int32 MaxResults, const FString& BlueprintName)
{
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions called with: SearchQuery='%s', Category='%s', MaxResults=%d, BlueprintName='%s'"), 
           *SearchQuery, *Category, MaxResults, *BlueprintName);
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    
    if (SearchQuery.IsEmpty())
    {
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("message"), TEXT("Search query cannot be empty"));
        ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
        ResultObj->SetNumberField(TEXT("action_count"), 0);
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
    }
    
    // Blueprint-local variable actions
    if (!BlueprintName.IsEmpty())
    {
        UBlueprint* Blueprint = nullptr;
        FString AssetPath = BlueprintName;
        
        // Try different path patterns to find the Blueprint
        TArray<FString> PathsToTry;
        
        if (AssetPath.StartsWith(TEXT("/Game/")))
        {
            // Already a full path, use as-is
            PathsToTry.Add(AssetPath);
        }
        else
        {
            // Try common Blueprint locations
            PathsToTry.Add(FString::Printf(TEXT("/Game/Blueprints/%s.%s"), *BlueprintName, *BlueprintName));
            PathsToTry.Add(FString::Printf(TEXT("/Game/%s.%s"), *BlueprintName, *BlueprintName));
            PathsToTry.Add(FString::Printf(TEXT("/Game/ThirdPerson/Blueprints/%s.%s"), *BlueprintName, *BlueprintName));
            
            // Also try without the duplicate name pattern
            PathsToTry.Add(FString::Printf(TEXT("/Game/Blueprints/%s"), *BlueprintName));
            PathsToTry.Add(FString::Printf(TEXT("/Game/%s"), *BlueprintName));
        }
        
        // Try loading from each path
        for (const FString& PathToTry : PathsToTry)
        {
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Trying to load Blueprint from path: %s"), *PathToTry);
            Blueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *PathToTry));
            if (Blueprint)
            {
                UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Successfully loaded Blueprint from: %s"), *PathToTry);
                break;
            }
        }
        
        if (Blueprint)
        {
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Adding variable actions for Blueprint: %s"), *Blueprint->GetName());
            AddBlueprintVariableActions(Blueprint, SearchQuery, ActionsArray);
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Added %d variable actions"), ActionsArray.Num());
            
            // Add custom function actions
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Adding custom function actions for Blueprint: %s"), *Blueprint->GetName());
            AddBlueprintCustomFunctionActions(Blueprint, SearchQuery, ActionsArray);
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Total actions after custom functions: %d"), ActionsArray.Num());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Failed to load Blueprint: %s. Tried paths:"), *BlueprintName);
            for (const FString& PathToTry : PathsToTry)
            {
                UE_LOG(LogTemp, Warning, TEXT("  - %s"), *PathToTry);
            }
        }
    }
    
    // Get the blueprint action database
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    FString SearchLower = SearchQuery.ToLower();
    FString CategoryLower = Category.ToLower();
    
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Searching for '%s' in category '%s'"), *SearchQuery, *Category);
    UE_LOG(LogTemp, Warning, TEXT("Total actions in database: %d"), ActionRegistry.Num());
    
    // Search through all actions
    for (const auto& ActionPair : ActionRegistry)
    {
        for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
        {
            if (NodeSpawner && IsValid(NodeSpawner))
            {
                UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();
                if (!TemplateNode)
                {
                    continue;
                }
                
                FString ActionName = TEXT("Unknown Action");
                FString ActionCategory = TEXT("Unknown");
                FString Tooltip = TEXT("");
                FString Keywords = TEXT("");
                FString NodeType = TEXT("Unknown");
                
                // Determine node type and get information
                if (TemplateNode->IsA<UK2Node_IfThenElse>())
                {
                    ActionName = TEXT("Branch");
                    ActionCategory = TEXT("Flow Control");
                    NodeType = TEXT("Branch");
                    Tooltip = TEXT("Conditional execution based on boolean input");
                    Keywords = TEXT("if then else conditional branch bool boolean");
                }
                else if (TemplateNode->IsA<UK2Node_ExecutionSequence>())
                {
                    ActionName = TEXT("Sequence");
                    ActionCategory = TEXT("Flow Control");
                    NodeType = TEXT("Sequence");
                    Tooltip = TEXT("Execute multiple outputs in order");
                    Keywords = TEXT("sequence multiple execution order flow");
                }
                else if (TemplateNode->IsA<UK2Node_DynamicCast>())
                {
                    ActionName = TEXT("Cast");
                    ActionCategory = TEXT("Utilities");
                    NodeType = TEXT("Cast");
                    Tooltip = TEXT("Cast object to different type");
                    Keywords = TEXT("cast convert type object class");
                }
                else if (TemplateNode->IsA<UK2Node_CustomEvent>())
                {
                    ActionName = TEXT("Custom Event");
                    ActionCategory = TEXT("Events");
                    NodeType = TEXT("CustomEvent");
                    Tooltip = TEXT("Create custom event that can be called");
                    Keywords = TEXT("custom event call dispatch");
                }
                else if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                {
                    if (UFunction* Function = FunctionNode->GetTargetFunction())
                    {
                        ActionName = Function->GetName();
                        UClass* OwnerClass = Function->GetOwnerClass();
                        ActionCategory = OwnerClass->GetName();
                        
                        // Better categorization
                        if (OwnerClass == UKismetMathLibrary::StaticClass())
                        {
                            ActionCategory = TEXT("Math");
                            Keywords = TEXT("math mathematics calculation");
                        }
                        else if (OwnerClass == UKismetSystemLibrary::StaticClass())
                        {
                            ActionCategory = TEXT("Utilities");
                            Keywords = TEXT("system utility helper");
                        }
                        else if (OwnerClass == UGameplayStatics::StaticClass())
                        {
                            ActionCategory = TEXT("Game");
                            Keywords = TEXT("gameplay game static");
                        }
                        
                        NodeType = TEXT("Function");
                    }
                }
                else if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                {
                    ActionName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                    if (ActionName.IsEmpty())
                    {
                        ActionName = K2Node->GetClass()->GetName();
                    }
                    NodeType = K2Node->GetClass()->GetName();
                    ActionCategory = TEXT("Node");
                }
                
                // Apply search and category filters
                FString ActionNameLower = ActionName.ToLower();
                FString ActionCategoryLower = ActionCategory.ToLower();
                FString TooltipLower = Tooltip.ToLower();
                FString KeywordsLower = Keywords.ToLower();
                
                bool bMatchesSearch = ActionNameLower.Contains(SearchLower) ||
                                     ActionCategoryLower.Contains(SearchLower) ||
                                     TooltipLower.Contains(SearchLower) ||
                                     KeywordsLower.Contains(SearchLower);
                
                bool bMatchesCategory = Category.IsEmpty() || ActionCategoryLower.Contains(CategoryLower);
                
                if (bMatchesSearch && bMatchesCategory)
                {
                    TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
                    
                    ActionObj->SetStringField(TEXT("title"), ActionName);
                    ActionObj->SetStringField(TEXT("tooltip"), Tooltip);
                    ActionObj->SetStringField(TEXT("category"), ActionCategory);
                    ActionObj->SetStringField(TEXT("keywords"), Keywords);
                    ActionObj->SetStringField(TEXT("node_type"), NodeType);
                    
                    if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                    {
                        if (UFunction* Function = FunctionNode->GetTargetFunction())
                        {
                            ActionObj->SetStringField(TEXT("function_name"), Function->GetName());
                            ActionObj->SetStringField(TEXT("class_name"), Function->GetOwnerClass()->GetName());
                            
                            if (Function->GetOwnerClass() == UKismetMathLibrary::StaticClass())
                            {
                                ActionObj->SetBoolField(TEXT("is_math_function"), true);
                            }
                        }
                    }
                    
                    ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
                    
                    // Limit results
                    if (ActionsArray.Num() >= MaxResults)
                    {
                        break;
                    }
                }
            }
        }
        
        if (ActionsArray.Num() >= MaxResults)
        {
            break;
        }
    }
    
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("search_query"), SearchQuery);
    ResultObj->SetStringField(TEXT("category_filter"), Category);
    ResultObj->SetArrayField(TEXT("actions"), ActionsArray);
    ResultObj->SetNumberField(TEXT("action_count"), ActionsArray.Num());
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d actions matching '%s'"), ActionsArray.Num(), *SearchQuery));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UUnrealMCPBlueprintActionCommands::GetNodePinInfo(const FString& NodeName, const FString& PinName)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    // Define known node pin information
    TMap<FString, TMap<FString, TSharedPtr<FJsonObject>>> NodePinDatabase;
    
    // Create Widget node
    TMap<FString, TSharedPtr<FJsonObject>> CreateWidgetPins;
    
    auto ClassPin = MakeShared<FJsonObject>();
    ClassPin->SetStringField(TEXT("pin_type"), TEXT("class"));
    ClassPin->SetStringField(TEXT("expected_type"), TEXT("Class<UserWidget>"));
    ClassPin->SetStringField(TEXT("description"), TEXT("The widget class to instantiate"));
    ClassPin->SetBoolField(TEXT("is_required"), true);
    ClassPin->SetBoolField(TEXT("is_input"), true);
    CreateWidgetPins.Add(TEXT("Class"), ClassPin);
    
    auto OwningPlayerPin = MakeShared<FJsonObject>();
    OwningPlayerPin->SetStringField(TEXT("pin_type"), TEXT("object"));
    OwningPlayerPin->SetStringField(TEXT("expected_type"), TEXT("PlayerController"));
    OwningPlayerPin->SetStringField(TEXT("description"), TEXT("The player controller that owns this widget"));
    OwningPlayerPin->SetBoolField(TEXT("is_required"), false);
    OwningPlayerPin->SetBoolField(TEXT("is_input"), true);
    CreateWidgetPins.Add(TEXT("Owning Player"), OwningPlayerPin);
    CreateWidgetPins.Add(TEXT("OwningPlayer"), OwningPlayerPin); // Alternative name
    
    auto ReturnValuePin = MakeShared<FJsonObject>();
    ReturnValuePin->SetStringField(TEXT("pin_type"), TEXT("object"));
    ReturnValuePin->SetStringField(TEXT("expected_type"), TEXT("UserWidget"));
    ReturnValuePin->SetStringField(TEXT("description"), TEXT("The created widget instance"));
    ReturnValuePin->SetBoolField(TEXT("is_required"), false);
    ReturnValuePin->SetBoolField(TEXT("is_input"), false);
    CreateWidgetPins.Add(TEXT("Return Value"), ReturnValuePin);
    
    NodePinDatabase.Add(TEXT("Create Widget"), CreateWidgetPins);
    NodePinDatabase.Add(TEXT("CreateWidget"), CreateWidgetPins);
    
    // Get Controller node
    TMap<FString, TSharedPtr<FJsonObject>> GetControllerPins;
    
    auto TargetPin = MakeShared<FJsonObject>();
    TargetPin->SetStringField(TEXT("pin_type"), TEXT("object"));
    TargetPin->SetStringField(TEXT("expected_type"), TEXT("Pawn"));
    TargetPin->SetStringField(TEXT("description"), TEXT("The pawn to get the controller from"));
    TargetPin->SetBoolField(TEXT("is_required"), true);
    TargetPin->SetBoolField(TEXT("is_input"), true);
    GetControllerPins.Add(TEXT("Target"), TargetPin);
    
    auto ControllerReturnPin = MakeShared<FJsonObject>();
    ControllerReturnPin->SetStringField(TEXT("pin_type"), TEXT("object"));
    ControllerReturnPin->SetStringField(TEXT("expected_type"), TEXT("Controller"));
    ControllerReturnPin->SetStringField(TEXT("description"), TEXT("The controller possessing this pawn"));
    ControllerReturnPin->SetBoolField(TEXT("is_required"), false);
    ControllerReturnPin->SetBoolField(TEXT("is_input"), false);
    GetControllerPins.Add(TEXT("Return Value"), ControllerReturnPin);
    
    NodePinDatabase.Add(TEXT("Get Controller"), GetControllerPins);
    NodePinDatabase.Add(TEXT("GetController"), GetControllerPins);
    
    // Cast To nodes
    TMap<FString, TSharedPtr<FJsonObject>> CastToPins;
    
    auto ObjectPin = MakeShared<FJsonObject>();
    ObjectPin->SetStringField(TEXT("pin_type"), TEXT("object"));
    ObjectPin->SetStringField(TEXT("expected_type"), TEXT("Object"));
    ObjectPin->SetStringField(TEXT("description"), TEXT("The object to cast"));
    ObjectPin->SetBoolField(TEXT("is_required"), true);
    ObjectPin->SetBoolField(TEXT("is_input"), true);
    CastToPins.Add(TEXT("Object"), ObjectPin);
    
    auto CastSuccessPin = MakeShared<FJsonObject>();
    CastSuccessPin->SetStringField(TEXT("pin_type"), TEXT("exec"));
    CastSuccessPin->SetStringField(TEXT("expected_type"), TEXT("exec"));
    CastSuccessPin->SetStringField(TEXT("description"), TEXT("Execution path if cast succeeds"));
    CastSuccessPin->SetBoolField(TEXT("is_required"), false);
    CastSuccessPin->SetBoolField(TEXT("is_input"), false);
    CastToPins.Add(TEXT("Cast Success"), CastSuccessPin);
    
    auto CastFailPin = MakeShared<FJsonObject>();
    CastFailPin->SetStringField(TEXT("pin_type"), TEXT("exec"));
    CastFailPin->SetStringField(TEXT("expected_type"), TEXT("exec"));
    CastFailPin->SetStringField(TEXT("description"), TEXT("Execution path if cast fails"));
    CastFailPin->SetBoolField(TEXT("is_required"), false);
    CastFailPin->SetBoolField(TEXT("is_input"), false);
    CastToPins.Add(TEXT("Cast Failed"), CastFailPin);
    
    NodePinDatabase.Add(TEXT("Cast to PlayerController"), CastToPins);
    NodePinDatabase.Add(TEXT("Cast to"), CastToPins);
    
    // Look up the requested node and pin
    FString NormalizedNodeName = NodeName;
    NormalizedNodeName = NormalizedNodeName.Replace(TEXT(" "), TEXT(""));
    
    TSharedPtr<FJsonObject> PinInfo = nullptr;
    if (NodePinDatabase.Contains(NodeName))
    {
        auto& PinMap = NodePinDatabase[NodeName];
        if (PinMap.Contains(PinName))
        {
            PinInfo = PinMap[PinName];
        }
    }
    else if (NodePinDatabase.Contains(NormalizedNodeName))
    {
        auto& PinMap = NodePinDatabase[NormalizedNodeName];
        if (PinMap.Contains(PinName))
        {
            PinInfo = PinMap[PinName];
        }
    }
    
    if (PinInfo.IsValid())
    {
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("node_name"), NodeName);
        ResultObj->SetStringField(TEXT("pin_name"), PinName);
        ResultObj->SetObjectField(TEXT("pin_info"), PinInfo);
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Found pin information for '%s' on node '%s'"), *PinName, *NodeName));
    }
    else
    {
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("node_name"), NodeName);
        ResultObj->SetStringField(TEXT("pin_name"), PinName);
        ResultObj->SetObjectField(TEXT("pin_info"), MakeShared<FJsonObject>());
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("No pin information found for '%s' on node '%s'"), *PinName, *NodeName));
        
        // Provide available pins for this node if we know the node
        if (NodePinDatabase.Contains(NodeName) || NodePinDatabase.Contains(NormalizedNodeName))
        {
            TArray<TSharedPtr<FJsonValue>> AvailablePins;
            const auto& PinMap = NodePinDatabase.Contains(NodeName) ? NodePinDatabase[NodeName] : NodePinDatabase[NormalizedNodeName];
            for (const auto& Pin : PinMap)
            {
                AvailablePins.Add(MakeShared<FJsonValueString>(Pin.Key));
            }
            ResultObj->SetArrayField(TEXT("available_pins"), AvailablePins);
        }
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    return OutputString;
}

FString UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    // Parse JSON parameters if provided
    TSharedPtr<FJsonObject> ParamsObject;
    if (!JsonParams.IsEmpty())
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonParams);
        if (!FJsonSerializer::Deserialize(Reader, ParamsObject) || !ParamsObject.IsValid())
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("message"), TEXT("Invalid JSON parameters"));
            
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
            return OutputString;
        }
    }
    
    // Find the blueprint
    UBlueprint* Blueprint = nullptr;
    
    // Find blueprint by searching for it in the asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> BlueprintAssets;
    AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
    
    for (const FAssetData& AssetData : BlueprintAssets)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName.Contains(BlueprintName) || BlueprintName.Contains(AssetName))
        {
            Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
            if (Blueprint)
                break;
        }
    }
    
    if (!Blueprint)
    {
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Blueprint '%s' not found"), *BlueprintName));
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
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
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("message"), TEXT("Could not find EventGraph in blueprint"));
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
    }
    
    // Parse node position first
    int32 PositionX = 0;
    int32 PositionY = 0;
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
                PositionX = FMath::RoundToInt((*JsonArray)[0]->AsNumber());
                PositionY = FMath::RoundToInt((*JsonArray)[1]->AsNumber());
            }
        }
        else
        {
            // Fallback: parse as string format "[x, y]" or "x,y"
            FString CleanPosition = NodePosition;
            CleanPosition = CleanPosition.Replace(TEXT("["), TEXT(""));
            CleanPosition = CleanPosition.Replace(TEXT("]"), TEXT(""));
            
            TArray<FString> Coords;
            CleanPosition.ParseIntoArray(Coords, TEXT(","));
            
            if (Coords.Num() == 2)
            {
                PositionX = FCString::Atoi(*Coords[0].TrimStartAndEnd());
                PositionY = FCString::Atoi(*Coords[1].TrimStartAndEnd());
            }
        }
    }
    
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
    // Loop node creation (K2Node_MacroInstance types)
    else if (FunctionName.Equals(TEXT("For Each Loop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("For Each Loop with Break"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Reverse for Each Loop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("For Loop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("For Loop with Break"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ForLoop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ForEachLoop"), ESearchCase::IgnoreCase))
    {
        // For now, return a helpful error message explaining this is a known limitation
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("message"), FString::Printf(
            TEXT("Loop nodes like '%s' are discovered correctly via search_blueprint_actions, but creating them programmatically requires special handling of engine macro instances. This is a known limitation. The node type is 'K2Node_MacroInstance' and it exists in the action database, but the C++ implementation needs enhancement to properly instantiate engine-internal macro graphs."), 
            *FunctionName));
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
    }

    // Variable getter/setter node creation - check this FIRST before function lookup
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
        // Try to find the variable in the Blueprint
        bool bFound = false;
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
        if (!bFound)
        {
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Variable '%s' not found in Blueprint '%s'"), *VarName, *BlueprintName));
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
            return OutputString;
        }
    }
    // Loop node creation (K2Node_MacroInstance types)
    else if (FunctionName.Equals(TEXT("For Each Loop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("For Each Loop with Break"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("Reverse for Each Loop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("For Loop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("For Loop with Break"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ForLoop"), ESearchCase::IgnoreCase) ||
             FunctionName.Equals(TEXT("ForEachLoop"), ESearchCase::IgnoreCase))
    {
        // For now, return a message indicating loop macro creation is not fully implemented
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("message"), TEXT("Loop macro nodes require special handling that is not yet fully implemented. The macro blueprints exist but finding and instantiating them programmatically requires additional Blueprint macro instance logic."));
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
    }
    // Special loop node types that are not macros  
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
    else
    {
        // Try to find the function and create a function call node
        UFunction* TargetFunction = nullptr;
        TargetClass = nullptr;
        
        // If a class name is provided, try to find it
        if (!ClassName.IsEmpty())
        {
            TargetClass = UClass::TryFindTypeSlow<UClass>(ClassName);
            if (!TargetClass)
            {
                // Try with common prefixes
                FString TestClassName = ClassName;
                if (!TestClassName.StartsWith(TEXT("U")) && !TestClassName.StartsWith(TEXT("A")) && !TestClassName.StartsWith(TEXT("/Script/")))
                {
                    TestClassName = TEXT("U") + ClassName;
                    TargetClass = UClass::TryFindTypeSlow<UClass>(TestClassName);
                }
                
                // Try with full path for common Unreal classes
                if (!TargetClass)
                {
                    if (ClassName.Equals(TEXT("KismetMathLibrary"), ESearchCase::IgnoreCase))
                    {
                        TargetClass = UKismetMathLibrary::StaticClass();
                    }
                    else if (ClassName.Equals(TEXT("KismetSystemLibrary"), ESearchCase::IgnoreCase))
                    {
                        TargetClass = UKismetSystemLibrary::StaticClass();
                    }
                    else if (ClassName.Equals(TEXT("GameplayStatics"), ESearchCase::IgnoreCase))
                    {
                        TargetClass = UGameplayStatics::StaticClass();
                    }
                }
            }
            
            if (TargetClass)
            {
                TargetFunction = TargetClass->FindFunctionByName(*FunctionName);
            }
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
            ResultObj->SetBoolField(TEXT("success"), false);
            ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Function '%s' not found and not a recognized control flow node"), *FunctionName));
            
            UE_LOG(LogTemp, Warning, TEXT("CreateNodeByActionName: Function '%s' not found"), *FunctionName);
            
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
            return OutputString;
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
        ResultObj->SetBoolField(TEXT("success"), false);
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to create node for '%s'"), *FunctionName));
        
        UE_LOG(LogTemp, Error, TEXT("CreateNodeByActionName: Failed to create node for '%s'"), *FunctionName);
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
        return OutputString;
    }
    
    UE_LOG(LogTemp, Log, TEXT("CreateNodeByActionName: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);

    
    // Mark blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // Create result with node information
    ResultObj->SetBoolField(TEXT("success"), true);
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
    
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created '%s' node (%s)"), 
                                                               *NodeTitle, *NodeType));
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    return OutputString;
}

