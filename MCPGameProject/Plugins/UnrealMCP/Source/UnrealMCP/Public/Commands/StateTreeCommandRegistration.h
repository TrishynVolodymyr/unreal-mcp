#pragma once

#include "CoreMinimal.h"

/**
 * Static class responsible for registering all StateTree-related commands
 * with the command registry system
 */
class UNREALMCP_API FStateTreeCommandRegistration
{
public:
    /**
     * Register all StateTree commands with the command registry
     * This should be called during module startup
     */
    static void RegisterAllStateTreeCommands();

    /**
     * Unregister all StateTree commands from the command registry
     * This should be called during module shutdown
     */
    static void UnregisterAllStateTreeCommands();

private:
    /** Array of registered command names for cleanup */
    static TArray<FString> RegisteredCommandNames;

    // Tier 1 - Essential Commands
    static void RegisterCreateStateTreeCommand();
    static void RegisterAddStateCommand();
    static void RegisterAddTransitionCommand();
    static void RegisterAddTaskToStateCommand();
    static void RegisterCompileStateTreeCommand();
    static void RegisterGetStateTreeMetadataCommand();

    // Tier 2 - Advanced Commands
    static void RegisterAddConditionToTransitionCommand();
    static void RegisterAddEnterConditionCommand();
    static void RegisterAddEvaluatorCommand();
    static void RegisterSetStateParametersCommand();
    static void RegisterRemoveStateCommand();
    static void RegisterRemoveTransitionCommand();
    static void RegisterDuplicateStateTreeCommand();

    // Tier 3 - Introspection Commands
    static void RegisterGetStateTreeDiagnosticsCommand();
    static void RegisterGetAvailableTasksCommand();
    static void RegisterGetAvailableConditionsCommand();
    static void RegisterGetAvailableEvaluatorsCommand();

    // Section 1 - Property Binding Commands
    static void RegisterBindPropertyCommand();
    static void RegisterRemoveBindingCommand();
    static void RegisterGetNodeBindableInputsCommand();
    static void RegisterGetNodeExposedOutputsCommand();

    // Section 2 - Schema/Context Configuration Commands
    static void RegisterGetSchemaContextPropertiesCommand();
    static void RegisterSetContextRequirementsCommand();

    // Section 3 - Blueprint Type Support
    static void RegisterGetBlueprintStateTreeTypesCommand();

    // Section 4 - Global Tasks Commands
    static void RegisterAddGlobalTaskCommand();
    static void RegisterRemoveGlobalTaskCommand();

    // Section 5 - State Completion Configuration Commands
    static void RegisterSetStateCompletionModeCommand();
    static void RegisterSetTaskRequiredCommand();
    static void RegisterSetLinkedStateAssetCommand();

    // Section 6 - Quest Persistence Commands
    static void RegisterConfigureStatePersistenceCommand();
    static void RegisterGetPersistentStateDataCommand();

    // Section 7 - Gameplay Tag Integration Commands
    static void RegisterAddGameplayTagToStateCommand();
    static void RegisterQueryStatesByTagCommand();

    // Section 8 - Runtime Inspection Commands
    static void RegisterGetActiveStateTreeStatusCommand();
    static void RegisterGetCurrentActiveStatesCommand();

    // Section 9 - Utility AI Consideration
    static void RegisterAddConsiderationCommand();

    // Section 10 - Task/Evaluator Modification Commands
    static void RegisterRemoveTaskFromStateCommand();
    static void RegisterSetTaskPropertiesCommand();
    static void RegisterRemoveEvaluatorCommand();
    static void RegisterSetEvaluatorPropertiesCommand();

    // Section 11 - Condition Removal Commands
    static void RegisterRemoveConditionFromTransitionCommand();
    static void RegisterRemoveEnterConditionCommand();

    // Section 12 - Transition Inspection/Modification Commands
    static void RegisterGetTransitionInfoCommand();
    static void RegisterSetTransitionPropertiesCommand();
    static void RegisterGetTransitionConditionsCommand();

    // Section 13 - State Event Handler Commands
    static void RegisterAddStateEventHandlerCommand();
    static void RegisterConfigureStateNotificationsCommand();

    // Section 14 - Linked State Configuration Commands
    static void RegisterGetLinkedStateInfoCommand();
    static void RegisterSetLinkedStateParametersCommand();
    static void RegisterSetStateSelectionWeightCommand();

    // Section 15 - Batch Operations Commands
    static void RegisterBatchAddStatesCommand();
    static void RegisterBatchAddTransitionsCommand();

    // Section 16 - Validation and Debugging Commands
    static void RegisterValidateAllBindingsCommand();
    static void RegisterGetStateExecutionHistoryCommand();

    /**
     * Helper to register a command and track it for cleanup
     * @param Command - Command to register
     */
    static void RegisterAndTrackCommand(TSharedPtr<class IUnrealMCPCommand> Command);
};
