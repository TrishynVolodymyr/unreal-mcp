// AnimationBlueprintStateMachineOps.cpp
// State machine operations for FAnimationBlueprintService
// This file is part of the AnimationBlueprintService implementation

#include "Services/AnimationBlueprintService.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationGraph.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/Kismet2NameValidators.h"

bool FAnimationBlueprintService::CreateStateMachine(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, FString& OutError)
{
    if (!AnimBlueprint)
    {
        OutError = TEXT("Invalid Animation Blueprint");
        return false;
    }

    // Find the AnimGraph
    UAnimationGraph* AnimGraph = FindAnimGraph(AnimBlueprint);
    if (!AnimGraph)
    {
        OutError = TEXT("Could not find AnimGraph in Animation Blueprint");
        return false;
    }

    // Create the state machine node using RF_Transactional (matches editor behavior)
    // EditorStateMachineGraph must be NULL for PostPlacedNewNode to work
    UAnimGraphNode_StateMachine* StateMachineNode = NewObject<UAnimGraphNode_StateMachine>(AnimGraph, NAME_None, RF_Transactional);
    if (!StateMachineNode)
    {
        OutError = TEXT("Failed to create state machine node");
        return false;
    }

    // Add the node to the AnimGraph FIRST (before PostPlacedNewNode)
    // PostPlacedNewNode uses GetGraph() which requires the node to be in a graph
    AnimGraph->AddNode(StateMachineNode, false, false);

    // Let PostPlacedNewNode do its work - this will:
    // 1. Create EditorStateMachineGraph using CreateNewGraph
    // 2. Set OwnerAnimGraphNode
    // 3. Create name validator and rename the graph
    // 4. Call Schema->CreateDefaultNodesForGraph()
    // 5. Add graph to SubGraphs
    // 6. Call EnsureBindingsArePresent() (from base class)
    // 7. Call UAnimBlueprintExtension::RequestExtensionsForNode() (from base class)
    StateMachineNode->PostPlacedNewNode();

    // Allocate pins AFTER PostPlacedNewNode (creates output pose pin)
    StateMachineNode->AllocateDefaultPins();

    // Now rename the state machine graph to our desired name
    if (StateMachineNode->EditorStateMachineGraph)
    {
        TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(StateMachineNode);
        FBlueprintEditorUtils::RenameGraphWithSuggestion(StateMachineNode->EditorStateMachineGraph, NameValidator, StateMachineName);
    }
    else
    {
        OutError = TEXT("PostPlacedNewNode failed to create state machine graph");
        return false;
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::CreateStateMachine: Created state machine '%s'"), *StateMachineName);
    return true;
}

bool FAnimationBlueprintService::AddStateToStateMachine(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, const FAnimStateParams& Params, FString& OutError)
{
    if (!AnimBlueprint)
    {
        OutError = TEXT("Invalid Animation Blueprint");
        return false;
    }

    // Find the state machine
    UAnimGraphNode_StateMachine* StateMachineNode = FindStateMachineNode(AnimBlueprint, StateMachineName);
    if (!StateMachineNode || !StateMachineNode->EditorStateMachineGraph)
    {
        OutError = FString::Printf(TEXT("Could not find state machine '%s'"), *StateMachineName);
        return false;
    }

    UAnimationStateMachineGraph* StateMachineGraph = StateMachineNode->EditorStateMachineGraph;

    // Create the state node (BoundGraph must be NULL for PostPlacedNewNode to work)
    UAnimStateNode* StateNode = NewObject<UAnimStateNode>(StateMachineGraph, NAME_None, RF_Transactional);
    if (!StateNode)
    {
        OutError = TEXT("Failed to create state node");
        return false;
    }

    // Set node position BEFORE adding to graph (affects layout)
    StateNode->NodePosX = static_cast<int32>(Params.NodePosition.X);
    StateNode->NodePosY = static_cast<int32>(Params.NodePosition.Y);

    // Add to graph FIRST (PostPlacedNewNode uses GetGraph())
    StateMachineGraph->AddNode(StateNode, false, false);

    // Let PostPlacedNewNode do its work - this will:
    // 1. Create BoundGraph using CreateNewGraph
    // 2. Rename with validator
    // 3. Call CreateDefaultNodesForGraph (creates MyResultNode)
    // 4. Add BoundGraph to SubGraphs
    StateNode->PostPlacedNewNode();

    // Rename the bound graph to our desired state name
    if (StateNode->BoundGraph)
    {
        TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(StateNode);
        FBlueprintEditorUtils::RenameGraphWithSuggestion(StateNode->BoundGraph, NameValidator, Params.StateName);
    }
    else
    {
        OutError = TEXT("PostPlacedNewNode failed to create state BoundGraph");
        return false;
    }

    // Allocate pins AFTER PostPlacedNewNode
    StateNode->AllocateDefaultPins();

    // Notify the graph that it changed so positions are properly applied
    StateMachineGraph->NotifyGraphChanged();

    // If there's an animation asset path, load it and create a sequence player node
    if (!Params.AnimationAssetPath.IsEmpty())
    {
        UAnimationStateGraph* StateGraph = Cast<UAnimationStateGraph>(StateNode->BoundGraph);
        if (StateGraph && StateGraph->MyResultNode)
        {
            // Load the animation asset
            UAnimationAsset* AnimAsset = LoadObject<UAnimationAsset>(nullptr, *Params.AnimationAssetPath);
            if (AnimAsset)
            {
                // Create sequence player node using FGraphNodeCreator pattern
                FGraphNodeCreator<UAnimGraphNode_SequencePlayer> SequencePlayerCreator(*StateGraph);
                UAnimGraphNode_SequencePlayer* SequencePlayer = SequencePlayerCreator.CreateNode();

                if (SequencePlayer)
                {
                    // Set the animation asset
                    SequencePlayer->SetAnimationAsset(AnimAsset);

                    // Finalize the node creation
                    SequencePlayerCreator.Finalize();

                    // Position the sequence player to the left of the result node
                    SequencePlayer->NodePosX = StateGraph->MyResultNode->NodePosX - 400;
                    SequencePlayer->NodePosY = StateGraph->MyResultNode->NodePosY;

                    // Connect the sequence player's Pose output to the result node's Result input
                    UEdGraphPin* OutputPin = SequencePlayer->FindPin(TEXT("Pose"), EGPD_Output);
                    UEdGraphPin* InputPin = StateGraph->MyResultNode->FindPin(TEXT("Result"), EGPD_Input);

                    if (OutputPin && InputPin)
                    {
                        OutputPin->MakeLinkTo(InputPin);
                        UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddStateToStateMachine: Connected animation '%s' to state '%s'"),
                            *AnimAsset->GetName(), *Params.StateName);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("FAnimationBlueprintService::AddStateToStateMachine: Could not find pins to connect animation. OutputPin: %s, InputPin: %s"),
                            OutputPin ? TEXT("Found") : TEXT("Not Found"), InputPin ? TEXT("Found") : TEXT("Not Found"));
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("FAnimationBlueprintService::AddStateToStateMachine: Could not load animation asset at '%s'"), *Params.AnimationAssetPath);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FAnimationBlueprintService::AddStateToStateMachine: State graph or result node not available for animation binding"));
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddStateToStateMachine: Added state '%s' to state machine '%s' at position (%d, %d)"),
        *Params.StateName, *StateMachineName, StateNode->NodePosX, StateNode->NodePosY);
    return true;
}

bool FAnimationBlueprintService::AddStateTransition(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, const FAnimTransitionParams& Params, FString& OutError)
{
    if (!AnimBlueprint)
    {
        OutError = TEXT("Invalid Animation Blueprint");
        return false;
    }

    // Find the state machine
    UAnimGraphNode_StateMachine* StateMachineNode = FindStateMachineNode(AnimBlueprint, StateMachineName);
    if (!StateMachineNode || !StateMachineNode->EditorStateMachineGraph)
    {
        OutError = FString::Printf(TEXT("Could not find state machine '%s'"), *StateMachineName);
        return false;
    }

    UAnimationStateMachineGraph* StateMachineGraph = StateMachineNode->EditorStateMachineGraph;

    // Find source and destination states
    UAnimStateNode* FromState = FindStateNode(StateMachineGraph, Params.FromStateName);
    UAnimStateNode* ToState = FindStateNode(StateMachineGraph, Params.ToStateName);

    if (!FromState)
    {
        OutError = FString::Printf(TEXT("Could not find source state '%s'"), *Params.FromStateName);
        return false;
    }

    if (!ToState)
    {
        OutError = FString::Printf(TEXT("Could not find destination state '%s'"), *Params.ToStateName);
        return false;
    }

    // Create the transition node with RF_Transactional flag
    UAnimStateTransitionNode* TransitionNode = NewObject<UAnimStateTransitionNode>(StateMachineGraph, NAME_None, RF_Transactional);
    if (!TransitionNode)
    {
        OutError = TEXT("Failed to create transition node");
        return false;
    }

    // Add to graph FIRST (PostPlacedNewNode uses GetGraph())
    StateMachineGraph->AddNode(TransitionNode, false, false);

    // Let PostPlacedNewNode create the BoundGraph (transition rule graph)
    // This creates UAnimationTransitionGraph with proper schema and default nodes
    TransitionNode->PostPlacedNewNode();

    // Allocate pins AFTER PostPlacedNewNode
    TransitionNode->AllocateDefaultPins();

    // Set up the transition properties
    TransitionNode->CrossfadeDuration = Params.BlendDuration;

    // Handle transition rule type
    FString RuleType = Params.TransitionRuleType.ToLower();

    if (RuleType == TEXT("timeremaining"))
    {
        // TimeRemaining: Use automatic rule based on sequence player's remaining time
        TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState = true;
        // Negative value means trigger 'CrossfadeDuration' seconds before the end
        // so a standard blend would finish just as the asset player ends
        TransitionNode->AutomaticRuleTriggerTime = -1.0f;
        TransitionNode->LogicType = ETransitionLogicType::TLT_StandardBlend;
        UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddStateTransition: Set TimeRemaining rule for transition"));
    }
    else if (RuleType == TEXT("inertialization"))
    {
        // Inertialization: Use inertialization blend mode
        TransitionNode->LogicType = ETransitionLogicType::TLT_Inertialization;
        TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState = false;
        UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddStateTransition: Set Inertialization rule for transition"));
    }
    else if (RuleType == TEXT("custom"))
    {
        // Custom: Use custom graph for transition logic
        TransitionNode->LogicType = ETransitionLogicType::TLT_Custom;
        TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState = false;
        UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddStateTransition: Set Custom rule for transition (requires manual graph setup)"));
    }
    else if (RuleType == TEXT("boolvariable") && !Params.ConditionVariableName.IsEmpty())
    {
        // BoolVariable: Standard blend with a condition variable
        // Note: The BoundGraph needs to be populated with the variable getter logic
        // For now, we log that this requires manual setup or future implementation
        TransitionNode->LogicType = ETransitionLogicType::TLT_StandardBlend;
        TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState = false;
        UE_LOG(LogTemp, Warning, TEXT("FAnimationBlueprintService::AddStateTransition: BoolVariable rule set, but variable logic in BoundGraph requires manual setup. Variable: %s"), *Params.ConditionVariableName);
    }
    else
    {
        // Default: CrossfadeBlend (standard blend without automatic rule)
        TransitionNode->LogicType = ETransitionLogicType::TLT_StandardBlend;
        TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState = false;
    }

    // Create connections between states through this transition
    // This properly wires the pins: FromState -> TransitionNode -> ToState
    TransitionNode->CreateConnections(FromState, ToState);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddStateTransition: Added transition from '%s' to '%s'"), *Params.FromStateName, *Params.ToStateName);
    return true;
}

bool FAnimationBlueprintService::GetStateMachineStates(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, TArray<FString>& OutStates)
{
    if (!AnimBlueprint)
    {
        return false;
    }

    OutStates.Empty();

    UAnimGraphNode_StateMachine* StateMachineNode = FindStateMachineNode(AnimBlueprint, StateMachineName);
    if (!StateMachineNode || !StateMachineNode->EditorStateMachineGraph)
    {
        return false;
    }

    for (UEdGraphNode* Node : StateMachineNode->EditorStateMachineGraph->Nodes)
    {
        if (UAnimStateNode* StateNode = Cast<UAnimStateNode>(Node))
        {
            OutStates.Add(StateNode->GetStateName());
        }
    }

    return true;
}
