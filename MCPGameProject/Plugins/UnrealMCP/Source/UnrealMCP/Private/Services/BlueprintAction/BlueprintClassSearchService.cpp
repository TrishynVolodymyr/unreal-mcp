// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/BlueprintAction/BlueprintClassSearchService.h"
#include "Services/NodeCreation/NodeCreationHelpers.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node.h"
#include "Kismet/KismetMathLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FBlueprintClassSearchService::FBlueprintClassSearchService()
{
}

FBlueprintClassSearchService::~FBlueprintClassSearchService()
{
}

FString FBlueprintClassSearchService::GetActionsForClass(
    const FString& ClassName,
    const FString& SearchFilter,
    int32 MaxResults
)
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
        
        // --- BEGIN: Add native property getter/setter nodes ---
        int32 PropertyActionsAdded = 0;
        for (TFieldIterator<FProperty> PropIt(TargetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
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
            FString PropNameLower = PropName.ToLower();
            FString PinTypeLower = PinType.ToLower();
            FString KeywordsLower = Keywords.ToLower();
            FString SearchFilterLower = SearchFilter.ToLower();
            // Apply search filter
            if (!SearchFilter.IsEmpty() && !(PropNameLower.Contains(SearchFilterLower) || PinTypeLower.Contains(SearchFilterLower) || KeywordsLower.Contains(SearchFilterLower)))
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
                PropertyActionsAdded++;
                if (ActionsArray.Num() >= MaxResults) { break; }
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
                PropertyActionsAdded++;
                if (ActionsArray.Num() >= MaxResults) { break; }
            }
            if (ActionsArray.Num() >= MaxResults) { break; }
        }
        // --- END: Add native property getter/setter nodes ---
        
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

FString FBlueprintClassSearchService::GetActionsForClassHierarchy(
    const FString& ClassName,
    const FString& SearchFilter,
    int32 MaxResults
)
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
        
        // --- BEGIN: Add native property getter/setter nodes for all classes in hierarchy ---
        TSet<FString> SeenPropertyNames;
        int32 PropertyActionsAdded = 0;
        for (UClass* HierarchyClass : ClassHierarchy)
        {
            for (TFieldIterator<FProperty> PropIt(HierarchyClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
            {
                FProperty* Property = *PropIt;
                if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
                {
                    continue;
                }
                FString PropName = Property->GetName();
                if (SeenPropertyNames.Contains(PropName))
                {
                    continue; // Avoid duplicates
                }
                SeenPropertyNames.Add(PropName);
                FString PinType = Property->GetCPPType();
                FString Category = FString::Printf(TEXT("Native Property (%s)"), *HierarchyClass->GetName());
                FString Keywords = FString::Printf(TEXT("property variable %s %s native %s"), *PropName, *PinType, *HierarchyClass->GetName());
                FString Tooltip = FString::Printf(TEXT("Access the %s property on %s"), *PropName, *HierarchyClass->GetName());
                FString PropNameLower = PropName.ToLower();
                FString PinTypeLower = PinType.ToLower();
                FString KeywordsLower = Keywords.ToLower();
                FString SearchFilterLower = SearchFilter.ToLower();
                // Apply search filter
                if (!SearchFilter.IsEmpty() && !(PropNameLower.Contains(SearchFilterLower) || PinTypeLower.Contains(SearchFilterLower) || KeywordsLower.Contains(SearchFilterLower)))
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
                    PropertyActionsAdded++;
                    if (ActionsArray.Num() >= MaxResults) { break; }
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
                    PropertyActionsAdded++;
                    if (ActionsArray.Num() >= MaxResults) { break; }
                }
                if (ActionsArray.Num() >= MaxResults) { break; }
            }
            if (ActionsArray.Num() >= MaxResults) { break; }
        }
        // --- END: Add native property getter/setter nodes for all classes in hierarchy ---
        
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

UClass* FBlueprintClassSearchService::ResolveClass(const FString& ClassName)
{
    // TODO: Move class resolution logic (appears in both functions)
    // Lines ~699-723 and ~920-945
    // 
    // Strategy:
    // 1. Try direct TryFindTypeSlow
    // 2. Try with /Script/Engine prefix
    // 3. Try with /Game/Blueprints prefix
    
    return nullptr;
}

TArray<FString> FBlueprintClassSearchService::BuildClassHierarchy(UClass* TargetClass)
{
    // TODO: Move hierarchy building (lines ~947-960)
    // Walks up parent class chain using GetSuperClass()
    
    TArray<FString> Hierarchy;
    return Hierarchy;
}

TMap<FString, int32> FBlueprintClassSearchService::CountActionsByCategory(
    const TArray<TSharedPtr<FJsonValue>>& Actions
)
{
    // TODO: Move category counting logic (lines ~1122-1140)
    // Groups actions by category and counts them
    
    TMap<FString, int32> CategoryCounts;
    return CategoryCounts;
}
