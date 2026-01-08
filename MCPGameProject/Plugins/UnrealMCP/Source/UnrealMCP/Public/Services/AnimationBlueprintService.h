#pragma once

#include "CoreMinimal.h"
#include "Services/IAnimationBlueprintService.h"

/**
 * Implementation of the Animation Blueprint service
 * Provides operations for creating and manipulating Animation Blueprints
 */
class UNREALMCP_API FAnimationBlueprintService : public IAnimationBlueprintService
{
public:
    /**
     * Get the singleton instance of the service
     */
    static FAnimationBlueprintService& Get();

    // ============================================================================
    // IAnimationBlueprintService Implementation
    // ============================================================================

    // Animation Blueprint Creation
    virtual UAnimBlueprint* CreateAnimBlueprint(const FAnimBlueprintCreationParams& Params) override;
    virtual UAnimBlueprint* FindAnimBlueprint(const FString& AnimBlueprintName) override;
    virtual bool CompileAnimBlueprint(UAnimBlueprint* AnimBlueprint, FString& OutError) override;

    // Animation Layers
    virtual bool LinkAnimationLayer(UAnimBlueprint* AnimBlueprint, const FAnimLayerLinkParams& Params, FString& OutError) override;
    virtual bool GetLinkedAnimationLayers(UAnimBlueprint* AnimBlueprint, TArray<FString>& OutLayers) override;

    // State Machines
    virtual bool CreateStateMachine(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, FString& OutError) override;
    virtual bool AddStateToStateMachine(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, const FAnimStateParams& Params, FString& OutError) override;
    virtual bool AddStateTransition(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, const FAnimTransitionParams& Params, FString& OutError) override;
    virtual bool GetStateMachineStates(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName, TArray<FString>& OutStates) override;

    // Animation Variables
    virtual bool AddAnimVariable(UAnimBlueprint* AnimBlueprint, const FString& VariableName, const FString& VariableType, const FString& DefaultValue, FString& OutError) override;
    virtual bool GetAnimVariables(UAnimBlueprint* AnimBlueprint, TArray<TPair<FString, FString>>& OutVariables) override;

    // Animation Slots
    virtual bool ConfigureAnimSlot(UAnimBlueprint* AnimBlueprint, const FString& SlotName, const FString& SlotGroupName, FString& OutError) override;

    // Metadata
    virtual bool GetAnimBlueprintMetadata(UAnimBlueprint* AnimBlueprint, TSharedPtr<FJsonObject>& OutMetadata) override;

    // AnimGraph Node Connections
    virtual bool ConnectAnimGraphNodes(UAnimBlueprint* AnimBlueprint, const FString& SourceNodeName, const FString& TargetNodeName, const FString& SourcePinName, const FString& TargetPinName, FString& OutError) override;

private:
    /**
     * Private constructor for singleton pattern
     */
    FAnimationBlueprintService() = default;

    /**
     * Resolve parent AnimInstance class from string
     * @param ParentClassName - Name or path of the parent class
     * @return Resolved UClass or nullptr
     */
    UClass* ResolveAnimInstanceClass(const FString& ParentClassName) const;

    /**
     * Find skeleton asset from path
     * @param SkeletonPath - Path to the skeleton asset
     * @return USkeleton or nullptr
     */
    class USkeleton* FindSkeleton(const FString& SkeletonPath) const;

    /**
     * Find the AnimGraph in an Animation Blueprint
     * @param AnimBlueprint - Target Animation Blueprint
     * @return AnimGraph or nullptr
     */
    class UAnimationGraph* FindAnimGraph(UAnimBlueprint* AnimBlueprint) const;

    /**
     * Find a state machine node by name
     * @param AnimBlueprint - Target Animation Blueprint
     * @param StateMachineName - Name of the state machine
     * @return State machine node or nullptr
     */
    class UAnimGraphNode_StateMachine* FindStateMachineNode(UAnimBlueprint* AnimBlueprint, const FString& StateMachineName) const;

    /**
     * Find a state node within a state machine
     * @param StateMachineGraph - The state machine graph
     * @param StateName - Name of the state
     * @return State node or nullptr
     */
    class UAnimStateNode* FindStateNode(class UAnimationStateMachineGraph* StateMachineGraph, const FString& StateName) const;

    /** Singleton instance */
    static TUniquePtr<FAnimationBlueprintService> Instance;
};
