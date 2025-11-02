// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/BlueprintAction/BlueprintPinSearchService.h"
#include "Services/NodeCreation/NodeCreationHelpers.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FBlueprintPinSearchService::FBlueprintPinSearchService()
{
}

FBlueprintPinSearchService::~FBlueprintPinSearchService()
{
}

FString FBlueprintPinSearchService::GetActionsForPin(
    const FString& PinType,
    const FString& PinSubCategory,
    const FString& SearchFilter,
    int32 MaxResults
)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    
    // Get the blueprint action database
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    // Before using pin_subcategory or class name for type resolution, convert short names to full path names
    FString ResolvedPinSubcategory = PinSubCategory;
    if (!PinSubCategory.IsEmpty() && !PinSubCategory.StartsWith("/"))
    {
        // Try to resolve common engine types
        if (PinSubCategory == "PlayerController")
        {
            ResolvedPinSubcategory = "/Script/Engine.PlayerController";
        }
        // Add more mappings as needed for other common types
    }
    
    UE_LOG(LogTemp, Warning, TEXT("GetActionsForPin: Searching for pin type '%s' with subcategory '%s'"), *PinType, *ResolvedPinSubcategory);
    UE_LOG(LogTemp, Warning, TEXT("Total actions in database: %d"), ActionRegistry.Num());
    
    // OPTIMIZATION: Resolve target class ONCE before the loop, not inside it
    UClass* TargetClass = nullptr;
    if (PinType.Equals(TEXT("object"), ESearchCase::IgnoreCase) && !ResolvedPinSubcategory.IsEmpty())
    {
        // Try to find the class - this is slow, so only do it once
        TargetClass = UClass::TryFindTypeSlow<UClass>(ResolvedPinSubcategory);
        if (!TargetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("GetActionsForPin: Could not find class for '%s'"), *ResolvedPinSubcategory);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GetActionsForPin: Found target class: %s"), *TargetClass->GetName());
        }
    }
    
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
                if (!bRelevant && PinType.Equals(TEXT("object"), ESearchCase::IgnoreCase) && TargetClass)
                {
                    // TargetClass already resolved outside the loop for performance
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
    
    // --- BEGIN: Add native property getter/setter nodes for pin context ---
    // TargetClass already resolved at the beginning of the function for performance
    if (TargetClass)
    {
        for (TFieldIterator<FProperty> PropIt(TargetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            if (ActionsArray.Num() >= MaxResults) break;
            FProperty* Property = *PropIt;
            if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
            {
                continue;
            }
            FString PropName = Property->GetName();
            FString PinType = Property->GetCPPType();
            FString Category = TEXT("Native Property");
            FString Keywords = FString::Printf(TEXT("property variable %s %s native"), *PropName, *PinType);
            FString Tooltip = FString::Printf(TEXT("Access the %s property on %s"), *PropName, *TargetClass->GetName());
            // Apply search filter
            if (!SearchFilter.IsEmpty() && !(PropName.ToLower().Contains(SearchFilter.ToLower()) || PinType.ToLower().Contains(SearchFilter.ToLower()) || Keywords.ToLower().Contains(SearchFilter.ToLower())))
            {
                continue;
            }
            // Getter node
            {
                TSharedPtr<FJsonObject> GetterObj = MakeShared<FJsonObject>();
                FString DisplayName = NodeCreationHelpers::ConvertPropertyNameToDisplay(PropName);
                GetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Get %s"), *DisplayName));
                GetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                GetterObj->SetStringField(TEXT("category"), Category);
                GetterObj->SetStringField(TEXT("variable_name"), PropName);
                GetterObj->SetStringField(TEXT("pin_type"), PinType);
                GetterObj->SetStringField(TEXT("function_name"), FString::Printf(TEXT("Get %s"), *DisplayName));
                GetterObj->SetBoolField(TEXT("is_native_property"), true);
                ActionsArray.Add(MakeShared<FJsonValueObject>(GetterObj));
                if (ActionsArray.Num() >= MaxResults) break;
            }
            // Setter node (if BlueprintReadWrite and not const)
            if (Property->HasMetaData(TEXT("BlueprintReadWrite")) && !Property->HasMetaData(TEXT("BlueprintReadOnly")) && !Property->HasAnyPropertyFlags(CPF_ConstParm))
            {
                TSharedPtr<FJsonObject> SetterObj = MakeShared<FJsonObject>();
                FString DisplayName = NodeCreationHelpers::ConvertPropertyNameToDisplay(PropName);
                SetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Set %s"), *DisplayName));
                SetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                SetterObj->SetStringField(TEXT("category"), Category);
                SetterObj->SetStringField(TEXT("variable_name"), PropName);
                SetterObj->SetStringField(TEXT("pin_type"), PinType);
                SetterObj->SetStringField(TEXT("function_name"), FString::Printf(TEXT("Set %s"), *DisplayName));
                SetterObj->SetBoolField(TEXT("is_native_property"), true);
                ActionsArray.Add(MakeShared<FJsonValueObject>(SetterObj));
                if (ActionsArray.Num() >= MaxResults) break;
            }
        }
    }
    // --- END: Add native property getter/setter nodes for pin context ---
    
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
    
    return FString();
}

FString FBlueprintPinSearchService::ResolveShortClassName(const FString& ShortName)
{
    // TODO: Move short name resolution logic (lines ~367-373)
    // Maps short names like "PlayerController" to "/Script/Engine.PlayerController"
    return FString();
}

bool FBlueprintPinSearchService::IsRelevantForMathOperations(UEdGraphNode* TemplateNode, const FString& PinType)
{
    // TODO: Move math operation relevance check (lines ~455-491)
    // Checks if node is from KismetMathLibrary and has float/int properties
    return false;
}

bool FBlueprintPinSearchService::IsRelevantForObjectType(UEdGraphNode* TemplateNode, UClass* TargetClass)
{
    // TODO: Move object type relevance check (lines ~494-507)
    // Checks class compatibility using IsChildOf
    return false;
}

void FBlueprintPinSearchService::AddNativePropertyNodes(
    UClass* TargetClass,
    TArray<TSharedPtr<FJsonValue>>& OutActions,
    int32 MaxResults
)
{
    // TODO: Move native property nodes addition (lines ~617-690)
    // Creates getter/setter nodes for BlueprintVisible properties
}
