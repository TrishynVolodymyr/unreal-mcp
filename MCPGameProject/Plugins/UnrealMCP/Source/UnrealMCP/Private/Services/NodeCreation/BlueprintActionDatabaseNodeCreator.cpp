// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintActionDatabaseNodeCreator.h"
#include "EnhancedInputActionNodeCreator.h"
#include "ArrayNodeCreator.h"
#include "SpawnActorNodeCreator.h"
#include "ActionSpawnerMatcher.h"
#include "VariableNodePostProcessor.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"

bool FBlueprintActionDatabaseNodeCreator::TryCreateNodeUsingBlueprintActionDatabase(
    const FString& FunctionName,
    const FString& ClassName,
    UEdGraph* EventGraph,
    float PositionX,
    float PositionY,
    UEdGraphNode*& NewNode,
    FString& NodeTitle,
    FString& NodeType,
    FString* OutErrorMessage,
    FString* OutWarningMessage
)
{
    UE_LOG(LogTemp, Warning, TEXT("=== TryCreateNodeUsingBlueprintActionDatabase START ==="));
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Attempting dynamic creation for '%s' with class '%s'"), *FunctionName, *ClassName);

    // Special handling for Enhanced Input Actions
    if (ClassName.Equals(TEXT("EnhancedInputAction"), ESearchCase::IgnoreCase))
    {
        return FEnhancedInputActionNodeCreator::TryCreateEnhancedInputActionNode(
            FunctionName,
            EventGraph,
            PositionX,
            PositionY,
            NewNode,
            NodeTitle,
            NodeType
        );
    }

    // Special handling for Array GET nodes
    if (FArrayNodeCreator::IsArrayGetOperation(FunctionName, ClassName))
    {
        return FArrayNodeCreator::TryCreateArrayGetNode(
            EventGraph,
            PositionX,
            PositionY,
            NewNode,
            NodeTitle,
            NodeType
        );
    }

    // Special handling for Array LENGTH nodes
    if (FArrayNodeCreator::IsArrayLengthOperation(FunctionName, ClassName))
    {
        return FArrayNodeCreator::TryCreateArrayLengthNode(
            EventGraph,
            PositionX,
            PositionY,
            NewNode,
            NodeTitle,
            NodeType
        );
    }

    // Special handling for Spawn Actor from Class node
    if (FSpawnActorNodeCreator::IsSpawnActorFromClassRequest(FunctionName))
    {
        return FSpawnActorNodeCreator::TryCreateSpawnActorFromClassNode(
            EventGraph,
            PositionX,
            PositionY,
            NewNode,
            NodeTitle,
            NodeType
        );
    }

    // Build list of function names to search for (including aliases and variations)
    TArray<FString> SearchNames = FActionSpawnerMatcher::BuildSearchNames(FunctionName);
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Searching for %d name variations"), SearchNames.Num());

    // Find matching spawners
    TArray<FMatchedSpawnerInfo> MatchedSpawners;
    TMap<FString, TArray<FString>> MatchingFunctionsByClass;
    FActionSpawnerMatcher::FindMatchingSpawners(SearchNames, ClassName, MatchedSpawners, MatchingFunctionsByClass);

    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Found %d matching spawners"), MatchedSpawners.Num());

    // Check for unresolved duplicates
    FString DuplicateError;
    if (FActionSpawnerMatcher::HasUnresolvedDuplicates(MatchingFunctionsByClass, ClassName, FunctionName, DuplicateError))
    {
        UE_LOG(LogTemp, Error, TEXT("%s"), *DuplicateError);
        if (OutErrorMessage)
        {
            *OutErrorMessage = DuplicateError;
        }
        return false;
    }

    // Select a compatible spawner
    if (MatchedSpawners.Num() > 0)
    {
        UBlueprint* TargetBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(EventGraph);
        const UBlueprintNodeSpawner* SelectedSpawner = FActionSpawnerMatcher::SelectCompatibleSpawner(MatchedSpawners, TargetBlueprint);

        if (!SelectedSpawner)
        {
            UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: No compatible spawner found"));
            return false;
        }

        // Get info from the selected spawner for logging
        FString NodeName = TEXT("");
        FString NodeClass = TEXT("");

        if (UEdGraphNode* TemplateNode = SelectedSpawner->GetTemplateNode())
        {
            NodeClass = TemplateNode->GetClass()->GetName();
            if (UK2Node* K2Node = Cast<UK2Node>(TemplateNode))
            {
                NodeName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Creating node using selected spawner (name: '%s', class: '%s')"),
               *NodeName, *NodeClass);

        // Create the node using the spawner
        NewNode = SelectedSpawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D(PositionX, PositionY));
        if (NewNode)
        {
            // Apply post-creation fixes for variable nodes
            if (UK2Node_VariableGet* GetNode = Cast<UK2Node_VariableGet>(NewNode))
            {
                FVariableNodePostProcessor::ProcessVariableGetNode(GetNode, ClassName, EventGraph);
            }
            else if (UK2Node_VariableSet* SetNode = Cast<UK2Node_VariableSet>(NewNode))
            {
                FVariableNodePostProcessor::ProcessVariableSetNode(SetNode, ClassName, EventGraph);
            }

            // Check for WidgetBlueprintLibrary warnings
            if (OutWarningMessage && Cast<UK2Node_CallFunction>(NewNode))
            {
                UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(NewNode);
                if (UFunction* Function = FunctionNode->GetTargetFunction())
                {
                    UClass* OwnerClass = Function->GetOwnerClass();
                    if (OwnerClass && OwnerClass->GetName().Contains(TEXT("WidgetBlueprintLibrary")))
                    {
                        // Check if we're in a Widget Blueprint
                        UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(EventGraph);
                        bool bIsWidgetBlueprint = Blueprint && Blueprint->IsA<UWidgetBlueprint>();

                        if (!bIsWidgetBlueprint)
                        {
                            *OutWarningMessage = FString::Printf(
                                TEXT("WARNING: '%s' is from WidgetBlueprintLibrary. When used in non-Widget Blueprints, this node has a hidden 'self' pin (WorldContext) that must also be connected. ")
                                TEXT("Connect the PlayerController to BOTH the visible parameter pin AND the hidden 'self' pin, or consider using PlayerController's native SetInputMode functions instead which don't have this issue."),
                                *Function->GetName()
                            );
                            UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: %s"), **OutWarningMessage);
                        }
                    }
                }
            }

            NodeTitle = NodeName.IsEmpty() ? NodeClass : NodeName;
            NodeType = NodeClass;
            UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: Successfully created node '%s' of type '%s'"), *NodeTitle, *NodeType);
            return true;
        }
    }

    // No matching spawner found - build helpful error message
    UE_LOG(LogTemp, Warning, TEXT("TryCreateNodeUsingBlueprintActionDatabase: No matching spawner found for '%s' (tried %d variations)"), *FunctionName, SearchNames.Num());

    if (OutErrorMessage)
    {
        *OutErrorMessage = FActionSpawnerMatcher::BuildNotFoundErrorMessage(FunctionName, SearchNames);
    }

    UE_LOG(LogTemp, Warning, TEXT("=== TryCreateNodeUsingBlueprintActionDatabase END (FAILED) ==="));
    return false;
}
