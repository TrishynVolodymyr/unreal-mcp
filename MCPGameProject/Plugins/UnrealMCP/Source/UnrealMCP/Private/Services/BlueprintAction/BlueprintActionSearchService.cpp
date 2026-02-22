// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/BlueprintAction/BlueprintActionSearchService.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintTypePromotion.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "Engine/Blueprint.h"
#include "Utils/UnrealMCPCommonUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "InputAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraphSchema_K2.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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

// Helper: Add Blueprint-local custom function actions
static void AddBlueprintCustomFunctionActions(UBlueprint* Blueprint, const FString& SearchFilter, TArray<TSharedPtr<FJsonValue>>& OutActions)
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
static void AddBlueprintVariableActions(UBlueprint* Blueprint, const FString& SearchFilter, TArray<TSharedPtr<FJsonValue>>& OutActions)
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

static void AddBlueprintComponentActions(UBlueprint* Blueprint, const FString& SearchFilter, TArray<TSharedPtr<FJsonValue>>& OutActions)
{
    if (!Blueprint) 
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: Blueprint is null"));
        return;
    }
    
    // Get components from the Blueprint's SimpleConstructionScript
    USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
    if (!SCS)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: No SimpleConstructionScript found"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: Processing Blueprint '%s' with %d component nodes"), 
           *Blueprint->GetName(), SCS->GetAllNodes().Num());
    
    int32 AddedActions = 0;
    
    for (USCS_Node* Node : SCS->GetAllNodes())
    {
        if (!Node || !Node->ComponentTemplate)
        {
            continue;
        }
        
        FString ComponentName = Node->GetVariableName().ToString();
        FString ComponentClassName = Node->ComponentTemplate->GetClass()->GetName();
        
        UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: Checking component '%s' (type: %s)"), *ComponentName, *ComponentClassName);
        
        if (!SearchFilter.IsEmpty() && !ComponentName.ToLower().Contains(SearchFilter.ToLower()))
        {
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: Component '%s' doesn't match search filter '%s'"), *ComponentName, *SearchFilter);
            continue;
        }
        
        // Getter
        {
            TSharedPtr<FJsonObject> GetterObj = MakeShared<FJsonObject>();
            GetterObj->SetStringField(TEXT("title"), FString::Printf(TEXT("Get %s"), *ComponentName));
            GetterObj->SetStringField(TEXT("tooltip"), FString::Printf(TEXT("Get the %s component (%s)"), *ComponentName, *ComponentClassName));
            GetterObj->SetStringField(TEXT("category"), TEXT("Components"));
            GetterObj->SetStringField(TEXT("variable_name"), ComponentName);
            GetterObj->SetStringField(TEXT("component_class"), ComponentClassName);
            GetterObj->SetStringField(TEXT("pin_type"), TEXT("object"));
            GetterObj->SetStringField(TEXT("function_name"), FString::Printf(TEXT("Get %s"), *ComponentName));
            GetterObj->SetBoolField(TEXT("is_blueprint_component"), true);
            OutActions.Add(MakeShared<FJsonValueObject>(GetterObj));
            AddedActions++;
            UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: Added getter for component '%s'"), *ComponentName);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AddBlueprintComponentActions: Added %d component actions total"), AddedActions);
}

FBlueprintActionSearchService::FBlueprintActionSearchService()
{
}

FBlueprintActionSearchService::~FBlueprintActionSearchService()
{
}

FString FBlueprintActionSearchService::SearchBlueprintActions(
    const FString& SearchQuery,
    const FString& Category,
    int32 MaxResults,
    const FString& BlueprintName
)
{
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions called with: SearchQuery='%s', Category='%s', MaxResults=%d, BlueprintName='%s'"), *SearchQuery, *Category, MaxResults, *BlueprintName);
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    
    // Convert CamelCase to Title Case for better search results (e.g., "GetActorLocation" -> "Get Actor Location")
    FString TitleCaseQuery = ConvertCamelCaseToTitleCase(SearchQuery);
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: CamelCase conversion: '%s' -> '%s'"), *SearchQuery, *TitleCaseQuery);
    
    // Use title case version for searching if it's different from original
    FString EffectiveSearchQuery = TitleCaseQuery.Equals(SearchQuery, ESearchCase::IgnoreCase) ? SearchQuery : TitleCaseQuery;
    
    // Keep original query for searching internal function names (e.g. "Conv_IntToText")
    // since CamelCase conversion can mangle underscore-separated names
    FString OriginalSearchLower = SearchQuery.ToLower();
    
    // Declare variables that might be used across different sections (to avoid goto issues)
    FString SearchLower = EffectiveSearchQuery.ToLower();
    const TSet<FName>& OperatorNames = FTypePromotion::GetAllOpNames();
    
    // Initialize variables that will be used after potential goto
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionContext FilterContext;
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
    
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
    
    // Check if this is a mathematical query that should prioritize math operators
    bool bIsMathQuery = SearchLower == TEXT("add") || SearchLower == TEXT("subtract") || 
                       SearchLower == TEXT("multiply") || SearchLower == TEXT("divide") ||
                       SearchLower == TEXT("math") || SearchLower == TEXT("operator") ||
                       SearchLower.Contains(TEXT("+")) || SearchLower.Contains(TEXT("-")) ||
                       SearchLower.Contains(TEXT("*")) || SearchLower.Contains(TEXT("/"));
    
    // Check if this is a comparison query that should prioritize comparison operators
    bool bIsComparisonQuery = SearchLower == TEXT("greater") || SearchLower == TEXT("less") ||
                             SearchLower == TEXT("equal") || SearchLower == TEXT("compare") ||
                             SearchLower == TEXT("comparison") || SearchLower.Contains(TEXT(">")) ||
                             SearchLower.Contains(TEXT("<")) || SearchLower.Contains(TEXT("=")) ||
                             SearchLower.Contains(TEXT("<=")) || SearchLower.Contains(TEXT(">=")) ||
                             SearchLower.Contains(TEXT("==")) || SearchLower.Contains(TEXT("!=")) ||
                             // Additional wildcard operator patterns
                             SearchLower == TEXT("operator") || SearchLower == TEXT("wildcard") ||
                             SearchLower == TEXT("promotable");
    
    // Combined check for any type promotion operator
    bool bIsTypePromotionQuery = bIsMathQuery || bIsComparisonQuery;
    
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
            
            // Add component actions
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Adding component actions for Blueprint: %s"), *Blueprint->GetName());
            AddBlueprintComponentActions(Blueprint, SearchQuery, ActionsArray);
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Added %d total actions after components"), ActionsArray.Num());
            
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
    
    // Search for Enhanced Input Actions via Asset Registry (like UK2Node_EnhancedInputAction::GetMenuActions does)
    // This is needed because Enhanced Input event nodes are registered through a different mechanism
    if (Category.IsEmpty() || Category.ToLower().Contains(TEXT("input")))
    {
        UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Searching for Enhanced Input Actions"));
        
        IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        TArray<FAssetData> ActionAssets;
        AssetRegistry.GetAssetsByClass(UInputAction::StaticClass()->GetClassPathName(), ActionAssets, true);
        
        UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Found %d Enhanced Input Action assets"), ActionAssets.Num());
        
        for (const FAssetData& ActionAsset : ActionAssets)
        {
            FString ActionName = ActionAsset.AssetName.ToString();
            FString ActionNameLower = ActionName.ToLower();
            
            // Check if this action matches our search
            if (ActionNameLower.Contains(SearchLower))
            {
                if (const UInputAction* Action = Cast<const UInputAction>(ActionAsset.GetAsset()))
                {
                    TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
                    ActionObj->SetStringField(TEXT("title"), ActionName);
                    ActionObj->SetStringField(TEXT("tooltip"), FString::Printf(TEXT("Enhanced Input Action event for '%s'"), *ActionName));
                    ActionObj->SetStringField(TEXT("category"), TEXT("Input|Enhanced Action Events"));
                    ActionObj->SetStringField(TEXT("function_name"), ActionName);
                    ActionObj->SetStringField(TEXT("class_name"), TEXT("EnhancedInputAction"));
                    
                    TArray<TSharedPtr<FJsonValue>> KeywordsArray;
                    KeywordsArray.Add(MakeShared<FJsonValueString>(TEXT("input")));
                    KeywordsArray.Add(MakeShared<FJsonValueString>(TEXT("enhanced")));
                    KeywordsArray.Add(MakeShared<FJsonValueString>(TEXT("action")));
                    KeywordsArray.Add(MakeShared<FJsonValueString>(TEXT("event")));
                    ActionObj->SetArrayField(TEXT("keywords"), KeywordsArray);
                    
                    ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
                    
                    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Added Enhanced Input Action: %s"), *ActionName);
                    
                    // Limit results
                    if (ActionsArray.Num() >= MaxResults)
                    {
                        goto EndSearch;
                    }
                }
            }
        }
    }
    
    // Prioritize type promotion operators (math and comparison) for relevant queries
    if (bIsTypePromotionQuery && (Category.IsEmpty() || Category.ToLower().Contains(TEXT("math")) || 
                                 Category.ToLower().Contains(TEXT("utilities")) || Category.ToLower().Contains(TEXT("operators")) ||
                                 Category.ToLower().Contains(TEXT("comparison"))))
    {
        FString QueryType = bIsMathQuery ? TEXT("mathematical") : (bIsComparisonQuery ? TEXT("comparison") : TEXT("type promotion"));
        UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Prioritizing %s operators for query '%s'"), *QueryType, *SearchQuery);
        UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Available operators count: %d"), OperatorNames.Num());
        
        for (const FName& OpName : OperatorNames)
        {
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Checking operator: %s"), *OpName.ToString());
        }
        
        for (const FName& OpName : OperatorNames)
        {
            FString OpNameString = OpName.ToString();
            FString OpNameLower = OpNameString.ToLower();
            
            // Check if this operator matches our search
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Testing operator '%s' against search '%s'"), *OpNameString, *SearchLower);
            bool bMatchesSearch = OpNameLower.Contains(SearchLower) ||
                                 // Math operators
                                 (SearchLower == TEXT("add") && (OpNameString == TEXT("Add") || OpNameString.Contains(TEXT("+")))) ||
                                 (SearchLower == TEXT("subtract") && (OpNameString == TEXT("Subtract") || OpNameString.Contains(TEXT("-")))) ||
                                 (SearchLower == TEXT("multiply") && (OpNameString == TEXT("Multiply") || OpNameString.Contains(TEXT("*")))) ||
                                 (SearchLower == TEXT("divide") && (OpNameString == TEXT("Divide") || OpNameString.Contains(TEXT("/")))) ||
                                 // Comparison operators - FIXED LOGIC
                                 (SearchLower.Contains(TEXT("<=")) && OpNameString == TEXT("LessEqual")) ||
                                 (SearchLower.Contains(TEXT(">=")) && OpNameString == TEXT("GreaterEqual")) ||
                                 (SearchLower.Contains(TEXT("==")) && OpNameString == TEXT("EqualEqual")) ||
                                 (SearchLower.Contains(TEXT("!=")) && OpNameString == TEXT("NotEqual")) ||
                                 (SearchLower.Contains(TEXT("<")) && !SearchLower.Contains(TEXT("<=")) && OpNameString == TEXT("Less")) ||
                                 (SearchLower.Contains(TEXT(">")) && !SearchLower.Contains(TEXT(">=")) && OpNameString == TEXT("Greater")) ||
                                 // Text-based matches - IMPROVED
                                 (SearchLower == TEXT("greater") && OpNameString == TEXT("Greater")) ||
                                 (SearchLower == TEXT("less") && OpNameString == TEXT("Less")) ||
                                 (SearchLower == TEXT("equal") && OpNameString == TEXT("EqualEqual")) ||
                                 // Symbol-to-operator mapping for wildcards
                                 (SearchLower == TEXT("+") && OpNameString == TEXT("Add")) ||
                                 (SearchLower == TEXT("-") && OpNameString == TEXT("Subtract")) ||
                                 (SearchLower == TEXT("*") && OpNameString == TEXT("Multiply")) ||
                                 (SearchLower == TEXT("/") && OpNameString == TEXT("Divide")) ||
                                 (SearchLower == TEXT("<") && OpNameString == TEXT("Less")) ||
                                 (SearchLower == TEXT(">") && OpNameString == TEXT("Greater")) ||
                                 (SearchLower == TEXT("<=") && OpNameString == TEXT("LessEqual")) ||
                                 (SearchLower == TEXT(">=") && OpNameString == TEXT("GreaterEqual")) ||
                                 (SearchLower == TEXT("==") && OpNameString == TEXT("EqualEqual")) ||
                                 (SearchLower == TEXT("!=") && OpNameString == TEXT("NotEqual")) ||
                                 // General terms
                                 (SearchLower == TEXT("math") || SearchLower == TEXT("operator") || SearchLower == TEXT("compare") || SearchLower == TEXT("comparison"));
            
            if (bMatchesSearch)
            {
                UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: MATCHED operator '%s' for search '%s'"), *OpNameString, *SearchLower);
                // Get additional information about the operator
                // NOTE: GetOperatorSpawner may return null if TypePromotion spawner map isn't populated
                // (happens when no Blueprint Editor context menu has been built yet).
                // We still add the operator to results because ArithmeticNodeCreator has its own
                // fallback that creates UK2Node_PromotableOperator directly without a spawner.
                FText UserFacingName = FTypePromotion::GetUserFacingOperatorName(OpName);
                
                TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
                ActionObj->SetStringField(TEXT("title"), UserFacingName.IsEmpty() ? OpNameString : UserFacingName.ToString());
                // Determine if this is a comparison operator
                bool bIsComparisonOp = FTypePromotion::IsComparisonOpName(OpName);
                FString OperatorType = bIsComparisonOp ? TEXT("Comparison operator") : TEXT("Mathematical operator");
                ActionObj->SetStringField(TEXT("tooltip"), FString::Printf(TEXT("%s: %s (wildcard — accepts any numeric type)"), *OperatorType, UserFacingName.IsEmpty() ? *OpNameString : *UserFacingName.ToString()));
                ActionObj->SetStringField(TEXT("category"), TEXT("Utilities|Operators"));
                // function_name is what create_node_by_action_name expects — ArithmeticNodeCreator handles the rest
                ActionObj->SetStringField(TEXT("function_name"), OpNameString);
                ActionObj->SetBoolField(TEXT("is_promotable_operator"), true);
                
                ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
                
                UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Added %s operator: %s"), bIsComparisonOp ? TEXT("comparison") : TEXT("mathematical"), *OpNameString);
                
                // Limit results
                if (ActionsArray.Num() >= MaxResults)
                {
                    goto EndSearch;
                }
            }
        }
    }

    // Use Unreal's native Blueprint Action Menu system for remaining search
    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Using native UE Blueprint Action Menu system for search '%s' in category '%s'"), *SearchQuery, *Category);
    if (!BlueprintName.IsEmpty())
    {
        // Fix: Handle both short names (BP_MyBlueprint) and full paths (/Game/Folder/BP_MyBlueprint)
        FString BlueprintPath;
        if (BlueprintName.StartsWith(TEXT("/Game/")))
        {
            // Already a full path - extract just the asset name for the class reference
            FString AssetName;
            BlueprintName.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
            AssetName.RemoveFromEnd(TEXT("_C")); // Remove _C suffix if present
            BlueprintPath = FString::Printf(TEXT("%s.%s"), *BlueprintName, *AssetName);
        }
        else
        {
            // Short name - add /Game/ prefix
            BlueprintPath = FString::Printf(TEXT("/Game/%s.%s"), *BlueprintName, *BlueprintName);
        }
        
        UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Loading blueprint from path '%s'"), *BlueprintPath);
        UBlueprint* ContextBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintPath, nullptr, LOAD_Quiet | LOAD_NoWarn));
        if (ContextBlueprint)
        {
            FilterContext.Blueprints.Add(ContextBlueprint);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Failed to load blueprint from path '%s'"), *BlueprintPath);
        }
    }
    
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
        
            // Get the display name and properties - handle function spawners differently
            FString ActionName;
            FString InternalFunctionName; // The C++ function name (e.g. "Conv_IntToText") - used for search
            FString NodeType;
            FString ActionCategory = TEXT("Unknown");
            FString Tooltip;
            FString Keywords;
            
            // Declare template node for later use
            UEdGraphNode* TemplateNode = nullptr;
            
            // Check if this is a function spawner (for KismetMathLibrary and other function libraries)
            if (UBlueprintFunctionNodeSpawner* FunctionSpawner = Cast<UBlueprintFunctionNodeSpawner>(NodeSpawner))
            {
                UFunction const* Function = FunctionSpawner->GetFunction();
                if (Function)
                {
                    ActionName = Function->GetDisplayNameText().ToString();
                    InternalFunctionName = Function->GetName();
                    if (ActionName.IsEmpty())
                    {
                        ActionName = InternalFunctionName;
                    }
                    NodeType = TEXT("Function");
                    
                    // Extract category from metadata
                    if (Function->HasMetaData(TEXT("Category")))
                    {
                        ActionCategory = Function->GetMetaData(TEXT("Category"));
                    }
                    
                    // Extract tooltip from metadata
                    if (Function->HasMetaData(TEXT("ToolTip")))
                    {
                        Tooltip = Function->GetMetaData(TEXT("ToolTip"));
                    }
                    else
                    {
                        Tooltip = Function->GetToolTipText().ToString();
                    }
                    
                    // Extract keywords from metadata
                    if (Function->HasMetaData(TEXT("Keywords")))
                    {
                        Keywords = Function->GetMetaData(TEXT("Keywords"));
                    }
                    
                    UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Found function '%s' from class '%s', category: '%s'"), 
                        *ActionName, *Function->GetOuterUClass()->GetName(), *ActionCategory);
                }
                else
                {
                    continue;
                }
            }
            // Check if this is an event spawner (for Event Tick, Custom Events, etc.)
            else if (UBlueprintEventNodeSpawner* EventSpawner = Cast<UBlueprintEventNodeSpawner>(NodeSpawner))
            {
                // Get menu signature to extract the display name
                const FBlueprintActionUiSpec& MenuSignature = NodeSpawner->DefaultMenuSignature;
                ActionName = MenuSignature.MenuName.ToString();
                NodeType = TEXT("Event");
                
                // Extract category from menu signature
                if (!MenuSignature.Category.IsEmpty())
                {
                    ActionCategory = MenuSignature.Category.ToString();
                }
                
                // Extract tooltip from menu signature
                if (!MenuSignature.Tooltip.IsEmpty())
                {
                    Tooltip = MenuSignature.Tooltip.ToString();
                }
                
                // Extract keywords from menu signature
                if (!MenuSignature.Keywords.IsEmpty())
                {
                    Keywords = MenuSignature.Keywords.ToString();
                }
                
                UE_LOG(LogTemp, Warning, TEXT("SearchBlueprintActions: Found event '%s', category: '%s'"), 
                    *ActionName, *ActionCategory);
            }
            else
            {
                // Handle regular template nodes
                TemplateNode = NodeSpawner->GetTemplateNode();
                if (!TemplateNode)
                {
                    continue;
                }
                
                if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                {
                    ActionName = FunctionNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
                    NodeType = TEXT("Function");
                    if (UFunction* Func = FunctionNode->GetTargetFunction())
                    {
                        InternalFunctionName = Func->GetName();
                    }
                }
                else
                {
                    ActionName = TemplateNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
                    NodeType = TemplateNode->GetClass()->GetName();
                }
                
                Tooltip = TemplateNode->GetTooltipText().ToString();
                Keywords = TemplateNode->GetKeywords().ToString();
            }
        
        // Apply search and category filters (SearchLower already declared at function start)
        FString ActionNameLower = ActionName.ToLower();
        FString InternalFunctionNameLower = InternalFunctionName.ToLower();
        FString ActionCategoryLower = ActionCategory.ToLower();
        FString TooltipLower = Tooltip.ToLower();
        FString KeywordsLower = Keywords.ToLower();
        
        // Search across display name, internal function name (e.g. Conv_IntToText),
        // category, tooltip, and keywords.
        // For internal function names, also match against the original (non-CamelCase-converted) query
        // because CamelCase conversion mangles underscore names like "Conv_IntToText".
        bool bMatchesSearch = ActionNameLower.Contains(SearchLower) ||
                             (!InternalFunctionNameLower.IsEmpty() && (
                                 InternalFunctionNameLower.Contains(SearchLower) ||
                                 InternalFunctionNameLower.Contains(OriginalSearchLower))) ||
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
            
            // Handle function spawners (for KismetMathLibrary and other function libraries)
            if (UBlueprintFunctionNodeSpawner* FunctionSpawner = Cast<UBlueprintFunctionNodeSpawner>(NodeSpawner))
            {
                UFunction const* Function = FunctionSpawner->GetFunction();
                if (Function)
                {
                    ActionObj->SetStringField(TEXT("function_name"), Function->GetName());
                    ActionObj->SetStringField(TEXT("class_name"), Function->GetOwnerClass()->GetName());
                    if (Function->GetOwnerClass() == UKismetMathLibrary::StaticClass())
                    {
                        ActionObj->SetBoolField(TEXT("is_math_function"), true);
                    }
                }
            }
            // Handle event spawners (for Event Tick, Custom Events, etc.)
            else if (UBlueprintEventNodeSpawner* EventSpawner = Cast<UBlueprintEventNodeSpawner>(NodeSpawner))
            {
                // For events, use the action name as function_name
                // Special handling for "Add Custom Event..." - use "CustomEvent" as function name
                if (ActionName.Contains(TEXT("Add Custom Event")))
                {
                    ActionObj->SetStringField(TEXT("function_name"), TEXT("CustomEvent"));
                }
                else
                {
                    ActionObj->SetStringField(TEXT("function_name"), ActionName);
                }
                ActionObj->SetStringField(TEXT("class_name"), TEXT(""));
            }
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
            else
            {
                // For non-function nodes, use the node class name or a fallback
                // This ensures all actions have function_name for create_node_by_action_name
                if (TemplateNode)
                {
                    // Use the action name as function_name for non-function nodes
                    ActionObj->SetStringField(TEXT("function_name"), ActionName);
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

bool FBlueprintActionSearchService::MatchesSearchQuery(
    const FString& ActionName,
    const FString& Category,
    const FString& Tooltip,
    const FString& Keywords,
    const FString& SearchQuery
)
{
    // TODO: Move search matching logic (lines ~1450-1470)
    // Case-insensitive search across name, category, tooltip, keywords
    
    return false;
}

bool FBlueprintActionSearchService::MatchesCategoryFilter(
    const FString& ActionCategory,
    const FString& CategoryFilter
)
{
    // TODO: Move category filtering logic (lines ~1472-1480)
    // Checks if action category matches filter
    
    return false;
}

void FBlueprintActionSearchService::DiscoverLocalVariables(
    const FString& BlueprintName,
    TArray<TSharedPtr<FJsonValue>>& OutActions,
    const FString& SearchQuery
)
{
    // TODO: Move local variable discovery (lines ~1327-1390)
    // Finds Blueprint variables and creates Get/Set node actions
}
