#include "Services/AnimationBlueprintService.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimStateMachineTypes.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationGraph.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimGraphNode_LinkedAnimLayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_Root.h"
#include "AnimationStateGraph.h"
#include "AnimationStateGraphSchema.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "KismetCompiler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "EdGraph/EdGraph.h"

// Singleton instance
TUniquePtr<FAnimationBlueprintService> FAnimationBlueprintService::Instance;

// Validation for FAnimBlueprintCreationParams
bool FAnimBlueprintCreationParams::IsValid(FString& OutError) const
{
    if (Name.IsEmpty())
    {
        OutError = TEXT("Animation Blueprint name cannot be empty");
        return false;
    }

    if (SkeletonPath.IsEmpty())
    {
        OutError = TEXT("Skeleton path is required for Animation Blueprint creation");
        return false;
    }

    return true;
}

FAnimationBlueprintService& FAnimationBlueprintService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = TUniquePtr<FAnimationBlueprintService>(new FAnimationBlueprintService());
    }
    return *Instance;
}

UAnimBlueprint* FAnimationBlueprintService::CreateAnimBlueprint(const FAnimBlueprintCreationParams& Params)
{
    // Validate parameters
    FString ValidationError;
    if (!Params.IsValid(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: %s"), *ValidationError);
        return nullptr;
    }

    // Find the skeleton
    USkeleton* Skeleton = FindSkeleton(Params.SkeletonPath);
    if (!Skeleton)
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Could not find skeleton at path '%s'"), *Params.SkeletonPath);
        return nullptr;
    }

    // Resolve parent class
    UClass* ParentClass = ResolveAnimInstanceClass(Params.ParentClassName);
    if (!ParentClass)
    {
        ParentClass = UAnimInstance::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Could not resolve parent class '%s', using UAnimInstance"), *Params.ParentClassName);
    }

    // Ensure parent class is an AnimInstance
    if (!ParentClass->IsChildOf(UAnimInstance::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Parent class '%s' is not derived from UAnimInstance"), *ParentClass->GetName());
        return nullptr;
    }

    // Build the package path
    FString PackagePath = Params.FolderPath.IsEmpty() ? TEXT("/Game/Animations") : Params.FolderPath;
    if (!PackagePath.StartsWith(TEXT("/")))
    {
        PackagePath = TEXT("/Game/") + PackagePath;
    }

    FString PackageName = PackagePath / Params.Name;

    // Create the package
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Failed to create package '%s'"), *PackageName);
        return nullptr;
    }

    // Create the Animation Blueprint
    UBlueprint* CreatedBlueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        *Params.Name,
        BPTYPE_Normal,
        UAnimBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass()
    );

    UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(CreatedBlueprint);
    if (!AnimBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Failed to create Animation Blueprint"));
        return nullptr;
    }

    // Set the target skeleton
    AnimBlueprint->TargetSkeleton = Skeleton;

    // Mark package dirty and save
    Package->MarkPackageDirty();

    // Compile if requested
    if (Params.bCompileOnCreation)
    {
        FString CompileError;
        if (!CompileAnimBlueprint(AnimBlueprint, CompileError))
        {
            UE_LOG(LogTemp, Warning, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Compilation warning - %s"), *CompileError);
        }
    }

    // Save the asset
    FAssetRegistryModule::AssetCreated(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::CreateAnimBlueprint: Successfully created Animation Blueprint '%s'"), *Params.Name);
    return AnimBlueprint;
}

UAnimBlueprint* FAnimationBlueprintService::FindAnimBlueprint(const FString& AnimBlueprintName)
{
    if (AnimBlueprintName.IsEmpty())
    {
        return nullptr;
    }

    // If it's a full path, try to load directly
    if (AnimBlueprintName.StartsWith(TEXT("/Game/")) || AnimBlueprintName.StartsWith(TEXT("/Script/")))
    {
        return LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintName);
    }

    // Search the asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(UAnimBlueprint::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString() == AnimBlueprintName)
        {
            return Cast<UAnimBlueprint>(AssetData.GetAsset());
        }
    }

    return nullptr;
}

bool FAnimationBlueprintService::CompileAnimBlueprint(UAnimBlueprint* AnimBlueprint, FString& OutError)
{
    if (!AnimBlueprint)
    {
        OutError = TEXT("Invalid Animation Blueprint");
        return false;
    }

    // Compile the blueprint
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(AnimBlueprint, EBlueprintCompileOptions::None, &Results);

    // Check for errors
    if (Results.NumErrors > 0)
    {
        OutError = FString::Printf(TEXT("Compilation failed with %d errors"), Results.NumErrors);
        return false;
    }

    return true;
}

bool FAnimationBlueprintService::LinkAnimationLayer(UAnimBlueprint* AnimBlueprint, const FAnimLayerLinkParams& Params, FString& OutError)
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

    // Find the layer interface class
    UClass* InterfaceClass = FindObject<UClass>(nullptr, *Params.LayerInterfaceName);
    if (!InterfaceClass)
    {
        // Try loading it
        InterfaceClass = LoadClass<UInterface>(nullptr, *Params.LayerInterfaceName);
    }

    if (!InterfaceClass)
    {
        OutError = FString::Printf(TEXT("Could not find animation layer interface '%s'"), *Params.LayerInterfaceName);
        return false;
    }

    // Create a linked anim layer node
    UAnimGraphNode_LinkedAnimLayer* LayerNode = NewObject<UAnimGraphNode_LinkedAnimLayer>(AnimGraph);
    if (!LayerNode)
    {
        OutError = TEXT("Failed to create linked animation layer node");
        return false;
    }

    // Set the interface
    LayerNode->Node.Interface = TSubclassOf<UAnimLayerInterface>(InterfaceClass);

    // Add the node to the graph
    AnimGraph->AddNode(LayerNode, false, false);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::LinkAnimationLayer: Linked layer interface '%s'"), *Params.LayerInterfaceName);
    return true;
}

bool FAnimationBlueprintService::GetLinkedAnimationLayers(UAnimBlueprint* AnimBlueprint, TArray<FString>& OutLayers)
{
    if (!AnimBlueprint)
    {
        return false;
    }

    OutLayers.Empty();

    UAnimationGraph* AnimGraph = FindAnimGraph(AnimBlueprint);
    if (!AnimGraph)
    {
        return false;
    }

    for (UEdGraphNode* Node : AnimGraph->Nodes)
    {
        if (UAnimGraphNode_LinkedAnimLayer* LayerNode = Cast<UAnimGraphNode_LinkedAnimLayer>(Node))
        {
            if (LayerNode->Node.Interface)
            {
                OutLayers.Add(LayerNode->Node.Interface->GetName());
            }
        }
    }

    return true;
}

// State machine operations (CreateStateMachine, AddStateToStateMachine, AddStateTransition, GetStateMachineStates)
// are implemented in AnimationBlueprintStateMachineOps.cpp

bool FAnimationBlueprintService::AddAnimVariable(UAnimBlueprint* AnimBlueprint, const FString& VariableName, const FString& VariableType, const FString& DefaultValue, FString& OutError)
{
    if (!AnimBlueprint)
    {
        OutError = TEXT("Invalid Animation Blueprint");
        return false;
    }

    // Map variable type string to FEdGraphPinType
    FEdGraphPinType PinType;

    if (VariableType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (VariableType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    }
    else if (VariableType.Equals(TEXT("Int"), ESearchCase::IgnoreCase) || VariableType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (VariableType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else if (VariableType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType);
        return false;
    }

    // Add the variable using BlueprintEditorUtils
    bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(AnimBlueprint, FName(*VariableName), PinType);

    if (!bSuccess)
    {
        OutError = FString::Printf(TEXT("Failed to add variable '%s'"), *VariableName);
        return false;
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::AddAnimVariable: Added variable '%s' of type '%s'"), *VariableName, *VariableType);
    return true;
}

bool FAnimationBlueprintService::GetAnimVariables(UAnimBlueprint* AnimBlueprint, TArray<TPair<FString, FString>>& OutVariables)
{
    if (!AnimBlueprint)
    {
        return false;
    }

    OutVariables.Empty();

    for (const FBPVariableDescription& Variable : AnimBlueprint->NewVariables)
    {
        FString TypeName = Variable.VarType.PinCategory.ToString();
        OutVariables.Add(TPair<FString, FString>(Variable.VarName.ToString(), TypeName));
    }

    return true;
}

bool FAnimationBlueprintService::ConfigureAnimSlot(UAnimBlueprint* AnimBlueprint, const FString& SlotName, const FString& SlotGroupName, FString& OutError)
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

    // Create a slot node with RF_Transactional flag
    UAnimGraphNode_Slot* SlotNode = NewObject<UAnimGraphNode_Slot>(AnimGraph, NAME_None, RF_Transactional);
    if (!SlotNode)
    {
        OutError = TEXT("Failed to create slot node");
        return false;
    }

    // Set the slot name
    SlotNode->Node.SlotName = FName(*SlotName);

    // Add the node to the graph FIRST (before PostPlacedNewNode)
    AnimGraph->AddNode(SlotNode, false, false);

    // Initialize the node properly (creates internal state)
    SlotNode->PostPlacedNewNode();

    // Allocate pins AFTER PostPlacedNewNode (creates Source/Pose pins)
    SlotNode->AllocateDefaultPins();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::ConfigureAnimSlot: Configured slot '%s' in group '%s'"), *SlotName, *SlotGroupName);
    return true;
}

bool FAnimationBlueprintService::GetAnimBlueprintMetadata(UAnimBlueprint* AnimBlueprint, TSharedPtr<FJsonObject>& OutMetadata)
{
    if (!AnimBlueprint)
    {
        return false;
    }

    OutMetadata = MakeShared<FJsonObject>();

    // Basic info
    OutMetadata->SetStringField(TEXT("name"), AnimBlueprint->GetName());
    OutMetadata->SetStringField(TEXT("path"), AnimBlueprint->GetPathName());

    // Parent class
    if (AnimBlueprint->ParentClass)
    {
        OutMetadata->SetStringField(TEXT("parent_class"), AnimBlueprint->ParentClass->GetName());
    }

    // Skeleton
    if (AnimBlueprint->TargetSkeleton)
    {
        OutMetadata->SetStringField(TEXT("skeleton"), AnimBlueprint->TargetSkeleton->GetPathName());
    }

    // Variables
    TArray<TSharedPtr<FJsonValue>> VariablesArray;
    for (const FBPVariableDescription& Variable : AnimBlueprint->NewVariables)
    {
        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VariablesArray.Add(MakeShared<FJsonValueObject>(VarObj));
    }
    OutMetadata->SetArrayField(TEXT("variables"), VariablesArray);

    // State machines with enhanced details
    TArray<TSharedPtr<FJsonValue>> StateMachinesArray;
    TArray<TSharedPtr<FJsonValue>> AnimGraphNodesArray;
    bool bHasRootConnection = false;
    UAnimationGraph* AnimGraph = FindAnimGraph(AnimBlueprint);
    if (AnimGraph)
    {
        // Check for root node connection
        for (UEdGraphNode* Node : AnimGraph->Nodes)
        {
            if (UAnimGraphNode_Root* RootNode = Cast<UAnimGraphNode_Root>(Node))
            {
                // Check if root node has an input connection
                for (UEdGraphPin* Pin : RootNode->Pins)
                {
                    if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
                    {
                        bHasRootConnection = true;
                        break;
                    }
                }
            }
        }

        // Collect all AnimGraph nodes
        for (UEdGraphNode* Node : AnimGraph->Nodes)
        {
            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
            NodeObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
            NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            NodeObj->SetNumberField(TEXT("position_x"), Node->NodePosX);
            NodeObj->SetNumberField(TEXT("position_y"), Node->NodePosY);

            // Check if this node is connected to root
            bool bConnectedToRoot = false;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin->Direction == EGPD_Output)
                {
                    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                    {
                        if (LinkedPin && LinkedPin->GetOwningNode())
                        {
                            if (Cast<UAnimGraphNode_Root>(LinkedPin->GetOwningNode()))
                            {
                                bConnectedToRoot = true;
                                break;
                            }
                        }
                    }
                }
                if (bConnectedToRoot) break;
            }
            NodeObj->SetBoolField(TEXT("connected_to_root"), bConnectedToRoot);

            AnimGraphNodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));

            // Process state machines with enhanced details
            if (UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node))
            {
                TSharedPtr<FJsonObject> SMObj = MakeShared<FJsonObject>();
                FString SMName = SMNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                SMObj->SetStringField(TEXT("name"), SMName);
                SMObj->SetStringField(TEXT("node_id"), SMNode->NodeGuid.ToString());
                SMObj->SetNumberField(TEXT("position_x"), SMNode->NodePosX);
                SMObj->SetNumberField(TEXT("position_y"), SMNode->NodePosY);
                SMObj->SetBoolField(TEXT("connected_to_root"), bConnectedToRoot);

                // Get states
                TArray<FString> States;
                GetStateMachineStates(AnimBlueprint, SMName, States);

                TArray<TSharedPtr<FJsonValue>> StatesArray;
                for (const FString& State : States)
                {
                    StatesArray.Add(MakeShared<FJsonValueString>(State));
                }
                SMObj->SetArrayField(TEXT("states"), StatesArray);

                // Get transitions from the state machine graph
                TArray<TSharedPtr<FJsonValue>> TransitionsArray;
                if (SMNode->EditorStateMachineGraph)
                {
                    FString EntryStateName;
                    for (UEdGraphNode* SMGraphNode : SMNode->EditorStateMachineGraph->Nodes)
                    {
                        // Get entry state
                        if (UAnimStateEntryNode* EntryNode = Cast<UAnimStateEntryNode>(SMGraphNode))
                        {
                            // Find what the entry node connects to
                            for (UEdGraphPin* Pin : EntryNode->Pins)
                            {
                                if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
                                {
                                    if (UAnimStateNode* LinkedState = Cast<UAnimStateNode>(Pin->LinkedTo[0]->GetOwningNode()))
                                    {
                                        EntryStateName = LinkedState->GetStateName();
                                    }
                                }
                            }
                        }

                        // Get transitions
                        if (UAnimStateTransitionNode* TransNode = Cast<UAnimStateTransitionNode>(SMGraphNode))
                        {
                            TSharedPtr<FJsonObject> TransObj = MakeShared<FJsonObject>();

                            // Get from/to state names
                            FString FromStateName, ToStateName;
                            if (UAnimStateNode* PrevState = Cast<UAnimStateNode>(TransNode->GetPreviousState()))
                            {
                                FromStateName = PrevState->GetStateName();
                            }
                            if (UAnimStateNode* NextState = Cast<UAnimStateNode>(TransNode->GetNextState()))
                            {
                                ToStateName = NextState->GetStateName();
                            }

                            TransObj->SetStringField(TEXT("from_state"), FromStateName);
                            TransObj->SetStringField(TEXT("to_state"), ToStateName);
                            TransObj->SetNumberField(TEXT("blend_duration"), TransNode->CrossfadeDuration);

                            // Transition rule type
                            FString RuleType;
                            if (TransNode->bAutomaticRuleBasedOnSequencePlayerInState)
                            {
                                RuleType = TEXT("TimeRemaining");
                            }
                            else
                            {
                                switch (TransNode->LogicType)
                                {
                                    case ETransitionLogicType::TLT_StandardBlend:
                                        RuleType = TEXT("CrossfadeBlend");
                                        break;
                                    case ETransitionLogicType::TLT_Inertialization:
                                        RuleType = TEXT("Inertialization");
                                        break;
                                    case ETransitionLogicType::TLT_Custom:
                                        RuleType = TEXT("Custom");
                                        break;
                                    default:
                                        RuleType = TEXT("Unknown");
                                        break;
                                }
                            }
                            TransObj->SetStringField(TEXT("rule_type"), RuleType);

                            TransitionsArray.Add(MakeShared<FJsonValueObject>(TransObj));
                        }
                    }
                    SMObj->SetStringField(TEXT("entry_state"), EntryStateName);
                }
                SMObj->SetArrayField(TEXT("transitions"), TransitionsArray);

                StateMachinesArray.Add(MakeShared<FJsonValueObject>(SMObj));
            }
        }
    }
    OutMetadata->SetArrayField(TEXT("state_machines"), StateMachinesArray);
    OutMetadata->SetArrayField(TEXT("animgraph_nodes"), AnimGraphNodesArray);
    OutMetadata->SetBoolField(TEXT("has_root_connection"), bHasRootConnection);

    // Linked layers
    TArray<FString> Layers;
    GetLinkedAnimationLayers(AnimBlueprint, Layers);

    TArray<TSharedPtr<FJsonValue>> LayersArray;
    for (const FString& Layer : Layers)
    {
        LayersArray.Add(MakeShared<FJsonValueString>(Layer));
    }
    OutMetadata->SetArrayField(TEXT("linked_layers"), LayersArray);

    return true;
}

bool FAnimationBlueprintService::ConnectAnimGraphNodes(UAnimBlueprint* AnimBlueprint, const FString& SourceNodeName, const FString& TargetNodeName, const FString& SourcePinName, const FString& TargetPinName, FString& OutError)
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

    // Find the source node (typically a state machine)
    UEdGraphNode* SourceNode = nullptr;

    // Check if source is a state machine
    if (UAnimGraphNode_StateMachine* SMNode = FindStateMachineNode(AnimBlueprint, SourceNodeName))
    {
        SourceNode = SMNode;
    }
    else
    {
        // Search other nodes by name
        for (UEdGraphNode* Node : AnimGraph->Nodes)
        {
            if (Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(SourceNodeName))
            {
                SourceNode = Node;
                break;
            }
        }
    }

    if (!SourceNode)
    {
        OutError = FString::Printf(TEXT("Could not find source node '%s' in AnimGraph"), *SourceNodeName);
        return false;
    }

    // Find the target node (typically the root/output pose)
    UEdGraphNode* TargetNode = nullptr;

    if (TargetNodeName.IsEmpty() || TargetNodeName.Equals(TEXT("OutputPose"), ESearchCase::IgnoreCase) || TargetNodeName.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
    {
        // Find the root node (output pose)
        for (UEdGraphNode* Node : AnimGraph->Nodes)
        {
            if (UAnimGraphNode_Root* RootNode = Cast<UAnimGraphNode_Root>(Node))
            {
                TargetNode = RootNode;
                break;
            }
        }
    }
    else
    {
        // Search other nodes by name
        for (UEdGraphNode* Node : AnimGraph->Nodes)
        {
            if (Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(TargetNodeName))
            {
                TargetNode = Node;
                break;
            }
        }
    }

    if (!TargetNode)
    {
        OutError = FString::Printf(TEXT("Could not find target node '%s' in AnimGraph"), *TargetNodeName);
        return false;
    }

    // Find the source pin (output)
    UEdGraphPin* SourcePin = nullptr;
    for (UEdGraphPin* Pin : SourceNode->Pins)
    {
        if (Pin->Direction == EGPD_Output && Pin->PinName.ToString().Contains(SourcePinName))
        {
            SourcePin = Pin;
            break;
        }
    }

    if (!SourcePin)
    {
        // List available output pins for error message
        TArray<FString> AvailablePins;
        for (UEdGraphPin* Pin : SourceNode->Pins)
        {
            if (Pin->Direction == EGPD_Output)
            {
                AvailablePins.Add(Pin->PinName.ToString());
            }
        }
        OutError = FString::Printf(TEXT("Could not find source output pin '%s'. Available: %s"), *SourcePinName, *FString::Join(AvailablePins, TEXT(", ")));
        return false;
    }

    // Find the target pin (input)
    UEdGraphPin* TargetPin = nullptr;
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        if (Pin->Direction == EGPD_Input && Pin->PinName.ToString().Contains(TargetPinName))
        {
            TargetPin = Pin;
            break;
        }
    }

    if (!TargetPin)
    {
        // List available input pins for error message
        TArray<FString> AvailablePins;
        for (UEdGraphPin* Pin : TargetNode->Pins)
        {
            if (Pin->Direction == EGPD_Input)
            {
                AvailablePins.Add(Pin->PinName.ToString());
            }
        }
        OutError = FString::Printf(TEXT("Could not find target input pin '%s'. Available: %s"), *TargetPinName, *FString::Join(AvailablePins, TEXT(", ")));
        return false;
    }

    // Create the connection
    const UEdGraphSchema* Schema = AnimGraph->GetSchema();
    if (!Schema)
    {
        OutError = TEXT("Could not get AnimGraph schema");
        return false;
    }

    // Try to create the connection
    bool bConnectionMade = Schema->TryCreateConnection(SourcePin, TargetPin);
    if (!bConnectionMade)
    {
        OutError = FString::Printf(TEXT("Failed to create connection from '%s.%s' to '%s.%s'"),
            *SourceNodeName, *SourcePinName,
            TargetNodeName.IsEmpty() ? TEXT("OutputPose") : *TargetNodeName, *TargetPinName);
        return false;
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);

    UE_LOG(LogTemp, Log, TEXT("FAnimationBlueprintService::ConnectAnimGraphNodes: Connected '%s.%s' to '%s.%s'"),
        *SourceNodeName, *SourcePinName,
        TargetNodeName.IsEmpty() ? TEXT("OutputPose") : *TargetNodeName, *TargetPinName);
    return true;
}

UClass* FAnimationBlueprintService::ResolveAnimInstanceClass(const FString& ParentClassName) const
{
    if (ParentClassName.IsEmpty())
    {
        return UAnimInstance::StaticClass();
    }

    // Try direct load for full paths
    if (ParentClassName.StartsWith(TEXT("/Script/")))
    {
        // For native C++ classes, use LoadClass
        if (UClass* FoundClass = LoadClass<UAnimInstance>(nullptr, *ParentClassName))
        {
            UE_LOG(LogTemp, Log, TEXT("ResolveAnimInstanceClass: Found native class at path: %s"), *ParentClassName);
            return FoundClass;
        }
        UE_LOG(LogTemp, Warning, TEXT("ResolveAnimInstanceClass: Failed to load native class at path: %s"), *ParentClassName);
    }
    else if (ParentClassName.StartsWith(TEXT("/Game/")))
    {
        // For Blueprint classes, use LoadObject<UBlueprint>
        if (UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *ParentClassName))
        {
            if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(UAnimInstance::StaticClass()))
            {
                UE_LOG(LogTemp, Log, TEXT("ResolveAnimInstanceClass: Found Blueprint class at path: %s"), *ParentClassName);
                return BP->GeneratedClass;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("ResolveAnimInstanceClass: Failed to load Blueprint class at path: %s"), *ParentClassName);
    }

    // Try common AnimInstance classes from known modules
    // Dynamically get the game module path from project name
    FString GameModulePath = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());

    TArray<FString> ModulePaths = {
        TEXT("/Script/Engine"),
        TEXT("/Script/AnimGraphRuntime"),
        GameModulePath  // Project module for custom AnimInstance classes - dynamically resolved
    };

    FString ClassName = ParentClassName;
    // Handle both with and without U prefix
    FString ClassNameWithU = ClassName.StartsWith(TEXT("U")) ? ClassName : TEXT("U") + ClassName;
    FString ClassNameWithoutU = ClassName.StartsWith(TEXT("U")) ? ClassName.RightChop(1) : ClassName;

    for (const FString& ModulePath : ModulePaths)
    {
        // Try with U prefix first
        FString ClassPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *ClassNameWithU);
        if (UClass* FoundClass = LoadClass<UAnimInstance>(nullptr, *ClassPath))
        {
            UE_LOG(LogTemp, Log, TEXT("ResolveAnimInstanceClass: Found class '%s' in module '%s'"), *ClassNameWithU, *ModulePath);
            return FoundClass;
        }

        // Try without U prefix (for class names like AnimLayer_Combat)
        ClassPath = FString::Printf(TEXT("%s.%s"), *ModulePath, *ClassNameWithoutU);
        if (UClass* FoundClass = LoadClass<UAnimInstance>(nullptr, *ClassPath))
        {
            UE_LOG(LogTemp, Log, TEXT("ResolveAnimInstanceClass: Found class '%s' in module '%s'"), *ClassNameWithoutU, *ModulePath);
            return FoundClass;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("ResolveAnimInstanceClass: Could not find class '%s', falling back to UAnimInstance"), *ParentClassName);
    return UAnimInstance::StaticClass();
}

USkeleton* FAnimationBlueprintService::FindSkeleton(const FString& SkeletonPath) const
{
    if (SkeletonPath.IsEmpty())
    {
        return nullptr;
    }

    // Try direct load
    if (USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath))
    {
        return Skeleton;
    }

    // Search asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssetsByClass(USkeleton::StaticClass()->GetClassPathName(), AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetName.ToString() == SkeletonPath || AssetData.GetObjectPathString().Contains(SkeletonPath))
        {
            return Cast<USkeleton>(AssetData.GetAsset());
        }
    }

    return nullptr;
}

UAnimationGraph* FAnimationBlueprintService::FindAnimGraph(UAnimBlueprint* AnimBlueprint) const
{
    if (!AnimBlueprint)
    {
        return nullptr;
    }

    for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
    {
        if (UAnimationGraph* AnimGraph = Cast<UAnimationGraph>(Graph))
        {
            return AnimGraph;
        }
    }

    return nullptr;
}

UAnimGraphNode_StateMachine* FAnimationBlueprintService::FindStateMachineNode(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName) const
{
    UAnimationGraph* AnimGraph = FindAnimGraph(AnimBlueprint);
    if (!AnimGraph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : AnimGraph->Nodes)
    {
        if (UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node))
        {
            if (SMNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Contains(StateMachineName))
            {
                return SMNode;
            }
        }
    }

    return nullptr;
}

UAnimStateNode* FAnimationBlueprintService::FindStateNode(UAnimationStateMachineGraph* StateMachineGraph, const FString& StateName) const
{
    if (!StateMachineGraph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : StateMachineGraph->Nodes)
    {
        if (UAnimStateNode* StateNode = Cast<UAnimStateNode>(Node))
        {
            if (StateNode->GetStateName() == StateName)
            {
                return StateNode;
            }
        }
    }

    return nullptr;
}
