// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActionSpawnerMatcher.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "NodeCreationHelpers.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/Blueprint.h"
#include "Animation/AnimBlueprint.h"

const TMap<FString, TArray<FString>>& FActionSpawnerMatcher::GetOperationAliases()
{
    static TMap<FString, TArray<FString>> OperationAliases;
    static bool bInitialized = false;

    if (!bInitialized)
    {
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

        bInitialized = true;
    }

    return OperationAliases;
}

FString FActionSpawnerMatcher::NormalizeFunctionName(const FString& Name)
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

bool FActionSpawnerMatcher::ClassNameMatches(const FString& ActualClassName, const FString& ExpectedClassName)
{
    // Try exact match
    if (ActualClassName.Equals(ExpectedClassName, ESearchCase::IgnoreCase))
    {
        return true;
    }

    // Try without U/A prefix
    if (ActualClassName.Len() > 1 &&
        (ActualClassName[0] == 'U' || ActualClassName[0] == 'A') &&
        ActualClassName.Mid(1).Equals(ExpectedClassName, ESearchCase::IgnoreCase))
    {
        return true;
    }

    // Try with U prefix added to ExpectedClassName
    if (!ExpectedClassName.StartsWith(TEXT("U")) && !ExpectedClassName.StartsWith(TEXT("A")) &&
        ActualClassName.Equals(FString::Printf(TEXT("U%s"), *ExpectedClassName), ESearchCase::IgnoreCase))
    {
        return true;
    }

    // Try common library name mappings
    if (ExpectedClassName.Equals(TEXT("GameplayStatics"), ESearchCase::IgnoreCase) &&
        ActualClassName.Equals(TEXT("UGameplayStatics"), ESearchCase::IgnoreCase))
    {
        return true;
    }
    if (ExpectedClassName.Equals(TEXT("KismetMathLibrary"), ESearchCase::IgnoreCase) &&
        ActualClassName.Equals(TEXT("UKismetMathLibrary"), ESearchCase::IgnoreCase))
    {
        return true;
    }
    if (ExpectedClassName.Equals(TEXT("KismetSystemLibrary"), ESearchCase::IgnoreCase) &&
        ActualClassName.Equals(TEXT("UKismetSystemLibrary"), ESearchCase::IgnoreCase))
    {
        return true;
    }

    return false;
}

TArray<FString> FActionSpawnerMatcher::BuildSearchNames(const FString& FunctionName)
{
    TArray<FString> SearchNames;
    SearchNames.Add(FunctionName);

    // Add CamelCase to Title Case conversion (e.g., "GetActorLocation" -> "Get Actor Location")
    FString TitleCaseVersion = NodeCreationHelpers::ConvertCamelCaseToTitleCase(FunctionName);
    if (!TitleCaseVersion.Equals(FunctionName, ESearchCase::IgnoreCase))
    {
        SearchNames.Add(TitleCaseVersion);
        UE_LOG(LogTemp, Verbose, TEXT("BuildSearchNames: Added Title Case variation: '%s' -> '%s'"), *FunctionName, *TitleCaseVersion);
    }

    // Add aliases if this is a known operation
    const TMap<FString, TArray<FString>>& Aliases = GetOperationAliases();
    if (Aliases.Contains(FunctionName))
    {
        SearchNames.Append(Aliases[FunctionName]);
    }

    // Also try some common variations
    SearchNames.Add(FString::Printf(TEXT("%s_FloatFloat"), *FunctionName));
    SearchNames.Add(FString::Printf(TEXT("%s_IntInt"), *FunctionName));
    SearchNames.Add(FString::Printf(TEXT("%s_DoubleDouble"), *FunctionName));

    return SearchNames;
}

void FActionSpawnerMatcher::FindMatchingSpawners(
    const TArray<FString>& SearchNames,
    const FString& ClassName,
    TArray<FMatchedSpawnerInfo>& OutMatchedSpawners,
    TMap<FString, TArray<FString>>& OutMatchingFunctionsByClass
)
{
    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();

    UE_LOG(LogTemp, Warning, TEXT("FindMatchingSpawners: Found %d action categories, searching for %d name variations"), ActionRegistry.Num(), SearchNames.Num());

    int32 ProcessedSpawners = 0;

    for (const auto& ActionPair : ActionRegistry)
    {
        for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
        {
            ProcessedSpawners++;
            if (ProcessedSpawners % 1000 == 0)
            {
                UE_LOG(LogTemp, Verbose, TEXT("FindMatchingSpawners: Processed %d spawners so far..."), ProcessedSpawners);
            }

            if (!NodeSpawner || !IsValid(NodeSpawner))
            {
                continue;
            }

            UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();
            if (!TemplateNode)
            {
                continue;
            }

            // Try to match based on node type and function name
            FString NodeName = TEXT("");
            FString NodeClass = TemplateNode->GetClass()->GetName();
            FString FunctionNameFromNode = TEXT("");
            FString DetectedClassName = TEXT("Unknown");

            // Get human-readable node name
            if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
            {
                NodeName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                if (NodeName.IsEmpty())
                {
                    NodeName = K2Node->GetClass()->GetName();
                }

                // For function calls, get the function name and class
                if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                {
                    if (UFunction* Function = FunctionNode->GetTargetFunction())
                    {
                        FunctionNameFromNode = Function->GetName();
                        if (NodeName.IsEmpty() || NodeName == NodeClass)
                        {
                            NodeName = FunctionNameFromNode;
                        }
                        if (UClass* OwnerClass = Function->GetOwnerClass())
                        {
                            DetectedClassName = OwnerClass->GetName();
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
            bool bExactMatch = false;
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
                if (!bFoundMatch && NodeName.Contains(SearchName, ESearchCase::IgnoreCase))
                {
                    bFoundMatch = true;
                    bExactMatch = false;
                    MatchedName = SearchName;
                    // Don't break - keep searching for exact match
                }
            }

            if (!bFoundMatch)
            {
                continue;
            }

            // When ClassName is NOT specified, prefer exact function name matches
            if (ClassName.IsEmpty() && !bExactMatch)
            {
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
                if (!bIsExactFunctionMatch)
                {
                    continue; // Skip partial matches when looking for exact
                }
            }

            // Check class name filter
            bool bClassMatches = true;
            if (!ClassName.IsEmpty())
            {
                bClassMatches = false;

                if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
                {
                    if (UFunction* Function = FunctionNode->GetTargetFunction())
                    {
                        if (UClass* OwnerClass = Function->GetOwnerClass())
                        {
                            bClassMatches = ClassNameMatches(OwnerClass->GetName(), ClassName);
                        }
                    }
                }
            }

            if (!bClassMatches)
            {
                continue;
            }

            // Check exact function name match when class is specified
            bool bFunctionNameMatches = true;
            if (!ClassName.IsEmpty() && !FunctionNameFromNode.IsEmpty())
            {
                FString NormalizedFunctionName = NormalizeFunctionName(FunctionNameFromNode);
                bFunctionNameMatches = false;

                for (const FString& SearchName : SearchNames)
                {
                    FString NormalizedSearchName = NormalizeFunctionName(SearchName);
                    if (NormalizedFunctionName.Equals(NormalizedSearchName, ESearchCase::IgnoreCase))
                    {
                        bFunctionNameMatches = true;
                        break;
                    }
                }
            }

            if (!bFunctionNameMatches)
            {
                continue;
            }

            // Track this match
            FMatchedSpawnerInfo Info;
            Info.Spawner = NodeSpawner;
            Info.DetectedClassName = DetectedClassName;
            Info.FunctionName = FunctionNameFromNode.IsEmpty() ? NodeName : FunctionNameFromNode;
            Info.NodeName = NodeName;
            Info.bExactMatch = bExactMatch;
            OutMatchedSpawners.Add(Info);

            // Track for duplicate detection
            FString TrackingKey = Info.FunctionName;
            if (!OutMatchingFunctionsByClass.Contains(TrackingKey))
            {
                OutMatchingFunctionsByClass.Add(TrackingKey, TArray<FString>());
            }
            OutMatchingFunctionsByClass[TrackingKey].Add(DetectedClassName);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("FindMatchingSpawners: Finished. Processed %d spawners, found %d matches."), ProcessedSpawners, OutMatchedSpawners.Num());
}

bool FActionSpawnerMatcher::HasUnresolvedDuplicates(
    const TMap<FString, TArray<FString>>& MatchingFunctionsByClass,
    const FString& ClassName,
    const FString& FunctionName,
    FString& OutErrorMessage
)
{
    if (!ClassName.IsEmpty())
    {
        return false; // Class was specified, no ambiguity
    }

    for (const auto& Pair : MatchingFunctionsByClass)
    {
        TSet<FString> UniqueClasses(Pair.Value);

        if (UniqueClasses.Num() > 1)
        {
            // Build error message listing all available classes
            OutErrorMessage = FString::Printf(
                TEXT("ERROR: Multiple functions found with name '%s' in different classes. You MUST specify 'class_name' parameter to disambiguate.\n\nAvailable classes:\n"),
                *Pair.Key
            );

            for (const FString& AvailableClass : UniqueClasses)
            {
                OutErrorMessage += FString::Printf(TEXT("  - %s\n"), *AvailableClass);
            }

            TArray<FString> ClassArray = UniqueClasses.Array();
            OutErrorMessage += FString::Printf(
                TEXT("\nExample:\n  create_node_by_action_name(\n      function_name=\"%s\",\n      class_name=\"%s\",  # <- REQUIRED!\n      ...\n  )"),
                *FunctionName,
                ClassArray.Num() > 0 ? *ClassArray[0] : TEXT("ClassName")
            );

            return true;
        }
    }

    return false;
}

bool FActionSpawnerMatcher::RequiresAnimBlueprint(const UBlueprintNodeSpawner* Spawner)
{
    if (!Spawner)
    {
        return false;
    }

    // CHECK 1: Examine the spawner's template node class
    if (UEdGraphNode* TemplateNode = Spawner->GetTemplateNode())
    {
        UClass* NodeClass = TemplateNode->GetClass();
        if (NodeClass)
        {
            FString NodeClassName = NodeClass->GetName();
            FString NodeClassPath = NodeClass->GetPathName();

            if (NodeClassName.Contains(TEXT("AnimGraph")) ||
                NodeClassName.Contains(TEXT("AnimNode")) ||
                NodeClassName.StartsWith(TEXT("UAnimGraphNode")) ||
                NodeClassPath.Contains(TEXT("/AnimGraph/")) ||
                NodeClassPath.Contains(TEXT("AnimGraphRuntime")))
            {
                return true;
            }

            // Check the node class's outer chain for AnimGraph module
            for (UObject* Outer = NodeClass->GetOuter(); Outer; Outer = Outer->GetOuter())
            {
                FString OuterName = Outer->GetName();
                if (OuterName.Contains(TEXT("AnimGraph")) || OuterName.Contains(TEXT("AnimNode")))
                {
                    return true;
                }
            }
        }

        // CHECK 4: For function call nodes, check if the function's owner class is animation-related
        if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(TemplateNode))
        {
            if (UFunction* Function = FunctionNode->GetTargetFunction())
            {
                UClass* OwnerClass = Function->GetOwnerClass();
                if (OwnerClass)
                {
                    FString OwnerClassName = OwnerClass->GetName();
                    FString OwnerClassPath = OwnerClass->GetPathName();

                    if (OwnerClassName.Contains(TEXT("AnimInstance")) ||
                        OwnerClassName.Contains(TEXT("AnimBlueprint")) ||
                        OwnerClassName.Contains(TEXT("AnimGraph")) ||
                        OwnerClassName.Contains(TEXT("AnimNode")) ||
                        OwnerClassName.Contains(TEXT("AnimSequence")) ||
                        OwnerClassPath.Contains(TEXT("/AnimGraph/")) ||
                        OwnerClassPath.Contains(TEXT("AnimGraphRuntime")))
                    {
                        return true;
                    }
                }

                FString FunctionPath = Function->GetPathName();
                if (FunctionPath.Contains(TEXT("AnimGraph")) || FunctionPath.Contains(TEXT("AnimNode")))
                {
                    return true;
                }
            }
        }
    }

    // CHECK 2: Examine the spawner's outer object (ActionKey)
    UObject* ActionOuter = Spawner->GetOuter();
    if (ActionOuter)
    {
        if (UClass* OuterClass = Cast<UClass>(ActionOuter))
        {
            if (OuterClass->IsChildOf(UAnimBlueprint::StaticClass()) ||
                OuterClass->GetName().Contains(TEXT("AnimGraph")) ||
                OuterClass->GetName().Contains(TEXT("AnimNode")))
            {
                return true;
            }
        }
        else if (Cast<UAnimBlueprint>(ActionOuter))
        {
            return true;
        }
        else
        {
            FString OuterClassName = ActionOuter->GetClass()->GetName();
            if (OuterClassName.Contains(TEXT("AnimGraph")) || OuterClassName.Contains(TEXT("AnimNode")))
            {
                return true;
            }
        }
    }

    // CHECK 3: Check the spawner class itself
    FString SpawnerClassName = Spawner->GetClass()->GetName();
    if (SpawnerClassName.Contains(TEXT("AnimGraph")) || SpawnerClassName.Contains(TEXT("AnimNode")))
    {
        return true;
    }

    return false;
}

const UBlueprintNodeSpawner* FActionSpawnerMatcher::SelectCompatibleSpawner(
    const TArray<FMatchedSpawnerInfo>& MatchedSpawners,
    UBlueprint* TargetBlueprint
)
{
    bool bIsAnimBlueprint = TargetBlueprint && TargetBlueprint->IsA<UAnimBlueprint>();

    for (const FMatchedSpawnerInfo& Info : MatchedSpawners)
    {
        if (!Info.Spawner)
        {
            continue;
        }

        bool bRequiresAnimBlueprint = RequiresAnimBlueprint(Info.Spawner);

        // Skip AnimBlueprint-specific spawners if target is not an AnimBlueprint
        if (bRequiresAnimBlueprint && !bIsAnimBlueprint)
        {
            UE_LOG(LogTemp, Warning, TEXT("SelectCompatibleSpawner: Skipping spawner (requires AnimBlueprint, target is %s)"),
                   TargetBlueprint ? *TargetBlueprint->GetClass()->GetName() : TEXT("NULL"));
            continue;
        }

        // This spawner is compatible
        return Info.Spawner;
    }

    UE_LOG(LogTemp, Warning, TEXT("SelectCompatibleSpawner: No compatible spawner found for Blueprint type '%s'"),
           TargetBlueprint ? *TargetBlueprint->GetClass()->GetName() : TEXT("NULL"));
    return nullptr;
}

FString FActionSpawnerMatcher::BuildNotFoundErrorMessage(
    const FString& FunctionName,
    const TArray<FString>& SearchNames
)
{
    // Search for similar function names to suggest alternatives
    TArray<FString> Suggestions;
    const int32 MaxSuggestions = 5;

    FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
    FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();

    for (const auto& ActionPair : ActionRegistry)
    {
        for (const UBlueprintNodeSpawner* NodeSpawner : ActionPair.Value)
        {
            if (Suggestions.Num() >= MaxSuggestions)
            {
                break;
            }

            if (!NodeSpawner || !IsValid(NodeSpawner))
            {
                continue;
            }

            UEdGraphNode* TemplateNode = NodeSpawner->GetTemplateNode();
            if (!TemplateNode)
            {
                continue;
            }

            FString NodeName = TEXT("");
            FString FunctionNameFromNode = TEXT("");
            FString OwnerClassName = TEXT("");

            if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
            {
                NodeName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();

                if (UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(K2Node))
                {
                    if (UFunction* Function = FunctionNode->GetTargetFunction())
                    {
                        FunctionNameFromNode = Function->GetName();
                        if (UClass* OwnerClass = Function->GetOwnerClass())
                        {
                            OwnerClassName = OwnerClass->GetName();
                        }
                    }
                }
            }

            // Check if this function name contains our search term (partial match)
            FString NameToCheck = FunctionNameFromNode.IsEmpty() ? NodeName : FunctionNameFromNode;
            if (NameToCheck.Contains(FunctionName, ESearchCase::IgnoreCase))
            {
                FString Suggestion;
                if (!OwnerClassName.IsEmpty())
                {
                    Suggestion = FString::Printf(TEXT("%s (from %s)"), *NameToCheck, *OwnerClassName);
                }
                else
                {
                    Suggestion = NameToCheck;
                }

                if (!Suggestions.Contains(Suggestion))
                {
                    Suggestions.Add(Suggestion);
                }
            }
        }

        if (Suggestions.Num() >= MaxSuggestions)
        {
            break;
        }
    }

    // Build error message
    FString ErrorMessage = FString::Printf(
        TEXT("Function '%s' not found in Blueprint Action Database."),
        *FunctionName
    );

    if (Suggestions.Num() > 0)
    {
        ErrorMessage += TEXT("\n\nDid you mean one of these?\n");
        for (const FString& Suggestion : Suggestions)
        {
            ErrorMessage += FString::Printf(TEXT("  - %s\n"), *Suggestion);
        }
    }

    ErrorMessage += TEXT("\nTip: Use search_blueprint_actions() to discover available function names.");

    return ErrorMessage;
}
