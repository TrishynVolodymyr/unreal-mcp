#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class UStateTree;
class UStateTreeState;

/**
 * Parameters for StateTree asset creation
 */
struct UNREALMCP_API FStateTreeCreationParams
{
    /** Name of the StateTree asset to create */
    FString Name;

    /** Folder path where the StateTree should be created (e.g., "/Game/AI/StateTrees") */
    FString FolderPath = TEXT("/Game/AI/StateTrees");

    /** Schema class name (e.g., "StateTreeComponentSchema", "StateTreeAIComponentSchema") */
    FString SchemaClass = TEXT("StateTreeComponentSchema");

    /** Whether to compile after creation */
    bool bCompileOnCreation = false;

    FStateTreeCreationParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for adding a state to a StateTree
 */
struct UNREALMCP_API FAddStateParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to create */
    FString StateName;

    /** Name of the parent state (empty for root-level state) */
    FString ParentStateName;

    /** Type of state: "State", "Group", "Linked", "LinkedAsset", "Subtree" */
    FString StateType = TEXT("State");

    /** Selection behavior: "TrySelectChildrenInOrder", "TrySelectChildrenAtRandom", "None" */
    FString SelectionBehavior = TEXT("TrySelectChildrenInOrder");

    /** Whether the state is enabled */
    bool bEnabled = true;

    FAddStateParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for adding a transition to a StateTree
 */
struct UNREALMCP_API FAddTransitionParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the source state */
    FString SourceStateName;

    /** Trigger type: "OnStateCompleted", "OnStateFailed", "OnEvent", "OnTick" */
    FString Trigger = TEXT("OnStateCompleted");

    /** Name of the target state (if using GotoState transition type) */
    FString TargetStateName;

    /** Transition type: "GotoState", "NextState", "Succeeded", "Failed" */
    FString TransitionType = TEXT("GotoState");

    /** Event tag for OnEvent trigger type */
    FString EventTag;

    /** Priority: "Low", "Normal", "High", "Critical" */
    FString Priority = TEXT("Normal");

    /** Whether to delay the transition */
    bool bDelayTransition = false;

    /** Duration of delay in seconds */
    float DelayDuration = 0.0f;

    FAddTransitionParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for adding a task to a state
 */
struct UNREALMCP_API FAddTaskParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to add the task to */
    FString StateName;

    /** Task struct path (e.g., "/Script/StateTreeModule.StateTreeDelayTask") */
    FString TaskStructPath;

    /** Task properties as JSON object */
    TSharedPtr<FJsonObject> TaskProperties;

    /** Optional name for the task instance */
    FString TaskName;

    FAddTaskParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for adding a condition to a transition
 */
struct UNREALMCP_API FAddConditionParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the source state containing the transition */
    FString SourceStateName;

    /** Index of the transition to add condition to (0-based) */
    int32 TransitionIndex = 0;

    /** Condition struct path (e.g., "/Script/StateTreeModule.StateTreeCompareIntCondition") */
    FString ConditionStructPath;

    /** Condition properties as JSON object */
    TSharedPtr<FJsonObject> ConditionProperties;

    /** How to combine with existing conditions: "And", "Or" */
    FString CombineMode = TEXT("And");

    FAddConditionParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for adding an enter condition to a state
 */
struct UNREALMCP_API FAddEnterConditionParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to add the enter condition to */
    FString StateName;

    /** Condition struct path */
    FString ConditionStructPath;

    /** Condition properties as JSON object */
    TSharedPtr<FJsonObject> ConditionProperties;

    FAddEnterConditionParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for adding an evaluator to the StateTree
 */
struct UNREALMCP_API FAddEvaluatorParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Evaluator struct path (e.g., "/Script/StateTreeModule.StateTreeObjectPropertyEvaluator") */
    FString EvaluatorStructPath;

    /** Evaluator properties as JSON object */
    TSharedPtr<FJsonObject> EvaluatorProperties;

    /** Optional name for the evaluator instance */
    FString EvaluatorName;

    FAddEvaluatorParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting state parameters
 */
struct UNREALMCP_API FSetStateParametersParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to modify */
    FString StateName;

    /** Parameters as JSON object (varies by task/state type) */
    TSharedPtr<FJsonObject> Parameters;

    FSetStateParametersParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for removing a state
 */
struct UNREALMCP_API FRemoveStateParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to remove */
    FString StateName;

    /** Whether to remove child states recursively */
    bool bRemoveChildren = true;

    FRemoveStateParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for removing a transition
 */
struct UNREALMCP_API FRemoveTransitionParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the source state containing the transition */
    FString SourceStateName;

    /** Index of the transition to remove (0-based) */
    int32 TransitionIndex = 0;

    FRemoveTransitionParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 1: Property Binding Params
// ============================================================================

/**
 * Parameters for binding a property between nodes
 */
struct UNREALMCP_API FBindPropertyParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Source node identifier (evaluator name or "Context" for schema context) */
    FString SourceNodeName;

    /** Property name on the source node to bind from */
    FString SourcePropertyName;

    /** Target node identifier (state name for tasks, or state:task_index format) */
    FString TargetNodeName;

    /** Property name on the target node to bind to */
    FString TargetPropertyName;

    /** Optional: Index of task within state (if binding to a specific task) */
    int32 TaskIndex = -1;

    FBindPropertyParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 4: Global Tasks Params
// ============================================================================

/**
 * Parameters for adding a global task to the StateTree
 */
struct UNREALMCP_API FAddGlobalTaskParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Task struct path */
    FString TaskStructPath;

    /** Optional name for the task instance */
    FString TaskName;

    /** Task properties as JSON object */
    TSharedPtr<FJsonObject> TaskProperties;

    FAddGlobalTaskParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for removing a global task
 */
struct UNREALMCP_API FRemoveGlobalTaskParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Index of the global task to remove (0-based) */
    int32 TaskIndex = 0;

    FRemoveGlobalTaskParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 5: State Completion Configuration Params
// ============================================================================

/**
 * Parameters for setting state completion mode
 */
struct UNREALMCP_API FSetStateCompletionModeParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to configure */
    FString StateName;

    /** Completion mode: "AllTasks", "AnyTask", "Explicit" */
    FString CompletionMode = TEXT("AllTasks");

    FSetStateCompletionModeParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting task required/optional status
 */
struct UNREALMCP_API FSetTaskRequiredParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state containing the task */
    FString StateName;

    /** Index of the task within the state (0-based) */
    int32 TaskIndex = 0;

    /** Whether the task is required (failure causes state failure) */
    bool bRequired = true;

    FSetTaskRequiredParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting linked state asset
 */
struct UNREALMCP_API FSetLinkedStateAssetParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the linked state to configure */
    FString StateName;

    /** Path to the external StateTree asset to link */
    FString LinkedAssetPath;

    FSetLinkedStateAssetParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 6: Quest Persistence Params
// ============================================================================

/**
 * Parameters for configuring state persistence
 */
struct UNREALMCP_API FConfigureStatePersistenceParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to configure */
    FString StateName;

    /** Whether this state should be persisted */
    bool bPersistent = true;

    /** Optional persistence key for save/load identification */
    FString PersistenceKey;

    FConfigureStatePersistenceParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 7: Gameplay Tag Integration Params
// ============================================================================

/**
 * Parameters for adding a gameplay tag to a state
 */
struct UNREALMCP_API FAddGameplayTagToStateParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to tag */
    FString StateName;

    /** Gameplay tag to add (e.g., "Quest.MainQuest.Active") */
    FString GameplayTag;

    FAddGameplayTagToStateParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for querying states by gameplay tag
 */
struct UNREALMCP_API FQueryStatesByTagParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Gameplay tag to search for */
    FString GameplayTag;

    /** Whether to match exact tag or include children */
    bool bExactMatch = false;

    FQueryStatesByTagParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 9: Utility AI Consideration Params
// ============================================================================

/**
 * Parameters for adding a consideration to a state
 */
struct UNREALMCP_API FAddConsiderationParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state to add consideration to */
    FString StateName;

    /** Consideration struct path */
    FString ConsiderationStructPath;

    /** Consideration properties as JSON object */
    TSharedPtr<FJsonObject> ConsiderationProperties;

    /** Weight for this consideration in utility scoring */
    float Weight = 1.0f;

    FAddConsiderationParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 10: Task/Evaluator Modification Params
// ============================================================================

/**
 * Parameters for removing a task from a state
 */
struct UNREALMCP_API FRemoveTaskFromStateParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state containing the task */
    FString StateName;

    /** Index of the task to remove (0-based) */
    int32 TaskIndex = 0;

    FRemoveTaskFromStateParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting task properties
 */
struct UNREALMCP_API FSetTaskPropertiesParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state containing the task */
    FString StateName;

    /** Index of the task to modify (0-based) */
    int32 TaskIndex = 0;

    /** Properties to set as JSON object */
    TSharedPtr<FJsonObject> Properties;

    FSetTaskPropertiesParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for removing an evaluator
 */
struct UNREALMCP_API FRemoveEvaluatorParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Index of the evaluator to remove (0-based) */
    int32 EvaluatorIndex = 0;

    FRemoveEvaluatorParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting evaluator properties
 */
struct UNREALMCP_API FSetEvaluatorPropertiesParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Index of the evaluator to modify (0-based) */
    int32 EvaluatorIndex = 0;

    /** Properties to set as JSON object */
    TSharedPtr<FJsonObject> Properties;

    FSetEvaluatorPropertiesParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 11: Condition Removal Params
// ============================================================================

/**
 * Parameters for removing a condition from a transition
 */
struct UNREALMCP_API FRemoveConditionFromTransitionParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the source state containing the transition */
    FString SourceStateName;

    /** Index of the transition (0-based) */
    int32 TransitionIndex = 0;

    /** Index of the condition to remove (0-based) */
    int32 ConditionIndex = 0;

    FRemoveConditionFromTransitionParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for removing an enter condition from a state
 */
struct UNREALMCP_API FRemoveEnterConditionParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state */
    FString StateName;

    /** Index of the enter condition to remove (0-based) */
    int32 ConditionIndex = 0;

    FRemoveEnterConditionParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 12: Transition Inspection/Modification Params
// ============================================================================

/**
 * Parameters for getting transition info
 */
struct UNREALMCP_API FGetTransitionInfoParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the source state containing the transition */
    FString SourceStateName;

    /** Index of the transition (0-based) */
    int32 TransitionIndex = 0;

    FGetTransitionInfoParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting transition properties
 */
struct UNREALMCP_API FSetTransitionPropertiesParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the source state containing the transition */
    FString SourceStateName;

    /** Index of the transition (0-based) */
    int32 TransitionIndex = 0;

    /** New trigger type (optional) */
    FString Trigger;

    /** New target state name (optional) */
    FString TargetStateName;

    /** New priority (optional) */
    FString Priority;

    /** New delay setting (optional) */
    TOptional<bool> bDelayTransition;

    /** New delay duration (optional) */
    TOptional<float> DelayDuration;

    FSetTransitionPropertiesParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 13: State Event Handler Params
// ============================================================================

/**
 * Parameters for adding a state event handler
 */
struct UNREALMCP_API FAddStateEventHandlerParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state */
    FString StateName;

    /** Event type: "Enter", "Exit", "Tick" */
    FString EventType = TEXT("Enter");

    /** Task struct path for the handler */
    FString TaskStructPath;

    /** Task properties as JSON object */
    TSharedPtr<FJsonObject> TaskProperties;

    FAddStateEventHandlerParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for configuring state notifications
 */
struct UNREALMCP_API FConfigureStateNotificationsParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state */
    FString StateName;

    /** Gameplay tag to send on state enter */
    FString EnterNotificationTag;

    /** Gameplay tag to send on state exit */
    FString ExitNotificationTag;

    FConfigureStateNotificationsParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 14: Linked State Configuration Params
// ============================================================================

/**
 * Parameters for getting linked state info
 */
struct UNREALMCP_API FGetLinkedStateInfoParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the linked state */
    FString StateName;

    FGetLinkedStateInfoParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting linked state parameters
 */
struct UNREALMCP_API FSetLinkedStateParametersParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the linked state */
    FString StateName;

    /** Parameters to pass to the linked state tree as JSON object */
    TSharedPtr<FJsonObject> Parameters;

    FSetLinkedStateParametersParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Parameters for setting state selection weight
 */
struct UNREALMCP_API FSetStateSelectionWeightParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Name of the state */
    FString StateName;

    /** Selection weight (used for weighted random selection) */
    float Weight = 1.0f;

    FSetStateSelectionWeightParams() = default;

    bool IsValid(FString& OutError) const;
};

// ============================================================================
// Section 15: Batch Operations Params
// ============================================================================

/**
 * Single state definition for batch operations
 */
struct UNREALMCP_API FBatchStateDefinition
{
    FString StateName;
    FString ParentStateName;
    FString StateType = TEXT("State");
    FString SelectionBehavior = TEXT("TrySelectChildrenInOrder");
    bool bEnabled = true;
};

/**
 * Parameters for batch adding states
 */
struct UNREALMCP_API FBatchAddStatesParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Array of state definitions to add */
    TArray<FBatchStateDefinition> States;

    FBatchAddStatesParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Single transition definition for batch operations
 */
struct UNREALMCP_API FBatchTransitionDefinition
{
    FString SourceStateName;
    FString TargetStateName;
    FString Trigger = TEXT("OnStateCompleted");
    FString TransitionType = TEXT("GotoState");
    FString Priority = TEXT("Normal");
};

/**
 * Parameters for batch adding transitions
 */
struct UNREALMCP_API FBatchAddTransitionsParams
{
    /** Path to the StateTree asset */
    FString StateTreePath;

    /** Array of transition definitions to add */
    TArray<FBatchTransitionDefinition> Transitions;

    FBatchAddTransitionsParams() = default;

    bool IsValid(FString& OutError) const;
};

/**
 * Interface for StateTree service operations
 */
class UNREALMCP_API IStateTreeService
{
public:
    virtual ~IStateTreeService() = default;

    // ============================================================================
    // Asset Management
    // ============================================================================

    /**
     * Create a new StateTree asset
     * @param Params - StateTree creation parameters
     * @param OutError - Error message if creation fails
     * @return Created StateTree or nullptr if failed
     */
    virtual UStateTree* CreateStateTree(const FStateTreeCreationParams& Params, FString& OutError) = 0;

    /**
     * Find a StateTree by path or name
     * @param PathOrName - Asset path or name
     * @return Found StateTree or nullptr
     */
    virtual UStateTree* FindStateTree(const FString& PathOrName) = 0;

    /**
     * Compile a StateTree for runtime use
     * @param StateTree - StateTree to compile
     * @param OutError - Error message if compilation fails
     * @return true if compilation succeeded
     */
    virtual bool CompileStateTree(UStateTree* StateTree, FString& OutError) = 0;

    /**
     * Duplicate a StateTree asset
     * @param SourcePath - Path to source StateTree
     * @param DestPath - Destination path
     * @param NewName - Name for the new asset
     * @param OutError - Error message if duplication fails
     * @return Duplicated StateTree or nullptr if failed
     */
    virtual UStateTree* DuplicateStateTree(const FString& SourcePath, const FString& DestPath, const FString& NewName, FString& OutError) = 0;

    // ============================================================================
    // State Management
    // ============================================================================

    /**
     * Add a state to a StateTree
     * @param Params - State parameters
     * @param OutError - Error message if addition fails
     * @return true if state was added successfully
     */
    virtual bool AddState(const FAddStateParams& Params, FString& OutError) = 0;

    /**
     * Remove a state from a StateTree
     * @param Params - Remove state parameters
     * @param OutError - Error message if removal fails
     * @return true if state was removed successfully
     */
    virtual bool RemoveState(const FRemoveStateParams& Params, FString& OutError) = 0;

    /**
     * Set parameters on a state
     * @param Params - State parameter settings
     * @param OutError - Error message if setting fails
     * @return true if parameters were set successfully
     */
    virtual bool SetStateParameters(const FSetStateParametersParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Transition Management
    // ============================================================================

    /**
     * Add a transition between states
     * @param Params - Transition parameters
     * @param OutError - Error message if addition fails
     * @return true if transition was added successfully
     */
    virtual bool AddTransition(const FAddTransitionParams& Params, FString& OutError) = 0;

    /**
     * Remove a transition from a state
     * @param Params - Remove transition parameters
     * @param OutError - Error message if removal fails
     * @return true if transition was removed successfully
     */
    virtual bool RemoveTransition(const FRemoveTransitionParams& Params, FString& OutError) = 0;

    /**
     * Add a condition to a transition
     * @param Params - Condition parameters
     * @param OutError - Error message if addition fails
     * @return true if condition was added successfully
     */
    virtual bool AddConditionToTransition(const FAddConditionParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Task and Evaluator Management
    // ============================================================================

    /**
     * Add a task to a state
     * @param Params - Task parameters
     * @param OutError - Error message if addition fails
     * @return true if task was added successfully
     */
    virtual bool AddTaskToState(const FAddTaskParams& Params, FString& OutError) = 0;

    /**
     * Add an enter condition to a state
     * @param Params - Enter condition parameters
     * @param OutError - Error message if addition fails
     * @return true if enter condition was added successfully
     */
    virtual bool AddEnterCondition(const FAddEnterConditionParams& Params, FString& OutError) = 0;

    /**
     * Add an evaluator to the StateTree
     * @param Params - Evaluator parameters
     * @param OutError - Error message if addition fails
     * @return true if evaluator was added successfully
     */
    virtual bool AddEvaluator(const FAddEvaluatorParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Introspection
    // ============================================================================

    /**
     * Get StateTree metadata as JSON
     * @param StateTree - StateTree to query
     * @param OutMetadata - JSON object containing metadata
     * @return true if metadata was retrieved successfully
     */
    virtual bool GetStateTreeMetadata(UStateTree* StateTree, TSharedPtr<FJsonObject>& OutMetadata) = 0;

    /**
     * Get StateTree compilation diagnostics
     * @param StateTree - StateTree to validate
     * @param OutDiagnostics - JSON object containing diagnostic information
     * @return true if diagnostics were retrieved successfully
     */
    virtual bool GetStateTreeDiagnostics(UStateTree* StateTree, TSharedPtr<FJsonObject>& OutDiagnostics) = 0;

    /**
     * Get available task types
     * @param OutTasks - Array of task struct paths and names
     * @return true if tasks were retrieved successfully
     */
    virtual bool GetAvailableTaskTypes(TArray<TPair<FString, FString>>& OutTasks) = 0;

    /**
     * Get available condition types
     * @param OutConditions - Array of condition struct paths and names
     * @return true if conditions were retrieved successfully
     */
    virtual bool GetAvailableConditionTypes(TArray<TPair<FString, FString>>& OutConditions) = 0;

    /**
     * Get available evaluator types
     * @param OutEvaluators - Array of evaluator struct paths and names
     * @return true if evaluators were retrieved successfully
     */
    virtual bool GetAvailableEvaluatorTypes(TArray<TPair<FString, FString>>& OutEvaluators) = 0;

    // ============================================================================
    // Section 1: Property Binding
    // ============================================================================

    /**
     * Bind a property from one node to another
     * @param Params - Binding parameters
     * @param OutError - Error message if binding fails
     * @return true if binding was successful
     */
    virtual bool BindProperty(const FBindPropertyParams& Params, FString& OutError) = 0;

    /**
     * Get bindable input properties for a node
     * @param StateTreePath - Path to the StateTree
     * @param NodeIdentifier - Node identifier (state name, evaluator name, etc.)
     * @param TaskIndex - Task index if querying a specific task (-1 for state/evaluator)
     * @param OutInputs - JSON object containing bindable inputs
     * @return true if inputs were retrieved successfully
     */
    virtual bool GetNodeBindableInputs(const FString& StateTreePath, const FString& NodeIdentifier, int32 TaskIndex, TSharedPtr<FJsonObject>& OutInputs) = 0;

    /**
     * Get exposed output properties from a node (evaluator or context)
     * @param StateTreePath - Path to the StateTree
     * @param NodeIdentifier - Node identifier (evaluator name or "Context")
     * @param OutOutputs - JSON object containing exposed outputs
     * @return true if outputs were retrieved successfully
     */
    virtual bool GetNodeExposedOutputs(const FString& StateTreePath, const FString& NodeIdentifier, TSharedPtr<FJsonObject>& OutOutputs) = 0;

    // ============================================================================
    // Section 2: Schema/Context Configuration
    // ============================================================================

    /**
     * Get schema context properties available in the StateTree
     * @param StateTreePath - Path to the StateTree
     * @param OutProperties - JSON object containing context properties
     * @return true if properties were retrieved successfully
     */
    virtual bool GetSchemaContextProperties(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutProperties) = 0;

    /**
     * Set context requirements for the StateTree schema
     * @param StateTreePath - Path to the StateTree
     * @param Requirements - JSON object containing requirement settings
     * @param OutError - Error message if setting fails
     * @return true if requirements were set successfully
     */
    virtual bool SetContextRequirements(const FString& StateTreePath, const TSharedPtr<FJsonObject>& Requirements, FString& OutError) = 0;

    // ============================================================================
    // Section 3: Blueprint Task/Condition/Evaluator Support
    // ============================================================================

    /**
     * Get Blueprint-based StateTree types (tasks, conditions, evaluators) in the project
     * @param OutTypes - JSON object containing arrays of BP types by category
     * @return true if types were retrieved successfully
     */
    virtual bool GetBlueprintStateTreeTypes(TSharedPtr<FJsonObject>& OutTypes) = 0;

    // ============================================================================
    // Section 4: Global Tasks
    // ============================================================================

    /**
     * Add a global task to the StateTree (runs at tree level)
     * @param Params - Global task parameters
     * @param OutError - Error message if addition fails
     * @return true if task was added successfully
     */
    virtual bool AddGlobalTask(const FAddGlobalTaskParams& Params, FString& OutError) = 0;

    /**
     * Remove a global task from the StateTree
     * @param Params - Remove task parameters
     * @param OutError - Error message if removal fails
     * @return true if task was removed successfully
     */
    virtual bool RemoveGlobalTask(const FRemoveGlobalTaskParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 5: State Completion Configuration
    // ============================================================================

    /**
     * Set how a state determines completion
     * @param Params - Completion mode parameters
     * @param OutError - Error message if setting fails
     * @return true if mode was set successfully
     */
    virtual bool SetStateCompletionMode(const FSetStateCompletionModeParams& Params, FString& OutError) = 0;

    /**
     * Set whether a task is required (failure causes state failure)
     * @param Params - Task required parameters
     * @param OutError - Error message if setting fails
     * @return true if setting was successful
     */
    virtual bool SetTaskRequired(const FSetTaskRequiredParams& Params, FString& OutError) = 0;

    /**
     * Set the linked asset for a LinkedAsset state type
     * @param Params - Linked asset parameters
     * @param OutError - Error message if setting fails
     * @return true if asset was linked successfully
     */
    virtual bool SetLinkedStateAsset(const FSetLinkedStateAssetParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 6: Quest Persistence
    // ============================================================================

    /**
     * Configure persistence settings for a state
     * @param Params - Persistence parameters
     * @param OutError - Error message if configuration fails
     * @return true if configuration was successful
     */
    virtual bool ConfigureStatePersistence(const FConfigureStatePersistenceParams& Params, FString& OutError) = 0;

    /**
     * Get persistent state data for save/load
     * @param StateTreePath - Path to the StateTree
     * @param OutData - JSON object containing persistent state data
     * @return true if data was retrieved successfully
     */
    virtual bool GetPersistentStateData(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutData) = 0;

    // ============================================================================
    // Section 7: Gameplay Tag Integration
    // ============================================================================

    /**
     * Add a gameplay tag to a state for external querying
     * @param Params - Tag parameters
     * @param OutError - Error message if addition fails
     * @return true if tag was added successfully
     */
    virtual bool AddGameplayTagToState(const FAddGameplayTagToStateParams& Params, FString& OutError) = 0;

    /**
     * Query states by gameplay tag
     * @param Params - Query parameters
     * @param OutStates - Array of state names matching the tag
     * @return true if query was successful
     */
    virtual bool QueryStatesByTag(const FQueryStatesByTagParams& Params, TArray<FString>& OutStates) = 0;

    // ============================================================================
    // Section 8: Runtime Inspection (PIE)
    // ============================================================================

    /**
     * Get the status of an active StateTree during PIE
     * @param StateTreePath - Path to the StateTree asset
     * @param ActorPath - Path to the actor running the StateTree (optional, for multiple instances)
     * @param OutStatus - JSON object containing runtime status
     * @return true if status was retrieved successfully
     */
    virtual bool GetActiveStateTreeStatus(const FString& StateTreePath, const FString& ActorPath, TSharedPtr<FJsonObject>& OutStatus) = 0;

    /**
     * Get currently active states during PIE
     * @param StateTreePath - Path to the StateTree asset
     * @param ActorPath - Path to the actor running the StateTree (optional)
     * @param OutActiveStates - Array of currently active state names
     * @return true if states were retrieved successfully
     */
    virtual bool GetCurrentActiveStates(const FString& StateTreePath, const FString& ActorPath, TArray<FString>& OutActiveStates) = 0;

    // ============================================================================
    // Section 9: Utility AI Considerations
    // ============================================================================

    /**
     * Add a consideration for utility-based state selection
     * @param Params - Consideration parameters
     * @param OutError - Error message if addition fails
     * @return true if consideration was added successfully
     */
    virtual bool AddConsideration(const FAddConsiderationParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 10: Task/Evaluator Modification
    // ============================================================================

    /**
     * Remove a task from a state
     * @param Params - Remove task parameters
     * @param OutError - Error message if removal fails
     * @return true if task was removed successfully
     */
    virtual bool RemoveTaskFromState(const FRemoveTaskFromStateParams& Params, FString& OutError) = 0;

    /**
     * Set properties on an existing task
     * @param Params - Task property parameters
     * @param OutError - Error message if setting fails
     * @return true if properties were set successfully
     */
    virtual bool SetTaskProperties(const FSetTaskPropertiesParams& Params, FString& OutError) = 0;

    /**
     * Remove an evaluator from the StateTree
     * @param Params - Remove evaluator parameters
     * @param OutError - Error message if removal fails
     * @return true if evaluator was removed successfully
     */
    virtual bool RemoveEvaluator(const FRemoveEvaluatorParams& Params, FString& OutError) = 0;

    /**
     * Set properties on an existing evaluator
     * @param Params - Evaluator property parameters
     * @param OutError - Error message if setting fails
     * @return true if properties were set successfully
     */
    virtual bool SetEvaluatorProperties(const FSetEvaluatorPropertiesParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 11: Condition Removal
    // ============================================================================

    /**
     * Remove a condition from a transition
     * @param Params - Remove condition parameters
     * @param OutError - Error message if removal fails
     * @return true if condition was removed successfully
     */
    virtual bool RemoveConditionFromTransition(const FRemoveConditionFromTransitionParams& Params, FString& OutError) = 0;

    /**
     * Remove an enter condition from a state
     * @param Params - Remove enter condition parameters
     * @param OutError - Error message if removal fails
     * @return true if enter condition was removed successfully
     */
    virtual bool RemoveEnterCondition(const FRemoveEnterConditionParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 12: Transition Inspection/Modification
    // ============================================================================

    /**
     * Get detailed information about a specific transition
     * @param Params - Transition info parameters
     * @param OutInfo - JSON object containing transition details
     * @return true if info was retrieved successfully
     */
    virtual bool GetTransitionInfo(const FGetTransitionInfoParams& Params, TSharedPtr<FJsonObject>& OutInfo) = 0;

    /**
     * Modify properties of an existing transition
     * @param Params - Transition property parameters
     * @param OutError - Error message if modification fails
     * @return true if transition was modified successfully
     */
    virtual bool SetTransitionProperties(const FSetTransitionPropertiesParams& Params, FString& OutError) = 0;

    /**
     * Get all conditions on a transition
     * @param StateTreePath - Path to the StateTree
     * @param SourceStateName - Name of the source state
     * @param TransitionIndex - Index of the transition
     * @param OutConditions - JSON object containing condition details
     * @return true if conditions were retrieved successfully
     */
    virtual bool GetTransitionConditions(const FString& StateTreePath, const FString& SourceStateName, int32 TransitionIndex, TSharedPtr<FJsonObject>& OutConditions) = 0;

    // ============================================================================
    // Section 13: State Event Handlers
    // ============================================================================

    /**
     * Add an event handler to a state (Enter/Exit/Tick)
     * @param Params - Event handler parameters
     * @param OutError - Error message if addition fails
     * @return true if handler was added successfully
     */
    virtual bool AddStateEventHandler(const FAddStateEventHandlerParams& Params, FString& OutError) = 0;

    /**
     * Configure gameplay event notifications for a state
     * @param Params - Notification parameters
     * @param OutError - Error message if configuration fails
     * @return true if notifications were configured successfully
     */
    virtual bool ConfigureStateNotifications(const FConfigureStateNotificationsParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 14: Linked State Configuration
    // ============================================================================

    /**
     * Get information about a linked state
     * @param Params - Linked state info parameters
     * @param OutInfo - JSON object containing linked state details
     * @return true if info was retrieved successfully
     */
    virtual bool GetLinkedStateInfo(const FGetLinkedStateInfoParams& Params, TSharedPtr<FJsonObject>& OutInfo) = 0;

    /**
     * Set parameters to pass to a linked/subtree state
     * @param Params - Linked state parameter settings
     * @param OutError - Error message if setting fails
     * @return true if parameters were set successfully
     */
    virtual bool SetLinkedStateParameters(const FSetLinkedStateParametersParams& Params, FString& OutError) = 0;

    /**
     * Set the selection weight for weighted random child selection
     * @param Params - Selection weight parameters
     * @param OutError - Error message if setting fails
     * @return true if weight was set successfully
     */
    virtual bool SetStateSelectionWeight(const FSetStateSelectionWeightParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 15: Batch Operations
    // ============================================================================

    /**
     * Add multiple states in a single operation
     * @param Params - Batch add states parameters
     * @param OutError - Error message if addition fails
     * @return true if all states were added successfully
     */
    virtual bool BatchAddStates(const FBatchAddStatesParams& Params, FString& OutError) = 0;

    /**
     * Add multiple transitions in a single operation
     * @param Params - Batch add transitions parameters
     * @param OutError - Error message if addition fails
     * @return true if all transitions were added successfully
     */
    virtual bool BatchAddTransitions(const FBatchAddTransitionsParams& Params, FString& OutError) = 0;

    // ============================================================================
    // Section 16: Validation and Debugging
    // ============================================================================

    /**
     * Validate all property bindings in the StateTree
     * @param StateTreePath - Path to the StateTree
     * @param OutValidationResults - JSON object containing validation results
     * @return true if validation ran successfully (not necessarily all bindings valid)
     */
    virtual bool ValidateAllBindings(const FString& StateTreePath, TSharedPtr<FJsonObject>& OutValidationResults) = 0;

    /**
     * Get execution history of a StateTree during PIE (for debugging)
     * @param StateTreePath - Path to the StateTree
     * @param ActorPath - Path to the actor running the StateTree (optional)
     * @param MaxEntries - Maximum history entries to return
     * @param OutHistory - JSON object containing execution history
     * @return true if history was retrieved successfully
     */
    virtual bool GetStateExecutionHistory(const FString& StateTreePath, const FString& ActorPath, int32 MaxEntries, TSharedPtr<FJsonObject>& OutHistory) = 0;
};
