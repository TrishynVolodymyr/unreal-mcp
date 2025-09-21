#include "Commands/BlueprintAction/UnrealMCPBlueprintActionCommands.h"
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
#include "Utils/UnrealMCPCommonUtils.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"

// Native Blueprint Action Menu includes
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionMenuItem.h"
#include "EdGraphSchema_K2_Actions.h"

// Blueprint Action Database includes
#include "K2Node.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintTypePromotion.h"
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

// Phase 2 includes removed - using universal dynamic creation via Blueprint Action Database
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet/KismetMathLibrary.h"

// For pin type analysis
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"

// Additional includes for node creation
#include "Utils/UnrealMCPCommonUtils.h"
#include "Commands/BlueprintNode/UnrealMCPNodeCreators.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

static FString ConvertPropertyNameToDisplay(const FString& InPropName)
{
    FString Name = InPropName;

    // Strip common bool prefix 'b' when followed by an uppercase letter (e.g., bShowMouseCursor -> ShowMouseCursor)
    if (Name.StartsWith(TEXT("b")) && Name.Len() > 1 && FChar::IsUpper(Name[1]))
    {
        Name = Name.RightChop(1);
    }

    FString Out;
    Out.Reserve(Name.Len() * 2);

    for (int32 Index = 0; Index < Name.Len(); ++Index)
    {
        const TCHAR Char = Name[Index];
        // Insert a space before an uppercase letter that follows a lowercase letter to break CamelCase words
        if (Index > 0 && FChar::IsUpper(Char) && !FChar::IsUpper(Name[Index - 1]))
        {
            Out += TEXT(" ");
        }
        Out.AppendChar(Char);
    }

    return Out;
}

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
    
    int32 AddedActions = 0;
    
    for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
    {
        if (!FunctionGraph)
        {
            continue;
        }
        
        FString FunctionName = FunctionGraph->GetName();
        
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintCustomFunctionActions: Checking function '%s'"), *FunctionName);
        
        if (!SearchFilter.IsEmpty() && !FunctionName.ToLower().Contains(SearchFilter.ToLower()))
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
    
    int32 AddedActions = 0;
    
    for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
    {
        FString VarName = VarDesc.VarName.ToString();
        
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintVariableActions: Checking variable '%s'"), *VarName);
        
        if (!SearchFilter.IsEmpty() && !VarName.ToLower().Contains(SearchFilter.ToLower()))
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
                if (!bRelevant && PinType.Equals(TEXT("object"), ESearchCase::IgnoreCase) && !ResolvedPinSubcategory.IsEmpty())
                {
                    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(ResolvedPinSubcategory);
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
    
    // --- BEGIN: Add native property getter/setter nodes for pin context ---
    UClass* TargetClass = UClass::TryFindTypeSlow<UClass>(ResolvedPinSubcategory);
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
                FString DisplayName = ConvertPropertyNameToDisplay(PropName);
                GetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Get %s"), *DisplayName));
                GetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                GetterObj->SetStringField(TEXT("category"), Category);
                GetterObj->SetStringField(TEXT("keywords"), Keywords);
                GetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableGet"));
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
                FString DisplayName = ConvertPropertyNameToDisplay(PropName);
                SetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Set %s"), *DisplayName));
                SetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                SetterObj->SetStringField(TEXT("category"), Category);
                SetterObj->SetStringField(TEXT("keywords"), Keywords);
                SetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableSet"));
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
                FString DisplayName = ConvertPropertyNameToDisplay(PropName);
                GetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Get %s"), *DisplayName));
                GetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                GetterObj->SetStringField(TEXT("category"), Category);
                GetterObj->SetStringField(TEXT("keywords"), Keywords);
                GetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableGet"));
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
                FString DisplayName = ConvertPropertyNameToDisplay(PropName);
                SetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Set %s"), *DisplayName));
                SetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                SetterObj->SetStringField(TEXT("category"), Category);
                SetterObj->SetStringField(TEXT("keywords"), Keywords);
                SetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableSet"));
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
                    FString DisplayName = ConvertPropertyNameToDisplay(PropName);
                    GetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Get %s"), *DisplayName));
                    GetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                    GetterObj->SetStringField(TEXT("category"), Category);
                    GetterObj->SetStringField(TEXT("keywords"), Keywords);
                    GetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableGet"));
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
                    FString DisplayName = ConvertPropertyNameToDisplay(PropName);
                    SetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Set %s"), *DisplayName));
                    SetterObj->SetStringField(TEXT("tooltip"), Tooltip);
                    SetterObj->SetStringField(TEXT("category"), Category);
                    SetterObj->SetStringField(TEXT("keywords"), Keywords);
                    SetterObj->SetStringField(TEXT("node_type"), TEXT("UK2Node_VariableSet"));
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

FString UUnrealMCPBlueprintActionCommands::GetNodePinInfo(const FString& NodeName, const FString& PinName)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    
    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Looking for pin '%s' on node '%s'"), *PinName, *NodeName);
    
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
    
    // SelectFloat node (KismetMathLibrary)
    TMap<FString, TSharedPtr<FJsonObject>> SelectFloatPins;
    
    auto SelectFloatAPin = MakeShared<FJsonObject>();
    SelectFloatAPin->SetStringField(TEXT("pin_type"), TEXT("real"));
    SelectFloatAPin->SetStringField(TEXT("expected_type"), TEXT("float"));
    SelectFloatAPin->SetStringField(TEXT("description"), TEXT("First float value option"));
    SelectFloatAPin->SetBoolField(TEXT("is_required"), true);
    SelectFloatAPin->SetBoolField(TEXT("is_input"), true);
    SelectFloatPins.Add(TEXT("A"), SelectFloatAPin);
    
    auto SelectFloatBPin = MakeShared<FJsonObject>();
    SelectFloatBPin->SetStringField(TEXT("pin_type"), TEXT("real"));
    SelectFloatBPin->SetStringField(TEXT("expected_type"), TEXT("float"));
    SelectFloatBPin->SetStringField(TEXT("description"), TEXT("Second float value option"));
    SelectFloatBPin->SetBoolField(TEXT("is_required"), true);
    SelectFloatBPin->SetBoolField(TEXT("is_input"), true);
    SelectFloatPins.Add(TEXT("B"), SelectFloatBPin);
    
    auto SelectFloatPickAPin = MakeShared<FJsonObject>();
    SelectFloatPickAPin->SetStringField(TEXT("pin_type"), TEXT("bool"));
    SelectFloatPickAPin->SetStringField(TEXT("expected_type"), TEXT("boolean"));
    SelectFloatPickAPin->SetStringField(TEXT("description"), TEXT("If true, returns A; if false, returns B"));
    SelectFloatPickAPin->SetBoolField(TEXT("is_required"), true);
    SelectFloatPickAPin->SetBoolField(TEXT("is_input"), true);
    SelectFloatPins.Add(TEXT("bPickA"), SelectFloatPickAPin);
    SelectFloatPins.Add(TEXT("Pick A"), SelectFloatPickAPin); // Alternative name
    
    auto SelectFloatReturnPin = MakeShared<FJsonObject>();
    SelectFloatReturnPin->SetStringField(TEXT("pin_type"), TEXT("real"));
    SelectFloatReturnPin->SetStringField(TEXT("expected_type"), TEXT("float"));
    SelectFloatReturnPin->SetStringField(TEXT("description"), TEXT("The selected float value (A or B)"));
    SelectFloatReturnPin->SetBoolField(TEXT("is_required"), false);
    SelectFloatReturnPin->SetBoolField(TEXT("is_input"), false);
    SelectFloatPins.Add(TEXT("ReturnValue"), SelectFloatReturnPin);
    SelectFloatPins.Add(TEXT("Return Value"), SelectFloatReturnPin); // Alternative name
    
    NodePinDatabase.Add(TEXT("SelectFloat"), SelectFloatPins);
    NodePinDatabase.Add(TEXT("Select Float"), SelectFloatPins);
    
    // RandomFloat node (KismetMathLibrary)
    TMap<FString, TSharedPtr<FJsonObject>> RandomFloatPins;
    
    auto RandomFloatReturnPin = MakeShared<FJsonObject>();
    RandomFloatReturnPin->SetStringField(TEXT("pin_type"), TEXT("real"));
    RandomFloatReturnPin->SetStringField(TEXT("expected_type"), TEXT("float"));
    RandomFloatReturnPin->SetStringField(TEXT("description"), TEXT("Random float value between 0.0 and 1.0"));
    RandomFloatReturnPin->SetBoolField(TEXT("is_required"), false);
    RandomFloatReturnPin->SetBoolField(TEXT("is_input"), false);
    RandomFloatPins.Add(TEXT("ReturnValue"), RandomFloatReturnPin);
    RandomFloatPins.Add(TEXT("Return Value"), RandomFloatReturnPin);
    
    NodePinDatabase.Add(TEXT("RandomFloat"), RandomFloatPins);
    NodePinDatabase.Add(TEXT("Random Float"), RandomFloatPins);
    
    // MultiplyByPi node (KismetMathLibrary)
    TMap<FString, TSharedPtr<FJsonObject>> MultiplyByPiPins;
    
    auto MultiplyByPiValuePin = MakeShared<FJsonObject>();
    MultiplyByPiValuePin->SetStringField(TEXT("pin_type"), TEXT("real"));
    MultiplyByPiValuePin->SetStringField(TEXT("expected_type"), TEXT("float"));
    MultiplyByPiValuePin->SetStringField(TEXT("description"), TEXT("Value to multiply by Pi"));
    MultiplyByPiValuePin->SetBoolField(TEXT("is_required"), true);
    MultiplyByPiValuePin->SetBoolField(TEXT("is_input"), true);
    MultiplyByPiPins.Add(TEXT("Value"), MultiplyByPiValuePin);
    
    auto MultiplyByPiReturnPin = MakeShared<FJsonObject>();
    MultiplyByPiReturnPin->SetStringField(TEXT("pin_type"), TEXT("real"));
    MultiplyByPiReturnPin->SetStringField(TEXT("expected_type"), TEXT("float"));
    MultiplyByPiReturnPin->SetStringField(TEXT("description"), TEXT("Value multiplied by Pi"));
    MultiplyByPiReturnPin->SetBoolField(TEXT("is_required"), false);
    MultiplyByPiReturnPin->SetBoolField(TEXT("is_input"), false);
    MultiplyByPiPins.Add(TEXT("ReturnValue"), MultiplyByPiReturnPin);
    MultiplyByPiPins.Add(TEXT("Return Value"), MultiplyByPiReturnPin);
    
    NodePinDatabase.Add(TEXT("MultiplyByPi"), MultiplyByPiPins);
    NodePinDatabase.Add(TEXT("Multiply By Pi"), MultiplyByPiPins);
    
    // Get Hidden node (Actor property)
    TMap<FString, TSharedPtr<FJsonObject>> GetHiddenPins;
    
    auto GetHiddenSelfPin = MakeShared<FJsonObject>();
    GetHiddenSelfPin->SetStringField(TEXT("pin_type"), TEXT("object"));
    GetHiddenSelfPin->SetStringField(TEXT("expected_type"), TEXT("Actor"));
    GetHiddenSelfPin->SetStringField(TEXT("description"), TEXT("The actor to get the hidden property from"));
    GetHiddenSelfPin->SetBoolField(TEXT("is_required"), true);
    GetHiddenSelfPin->SetBoolField(TEXT("is_input"), true);
    GetHiddenPins.Add(TEXT("self"), GetHiddenSelfPin);
    GetHiddenPins.Add(TEXT("Target"), GetHiddenSelfPin); // Alternative name
    
    auto GetHiddenReturnPin = MakeShared<FJsonObject>();
    GetHiddenReturnPin->SetStringField(TEXT("pin_type"), TEXT("bool"));
    GetHiddenReturnPin->SetStringField(TEXT("expected_type"), TEXT("boolean"));
    GetHiddenReturnPin->SetStringField(TEXT("description"), TEXT("Whether the actor is hidden"));
    GetHiddenReturnPin->SetBoolField(TEXT("is_required"), false);
    GetHiddenReturnPin->SetBoolField(TEXT("is_input"), false);
    GetHiddenPins.Add(TEXT("bHidden"), GetHiddenReturnPin);
    GetHiddenPins.Add(TEXT("Hidden"), GetHiddenReturnPin); // Alternative name
    
    NodePinDatabase.Add(TEXT("Get Hidden"), GetHiddenPins);
    NodePinDatabase.Add(TEXT("GetHidden"), GetHiddenPins);
    
    // For Each Loop (Map) node
    TMap<FString, TSharedPtr<FJsonObject>> ForEachMapPins;
    
    auto ForEachMapExecPin = MakeShared<FJsonObject>();
    ForEachMapExecPin->SetStringField(TEXT("pin_type"), TEXT("exec"));
    ForEachMapExecPin->SetStringField(TEXT("expected_type"), TEXT("exec"));
    ForEachMapExecPin->SetStringField(TEXT("description"), TEXT("Execution input to start the loop"));
    ForEachMapExecPin->SetBoolField(TEXT("is_required"), true);
    ForEachMapExecPin->SetBoolField(TEXT("is_input"), true);
    ForEachMapPins.Add(TEXT("execute"), ForEachMapExecPin);
    ForEachMapPins.Add(TEXT("Exec"), ForEachMapExecPin); // Alternative name
    
    auto ForEachMapPin = MakeShared<FJsonObject>();
    ForEachMapPin->SetStringField(TEXT("pin_type"), TEXT("wildcard"));
    ForEachMapPin->SetStringField(TEXT("expected_type"), TEXT("Map"));
    ForEachMapPin->SetStringField(TEXT("description"), TEXT("The map to iterate over"));
    ForEachMapPin->SetBoolField(TEXT("is_required"), true);
    ForEachMapPin->SetBoolField(TEXT("is_input"), true);
    ForEachMapPins.Add(TEXT("MapPin"), ForEachMapPin);
    ForEachMapPins.Add(TEXT("Map"), ForEachMapPin); // Alternative name
    
    auto ForEachMapThenPin = MakeShared<FJsonObject>();
    ForEachMapThenPin->SetStringField(TEXT("pin_type"), TEXT("exec"));
    ForEachMapThenPin->SetStringField(TEXT("expected_type"), TEXT("exec"));
    ForEachMapThenPin->SetStringField(TEXT("description"), TEXT("Execution output for each iteration"));
    ForEachMapThenPin->SetBoolField(TEXT("is_required"), false);
    ForEachMapThenPin->SetBoolField(TEXT("is_input"), false);
    ForEachMapPins.Add(TEXT("then"), ForEachMapThenPin);
    ForEachMapPins.Add(TEXT("Loop Body"), ForEachMapThenPin); // Alternative name
    
    auto ForEachMapKeyPin = MakeShared<FJsonObject>();
    ForEachMapKeyPin->SetStringField(TEXT("pin_type"), TEXT("wildcard"));
    ForEachMapKeyPin->SetStringField(TEXT("expected_type"), TEXT("Key"));
    ForEachMapKeyPin->SetStringField(TEXT("description"), TEXT("Current key in the iteration"));
    ForEachMapKeyPin->SetBoolField(TEXT("is_required"), false);
    ForEachMapKeyPin->SetBoolField(TEXT("is_input"), false);
    ForEachMapPins.Add(TEXT("KeyPin"), ForEachMapKeyPin);
    ForEachMapPins.Add(TEXT("Key"), ForEachMapKeyPin); // Alternative name
    
    auto ForEachMapValuePin = MakeShared<FJsonObject>();
    ForEachMapValuePin->SetStringField(TEXT("pin_type"), TEXT("wildcard"));
    ForEachMapValuePin->SetStringField(TEXT("expected_type"), TEXT("Value"));
    ForEachMapValuePin->SetStringField(TEXT("description"), TEXT("Current value in the iteration"));
    ForEachMapValuePin->SetBoolField(TEXT("is_required"), false);
    ForEachMapValuePin->SetBoolField(TEXT("is_input"), false);
    ForEachMapPins.Add(TEXT("ValuePin"), ForEachMapValuePin);
    ForEachMapPins.Add(TEXT("Value"), ForEachMapValuePin); // Alternative name
    
    auto ForEachMapCompletedPin = MakeShared<FJsonObject>();
    ForEachMapCompletedPin->SetStringField(TEXT("pin_type"), TEXT("exec"));
    ForEachMapCompletedPin->SetStringField(TEXT("expected_type"), TEXT("exec"));
    ForEachMapCompletedPin->SetStringField(TEXT("description"), TEXT("Execution output when loop completes"));
    ForEachMapCompletedPin->SetBoolField(TEXT("is_required"), false);
    ForEachMapCompletedPin->SetBoolField(TEXT("is_input"), false);
    ForEachMapPins.Add(TEXT("CompletedPin"), ForEachMapCompletedPin);
    ForEachMapPins.Add(TEXT("Completed"), ForEachMapCompletedPin); // Alternative name
    
    NodePinDatabase.Add(TEXT("For Each Loop (Map)"), ForEachMapPins);
    NodePinDatabase.Add(TEXT("ForEachLoop(Map)"), ForEachMapPins);
    NodePinDatabase.Add(TEXT("For Each Loop Map"), ForEachMapPins);
    
    // Look up the requested node and pin with flexible matching
    FString NormalizedNodeName = NodeName;
    NormalizedNodeName = NormalizedNodeName.Replace(TEXT(" "), TEXT(""));
    
    TSharedPtr<FJsonObject> PinInfo = nullptr;
    FString FoundNodeKey;
    TMap<FString, TSharedPtr<FJsonObject>>* FoundPinMap = nullptr;
    
    // Try exact match first
    if (NodePinDatabase.Contains(NodeName))
    {
        FoundNodeKey = NodeName;
        FoundPinMap = &NodePinDatabase[NodeName];
    }
    // Try normalized match (no spaces)
    else if (NodePinDatabase.Contains(NormalizedNodeName))
    {
        FoundNodeKey = NormalizedNodeName;
        FoundPinMap = &NodePinDatabase[NormalizedNodeName];
    }
    // Try case-insensitive match
    else
    {
        for (const auto& NodeEntry : NodePinDatabase)
        {
            if (NodeEntry.Key.Equals(NodeName, ESearchCase::IgnoreCase) ||
                NodeEntry.Key.Replace(TEXT(" "), TEXT("")).Equals(NormalizedNodeName, ESearchCase::IgnoreCase))
            {
                FoundNodeKey = NodeEntry.Key;
                FoundPinMap = const_cast<TMap<FString, TSharedPtr<FJsonObject>>*>(&NodeEntry.Value);
                break;
            }
        }
    }
    
    // Look for the pin in the found node
    if (FoundPinMap)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Found node '%s' (matched with '%s')"), *FoundNodeKey, *NodeName);
        
        // Try exact pin name match first
        if (FoundPinMap->Contains(PinName))
        {
            PinInfo = (*FoundPinMap)[PinName];
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Found exact pin match '%s'"), *PinName);
        }
        // Try case-insensitive pin match
        else
        {
            for (const auto& PinEntry : *FoundPinMap)
            {
                if (PinEntry.Key.Equals(PinName, ESearchCase::IgnoreCase))
                {
                    PinInfo = PinEntry.Value;
                    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Found case-insensitive pin match '%s' -> '%s'"), *PinName, *PinEntry.Key);
                    break;
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Node '%s' not found in database"), *NodeName);
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
        ResultObj->SetStringField(TEXT("error"), FString::Printf(TEXT("No pin information found for '%s' on node '%s'"), *PinName, *NodeName));
        
        // Provide available pins for this node if we know the node
        if (FoundPinMap)
        {
            TArray<TSharedPtr<FJsonValue>> AvailablePins;
            for (const auto& Pin : *FoundPinMap)
            {
                AvailablePins.Add(MakeShared<FJsonValueString>(Pin.Key));
            }
            ResultObj->SetArrayField(TEXT("available_pins"), AvailablePins);
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Provided %d available pins for node '%s'"), AvailablePins.Num(), *FoundNodeKey);
        }
        else
        {
            // Provide list of known nodes
            TArray<TSharedPtr<FJsonValue>> AvailableNodes;
            for (const auto& NodeEntry : NodePinDatabase)
            {
                AvailableNodes.Add(MakeShared<FJsonValueString>(NodeEntry.Key));
            }
            ResultObj->SetArrayField(TEXT("available_nodes"), AvailableNodes);
            UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Provided %d available nodes"), AvailableNodes.Num());
        }
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    
    UE_LOG(LogTemp, Warning, TEXT("GetNodePinInfo: Returning JSON response: %s"), *OutputString);
    
    return OutputString;
}

FString UUnrealMCPBlueprintActionCommands::SearchBlueprintActions(const FString& SearchQuery, const FString& Category, int32 MaxResults, const FString& BlueprintName)
{
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions called with: SearchQuery='%s', Category='%s', MaxResults=%d, BlueprintName='%s'"), *SearchQuery, *Category, MaxResults, *BlueprintName);
    
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
        UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Using FUnrealMCPCommonUtils to find Blueprint: %s"), *BlueprintName);
        UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprintByName(BlueprintName);
        
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
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Failed to load Blueprint: %s using FUnrealMCPCommonUtils"), *BlueprintName);
        }
    }
    
    // Use Unreal's native Blueprint Action Menu system instead of custom search
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Using native UE Blueprint Action Menu system for search '%s' in category '%s'"), *SearchQuery, *Category);
    
    // Go back to using FBlueprintActionDatabase directly - the native approach was causing crashes
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    
    // Create a context for filtering
    FBlueprintActionContext FilterContext;
    if (!BlueprintName.IsEmpty())
    {
        UBlueprint* ContextBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *FString::Printf(TEXT("/Game/%s.%s"), *BlueprintName, *BlueprintName), nullptr, LOAD_Quiet | LOAD_NoWarn));
        if (ContextBlueprint)
        {
            FilterContext.Blueprints.Add(ContextBlueprint);
        }
    }
    
    // Use BlueprintActionDatabase to get actions with minimal filtering
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Action database has %d action lists"), ActionRegistry.Num());
    
    // Go through all actions in the database
    for (auto Iterator(ActionRegistry.CreateConstIterator()); Iterator; ++Iterator)
    {
        const FBlueprintActionDatabase::FActionList& ActionList = Iterator.Value();
        for (UBlueprintNodeSpawner* NodeSpawner : ActionList)
        {
            if (!NodeSpawner)
            {
                continue;
            }
        
        UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();
        if (!TemplateNode)
        {
            continue;
        }
        
        // Get the display name using the proper node method
        FString ActionName;
        FString NodeType;
        
        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
        {
            ActionName = FunctionNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
            NodeType = TEXT("Function");
        }
        else
        {
            ActionName = TemplateNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
            NodeType = TemplateNode->GetClass()->GetName();
        }
        
        // Get other properties
        FString ActionCategory = TEXT("Unknown");
        FString Tooltip = TemplateNode->GetTooltipText().ToString();
        FString Keywords = TemplateNode->GetKeywords().ToString();
        
        // Apply search and category filters
        FString SearchLower = SearchQuery.ToLower();
        FString ActionNameLower = ActionName.ToLower();
        FString ActionCategoryLower = ActionCategory.ToLower();
        FString TooltipLower = Tooltip.ToLower();
        FString KeywordsLower = Keywords.ToLower();
        
        bool bMatchesSearch = ActionNameLower.Contains(SearchLower) ||
                             ActionCategoryLower.Contains(SearchLower) ||
                             TooltipLower.Contains(SearchLower) ||
                             KeywordsLower.Contains(SearchLower);
        
        bool bMatchesCategory = Category.IsEmpty() || ActionCategoryLower.Contains(Category.ToLower());
        
        if (bMatchesSearch && bMatchesCategory)
        {
            TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
            ActionObj->SetStringField(TEXT("title"), ActionName);
            ActionObj->SetStringField(TEXT("tooltip"), Tooltip);
            ActionObj->SetStringField(TEXT("category"), ActionCategory);
            ActionObj->SetStringField(TEXT("keywords"), Keywords);
            ActionObj->SetStringField(TEXT("node_type"), NodeType);
            
            // Try to extract additional information if it's a function call
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
                goto EndSearch; // Break out of both loops
            }
        } // Close bMatchesSearch condition
        } // Close inner NodeSpawner loop
    }
    
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Standard search completed. Found %d actions from database iteration"), ActionsArray.Num());
    
EndSearch:
    
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

FString UUnrealMCPBlueprintActionCommands::CreateNodeByActionName(const FString& BlueprintName, const FString& FunctionName, const FString& ClassName, const FString& NodePosition, const FString& JsonParams)
{
    // Delegate to the extracted function in UnrealMCPNodeCreators
    return UnrealMCPNodeCreators::CreateNodeByActionName(BlueprintName, FunctionName, ClassName, NodePosition, JsonParams);
}



