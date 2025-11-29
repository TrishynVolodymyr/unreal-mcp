// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintActionDatabaseNodeCreator.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "NodeCreationHelpers.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"

// Helper function to normalize function names by removing common UE prefixes
static FString NormalizeFunctionName(const FString& Name)
{
    // List of known Unreal Engine function prefixes
    static const TArray<FString> Prefixes = {
        TEXT("K2_"),      // Blueprint-callable functions (e.g., K2_GetActorLocation)
        TEXT("BP_"),      // Blueprint-specific functions
        TEXT("EdGraph_"), // EdGraph functions
        TEXT("UE_")       // UE-specific functions
    };
    
    for (const FString& Prefix : Prefixes)
    {
        if (Name.StartsWith(Prefix))
        {
            return Name.Mid(Prefix.Len());
        }
    }
    
    return Name;
}

bool FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(
    const FString& FunctionName,
    const FString& ClassName,
    UEdGraph* EventGraph,
    float PositionX,
    float PositionY,
    UEdGraphNode*& NewNode,
    FString& NodeTitle,
    FString& NodeType,
    FString* OutErrorMessage
)
{
    UE_LOG(LogTemp, Warning, TEXT("=== TryCreateNodeUsingBlueprintActionDatabase START ==="));
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
    FString TitleCaseVersion = NodeCreationHelpers::ConvertCamelCaseToTitleCase(FunctionName);
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
    
    // CRITICAL FIX: Track all matching functions to detect duplicates
    TMap<FString, TArray<FString>> MatchingFunctionsByClass; // FunctionName -> Array of ClassName
    TArray<const UBlueprintNodeSpawner*> MatchingSpawners; // Store spawners for later use
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Starting spawner search loop..."));
    int32 ProcessedSpawners = 0;
    
    // First pass: Collect all matching functions
    for (const auto& ActionPair : ActionRegistry)
    {
        for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
        {
            ProcessedSpawners++;
            if (ProcessedSpawners % 1000 == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Processed %d spawners so far..."), ProcessedSpawners);
            }
            
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
                    
                    // Extract class information for duplicate detection
                    FString DetectedClassName = TEXT("Unknown");
                    if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                    {
                        if (UFunction* Function = FunctionNode->GetTargetFunction())
                        {
                            if (UClass* OwnerClass = Function->GetOwnerClass())
                            {
                                DetectedClassName = OwnerClass->GetName();
                            }
                        }
                    }
                    
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
                        // We normalize both the search name and the function name to handle UE prefixes (K2_, BP_, etc.)
                        bool bFunctionNameMatches = true;
                        if (!ClassName.IsEmpty() && !FunctionNameFromNode.IsEmpty())
                        {
                            // Normalize the actual function name from the node
                            FString NormalizedFunctionName = NormalizeFunctionName(FunctionNameFromNode);
                            
                            // Check if the normalized function name matches any of our search names
                            bFunctionNameMatches = false;
                            for (const FString& SearchName : SearchNames)
                            {
                                FString NormalizedSearchName = NormalizeFunctionName(SearchName);
                                
                                if (NormalizedFunctionName.Equals(NormalizedSearchName, ESearchCase::IgnoreCase))
                                {
                                    bFunctionNameMatches = true;
                                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Function name match after normalization: '%s' (%s) == '%s' (%s)"), 
                                           *FunctionNameFromNode, *NormalizedFunctionName, *SearchName, *NormalizedSearchName);
                                    break;
                                }
                            }
                            
                            if (!bFunctionNameMatches)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Skipping '%s' - function name mismatch (wanted exact match for '%s' (normalized: '%s'), got '%s' (normalized: '%s'))"), 
                                       *NodeName, *FunctionName, *NormalizeFunctionName(FunctionName), *FunctionNameFromNode, *NormalizedFunctionName);
                            }
                        }
                        
                        if (bFunctionNameMatches)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found matching spawner for '%s' -> '%s' (node class: %s, function: %s)"), 
                                   *FunctionName, *MatchedName, *NodeClass, *FunctionNameFromNode);
                            
                            // Track this match for duplicate detection
                            FString TrackingKey = FunctionNameFromNode.IsEmpty() ? NodeName : FunctionNameFromNode;
                            if (!MatchingFunctionsByClass.Contains(TrackingKey))
                            {
                                MatchingFunctionsByClass.Add(TrackingKey, TArray<FString>());
                            }
                            MatchingFunctionsByClass[TrackingKey].Add(DetectedClassName);
                            MatchingSpawners.Add(NodeSpawner);
                        }
                    }
                    // If class doesn't match, DON'T return - continue to next spawner
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Finished spawner loop. Processed %d spawners total. Found %d matching spawners."), ProcessedSpawners, MatchingSpawners.Num());
    
    // CRITICAL: Check for duplicate function names across classes
    // If we found multiple functions with the same name but ClassName was not specified, throw error
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Checking for duplicates in %d matching functions..."), MatchingFunctionsByClass.Num());
    
    if (MatchingSpawners.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found %d matching spawners, checking for duplicates..."), MatchingSpawners.Num());
        
        // Check if we have multiple classes for the same function
        bool bHasDuplicates = false;
        FString DuplicateFunctionName;
        TArray<FString> DuplicateClasses;
        
        for (const auto& Pair : MatchingFunctionsByClass)
        {
            // Remove duplicate class names from the array
            TSet<FString> UniqueClasses(Pair.Value);
            
            if (UniqueClasses.Num() > 1)
            {
                bHasDuplicates = true;
                DuplicateFunctionName = Pair.Key;
                DuplicateClasses = UniqueClasses.Array();
                break;
            }
        }
        
        if (bHasDuplicates && ClassName.IsEmpty())
        {
            // Build error message listing all available classes
            FString ErrorMessage = FString::Printf(
                TEXT("ERROR: Multiple functions found with name '%s' in different classes. You MUST specify 'class_name' parameter to disambiguate.\n\nAvailable classes:\n"),
                *DuplicateFunctionName
            );
            
            for (const FString& AvailableClass : DuplicateClasses)
            {
                ErrorMessage += FString::Printf(TEXT("  - %s\n"), *AvailableClass);
            }
            
            ErrorMessage += FString::Printf(
                TEXT("\nExample:\n  create_node_by_action_name(\n      function_name=\"%s\",\n      class_name=\"%s\",  # ← REQUIRED!\n      ...\n  )"),
                *FunctionName,
                *DuplicateClasses[0]
            );
            
            UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
            
            // Set error message if caller wants it
            if (OutErrorMessage)
            {
                *OutErrorMessage = ErrorMessage;
            }
            
            // Don't create any node - return false and let the caller handle the error
            return false;
        }
        
        // If no duplicates, or ClassName was specified and matched, create the first matching node
        // (by this point, if ClassName was specified, only matching spawners remain in the list)
        if (MatchingSpawners.Num() > 0)
        {
            const UBlueprintNodeSpawner* SelectedSpawner = MatchingSpawners[0];
            
            // Get info from the selected spawner for logging
            if (UEdGraphNode* TemplateNode = SelectedSpawner->GetTemplateNode())
            {
                FString NodeName = TEXT("");
                FString NodeClass = TemplateNode->GetClass()->GetName();
                
                if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
                {
                    NodeName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                }
                
                UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Creating node using selected spawner (name: '%s', class: '%s')"), 
                       *NodeName, *NodeClass);
                
                // Create the node using the spawner
                NewNode = SelectedSpawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D(PositionX, PositionY));
                if (NewNode)
                {
                    NodeTitle = NodeName.IsEmpty() ? NodeClass : NodeName;
                    NodeType = NodeClass;
                    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);
                    return true;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: No matching spawner found for '%s' (tried %d variations)"), *FunctionName, SearchNames.Num());
    UE_LOG(LogTemp, Warning, TEXT("=== TryCreateNodeUsingBlueprintActionDatabase END (FAILED) ==="));
    return false;
}
