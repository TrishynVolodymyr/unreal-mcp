#pragma once

#include "CoreMinimal.h"
#include "Services/IStateTreeService.h"

/**
 * Implementation of StateTree service operations
 */
class UNREALMCP_API FStateTreeService : public IStateTreeService
{
public:
    FStateTreeService();
    virtual ~FStateTreeService() = default;

    /** Get singleton instance */
    static FStateTreeService& Get();

    // ============================================================================
    // IStateTreeService Implementation - Asset Management
    // ============================================================================

    virtual UStateTree* CreateStateTree(const FStateTreeCreationParams& Params, FString& OutError) override;
    virtual UStateTree* FindStateTree(const FString& PathOrName) override;
    virtual bool CompileStateTree(UStateTree* StateTree, FString& OutError) override;
    virtual UStateTree* DuplicateStateTree(const FString& SourcePath, const FString& DestPath, const FString& NewName, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - State Management
    // ============================================================================

    virtual bool AddState(const FAddStateParams& Params, FString& OutError) override;
    virtual bool RemoveState(const FRemoveStateParams& Params, FString& OutError) override;
    virtual bool SetStateParameters(const FSetStateParametersParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Transition Management
    // ============================================================================

    virtual bool AddTransition(const FAddTransitionParams& Params, FString& OutError) override;
    virtual bool RemoveTransition(const FRemoveTransitionParams& Params, FString& OutError) override;
    virtual bool AddConditionToTransition(const FAddConditionParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Task and Evaluator Management
    // ============================================================================

    virtual bool AddTaskToState(const FAddTaskParams& Params, FString& OutError) override;
    virtual bool AddEnterCondition(const FAddEnterConditionParams& Params, FString& OutError) override;
    virtual bool AddEvaluator(const FAddEvaluatorParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Introspection
    // ============================================================================

    virtual bool GetStateTreeMetadata(UStateTree* StateTree, TSharedPtr<FJsonObject>& OutMetadata) override;
    virtual bool GetStateTreeDiagnostics(UStateTree* StateTree, TSharedPtr<FJsonObject>& OutDiagnostics) override;
    virtual bool GetAvailableTaskTypes(TArray<TPair<FString, FString>>& OutTasks) override;
    virtual bool GetAvailableConditionTypes(TArray<TPair<FString, FString>>& OutConditions) override;
    virtual bool GetAvailableEvaluatorTypes(TArray<TPair<FString, FString>>& OutEvaluators) override;

    // ============================================================================
    // IStateTreeService Implementation - Property Binding (Section 1)
    // ============================================================================

    virtual bool BindProperty(const FBindPropertyParams& Params, FString& OutError) override;
    virtual bool RemoveBinding(const FRemoveBindingParams& Params, FString& OutError) override;
    virtual bool GetNodeBindableInputs(const FString& StateTreePath, const FString& NodeIdentifier, int32 TaskIndex, TSharedPtr<FJsonObject>& OutInputs) override;
    virtual bool GetNodeExposedOutputs(const FString& StateTreePath, const FString& NodeIdentifier, TSharedPtr<FJsonObject>& OutOutputs) override;

    // ============================================================================
    // IStateTreeService Implementation - Schema/Context Configuration (Section 2)
    // ============================================================================

    virtual bool GetSchemaContextProperties(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutProperties) override;
    virtual bool SetContextRequirements(const FString& StateTreePath, const TSharedPtr<FJsonObject>& Requirements, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Blueprint Type Support (Section 3)
    // ============================================================================

    virtual bool GetBlueprintStateTreeTypes(TSharedPtr<FJsonObject>& OutTypes) override;

    // ============================================================================
    // IStateTreeService Implementation - Global Tasks (Section 4)
    // ============================================================================

    virtual bool AddGlobalTask(const FAddGlobalTaskParams& Params, FString& OutError) override;
    virtual bool RemoveGlobalTask(const FRemoveGlobalTaskParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - State Completion Configuration (Section 5)
    // ============================================================================

    virtual bool SetStateCompletionMode(const FSetStateCompletionModeParams& Params, FString& OutError) override;
    virtual bool SetTaskRequired(const FSetTaskRequiredParams& Params, FString& OutError) override;
    virtual bool SetLinkedStateAsset(const FSetLinkedStateAssetParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Quest Persistence (Section 6)
    // ============================================================================

    virtual bool ConfigureStatePersistence(const FConfigureStatePersistenceParams& Params, FString& OutError) override;
    virtual bool GetPersistentStateData(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutData) override;

    // ============================================================================
    // IStateTreeService Implementation - Gameplay Tag Integration (Section 7)
    // ============================================================================

    virtual bool AddGameplayTagToState(const FAddGameplayTagToStateParams& Params, FString& OutError) override;
    virtual bool QueryStatesByTag(const FQueryStatesByTagParams& Params, TArray<FString>& OutStates) override;

    // ============================================================================
    // IStateTreeService Implementation - Runtime Inspection (Section 8)
    // ============================================================================

    virtual bool GetActiveStateTreeStatus(const FString& StateTreePath, const FString& ActorPath, TSharedPtr<FJsonObject>& OutStatus) override;
    virtual bool GetCurrentActiveStates(const FString& StateTreePath, const FString& ActorPath, TArray<FString>& OutActiveStates) override;

    // ============================================================================
    // IStateTreeService Implementation - Utility AI Considerations (Section 9)
    // ============================================================================

    virtual bool AddConsideration(const FAddConsiderationParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Task/Evaluator Modification (Section 10)
    // ============================================================================

    virtual bool RemoveTaskFromState(const FRemoveTaskFromStateParams& Params, FString& OutError) override;
    virtual bool SetTaskProperties(const FSetTaskPropertiesParams& Params, FString& OutError) override;
    virtual bool RemoveEvaluator(const FRemoveEvaluatorParams& Params, FString& OutError) override;
    virtual bool SetEvaluatorProperties(const FSetEvaluatorPropertiesParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Condition Removal (Section 11)
    // ============================================================================

    virtual bool RemoveConditionFromTransition(const FRemoveConditionFromTransitionParams& Params, FString& OutError) override;
    virtual bool RemoveEnterCondition(const FRemoveEnterConditionParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Transition Inspection/Modification (Section 12)
    // ============================================================================

    virtual bool GetTransitionInfo(const FGetTransitionInfoParams& Params, TSharedPtr<FJsonObject>& OutInfo) override;
    virtual bool SetTransitionProperties(const FSetTransitionPropertiesParams& Params, FString& OutError) override;
    virtual bool GetTransitionConditions(const FString& StateTreePath, const FString& SourceStateName, int32 TransitionIndex, TSharedPtr<FJsonObject>& OutConditions) override;

    // ============================================================================
    // IStateTreeService Implementation - State Event Handlers (Section 13)
    // ============================================================================

    virtual bool AddStateEventHandler(const FAddStateEventHandlerParams& Params, FString& OutError) override;
    virtual bool ConfigureStateNotifications(const FConfigureStateNotificationsParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Linked State Configuration (Section 14)
    // ============================================================================

    virtual bool GetLinkedStateInfo(const FGetLinkedStateInfoParams& Params, TSharedPtr<FJsonObject>& OutInfo) override;
    virtual bool SetLinkedStateParameters(const FSetLinkedStateParametersParams& Params, FString& OutError) override;
    virtual bool SetStateSelectionWeight(const FSetStateSelectionWeightParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Batch Operations (Section 15)
    // ============================================================================

    virtual bool BatchAddStates(const FBatchAddStatesParams& Params, FString& OutError) override;
    virtual bool BatchAddTransitions(const FBatchAddTransitionsParams& Params, FString& OutError) override;

    // ============================================================================
    // IStateTreeService Implementation - Validation and Debugging (Section 16)
    // ============================================================================

    virtual bool ValidateAllBindings(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutValidationResults) override;
    virtual bool GetStateExecutionHistory(const FString& StateTreePath, const FString& ActorPath, int32 MaxEntries, TSharedPtr<FJsonObject>& OutHistory) override;

private:
    /** Helper to find a state by name within editor data */
    class UStateTreeState* FindStateByName(class UStateTreeEditorData* EditorData, const FString& StateName);

    /** Helper to find a state recursively */
    class UStateTreeState* FindStateByNameRecursive(class UStateTreeState* State, const FString& StateName);

    /** Helper to get all root states from editor data */
    TArray<class UStateTreeState*> GetRootStates(class UStateTreeEditorData* EditorData);

    /** Helper to build state metadata recursively */
    TSharedPtr<FJsonObject> BuildStateMetadata(class UStateTreeState* State);

    /** Helper to parse state type string to enum */
    int32 ParseStateType(const FString& StateTypeString);

    /** Helper to parse selection behavior string to enum */
    int32 ParseSelectionBehavior(const FString& BehaviorString);

    /** Helper to parse transition trigger string to enum */
    int32 ParseTransitionTrigger(const FString& TriggerString);

    /** Helper to parse transition type string to enum */
    int32 ParseTransitionType(const FString& TypeString);

    /** Helper to parse priority string to enum */
    int32 ParsePriority(const FString& PriorityString);

    /** Helper to save an asset to disk */
    bool SaveAsset(UObject* Asset, FString& OutError);
};
