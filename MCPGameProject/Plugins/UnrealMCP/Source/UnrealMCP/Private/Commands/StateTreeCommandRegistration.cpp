#include "Commands/StateTreeCommandRegistration.h"
#include "Commands/UnrealMCPCommandRegistry.h"
#include "Services/StateTreeService.h"

// Tier 1 - Essential Commands
#include "Commands/StateTree/CreateStateTreeCommand.h"
#include "Commands/StateTree/AddStateCommand.h"
#include "Commands/StateTree/AddTransitionCommand.h"
#include "Commands/StateTree/AddTaskToStateCommand.h"
#include "Commands/StateTree/CompileStateTreeCommand.h"
#include "Commands/StateTree/GetStateTreeMetadataCommand.h"

// Tier 2 - Advanced Commands
#include "Commands/StateTree/AddConditionToTransitionCommand.h"
#include "Commands/StateTree/AddEnterConditionCommand.h"
#include "Commands/StateTree/AddEvaluatorCommand.h"
#include "Commands/StateTree/SetStateParametersCommand.h"
#include "Commands/StateTree/RemoveStateCommand.h"
#include "Commands/StateTree/RemoveTransitionCommand.h"
#include "Commands/StateTree/DuplicateStateTreeCommand.h"

// Tier 3 - Introspection Commands
#include "Commands/StateTree/GetStateTreeDiagnosticsCommand.h"
#include "Commands/StateTree/GetAvailableTasksCommand.h"
#include "Commands/StateTree/GetAvailableConditionsCommand.h"
#include "Commands/StateTree/GetAvailableEvaluatorsCommand.h"

// Section 1 - Property Binding Commands
#include "Commands/StateTree/BindPropertyCommand.h"
#include "Commands/StateTree/RemoveBindingCommand.h"
#include "Commands/StateTree/GetNodeBindableInputsCommand.h"
#include "Commands/StateTree/GetNodeExposedOutputsCommand.h"

// Section 2 - Schema/Context Configuration Commands
#include "Commands/StateTree/GetSchemaContextPropertiesCommand.h"
#include "Commands/StateTree/SetContextRequirementsCommand.h"

// Section 3 - Blueprint Type Support
#include "Commands/StateTree/GetBlueprintStateTreeTypesCommand.h"

// Section 4 - Global Tasks Commands
#include "Commands/StateTree/AddGlobalTaskCommand.h"
#include "Commands/StateTree/RemoveGlobalTaskCommand.h"

// Section 5 - State Completion Configuration Commands
#include "Commands/StateTree/SetStateCompletionModeCommand.h"
#include "Commands/StateTree/SetTaskRequiredCommand.h"
#include "Commands/StateTree/SetLinkedStateAssetCommand.h"

// Section 6 - Quest Persistence Commands
#include "Commands/StateTree/ConfigureStatePersistenceCommand.h"
#include "Commands/StateTree/GetPersistentStateDataCommand.h"

// Section 7 - Gameplay Tag Integration Commands
#include "Commands/StateTree/AddGameplayTagToStateCommand.h"
#include "Commands/StateTree/QueryStatesByTagCommand.h"

// Section 8 - Runtime Inspection Commands
#include "Commands/StateTree/GetActiveStateTreeStatusCommand.h"
#include "Commands/StateTree/GetCurrentActiveStatesCommand.h"

// Section 9 - Utility AI Consideration
#include "Commands/StateTree/AddConsiderationCommand.h"

// Section 10 - Task/Evaluator Modification Commands
#include "Commands/StateTree/RemoveTaskFromStateCommand.h"
#include "Commands/StateTree/SetTaskPropertiesCommand.h"
#include "Commands/StateTree/RemoveEvaluatorCommand.h"
#include "Commands/StateTree/SetEvaluatorPropertiesCommand.h"

// Section 11 - Condition Removal Commands
#include "Commands/StateTree/RemoveConditionFromTransitionCommand.h"
#include "Commands/StateTree/RemoveEnterConditionCommand.h"

// Section 12 - Transition Inspection/Modification Commands
#include "Commands/StateTree/GetTransitionInfoCommand.h"
#include "Commands/StateTree/SetTransitionPropertiesCommand.h"
#include "Commands/StateTree/GetTransitionConditionsCommand.h"

// Section 13 - State Event Handler Commands
#include "Commands/StateTree/AddStateEventHandlerCommand.h"
#include "Commands/StateTree/ConfigureStateNotificationsCommand.h"

// Section 14 - Linked State Configuration Commands
#include "Commands/StateTree/GetLinkedStateInfoCommand.h"
#include "Commands/StateTree/SetLinkedStateParametersCommand.h"
#include "Commands/StateTree/SetStateSelectionWeightCommand.h"

// Section 15 - Batch Operations Commands
#include "Commands/StateTree/BatchAddStatesCommand.h"
#include "Commands/StateTree/BatchAddTransitionsCommand.h"

// Section 16 - Validation and Debugging Commands
#include "Commands/StateTree/ValidateAllBindingsCommand.h"
#include "Commands/StateTree/GetStateExecutionHistoryCommand.h"

// Static member definition
TArray<FString> FStateTreeCommandRegistration::RegisteredCommandNames;

void FStateTreeCommandRegistration::RegisterAllStateTreeCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FStateTreeCommandRegistration::RegisterAllStateTreeCommands: Starting StateTree command registration"));

    // Clear any existing registrations
    RegisteredCommandNames.Empty();

    // Tier 1 - Essential Commands
    RegisterCreateStateTreeCommand();
    RegisterAddStateCommand();
    RegisterAddTransitionCommand();
    RegisterAddTaskToStateCommand();
    RegisterCompileStateTreeCommand();
    RegisterGetStateTreeMetadataCommand();

    // Tier 2 - Advanced Commands
    RegisterAddConditionToTransitionCommand();
    RegisterAddEnterConditionCommand();
    RegisterAddEvaluatorCommand();
    RegisterSetStateParametersCommand();
    RegisterRemoveStateCommand();
    RegisterRemoveTransitionCommand();
    RegisterDuplicateStateTreeCommand();

    // Tier 3 - Introspection Commands
    RegisterGetStateTreeDiagnosticsCommand();
    RegisterGetAvailableTasksCommand();
    RegisterGetAvailableConditionsCommand();
    RegisterGetAvailableEvaluatorsCommand();

    // Section 1 - Property Binding Commands
    RegisterBindPropertyCommand();
    RegisterRemoveBindingCommand();
    RegisterGetNodeBindableInputsCommand();
    RegisterGetNodeExposedOutputsCommand();

    // Section 2 - Schema/Context Configuration Commands
    RegisterGetSchemaContextPropertiesCommand();
    RegisterSetContextRequirementsCommand();

    // Section 3 - Blueprint Type Support
    RegisterGetBlueprintStateTreeTypesCommand();

    // Section 4 - Global Tasks Commands
    RegisterAddGlobalTaskCommand();
    RegisterRemoveGlobalTaskCommand();

    // Section 5 - State Completion Configuration Commands
    RegisterSetStateCompletionModeCommand();
    RegisterSetTaskRequiredCommand();
    RegisterSetLinkedStateAssetCommand();

    // Section 6 - Quest Persistence Commands
    RegisterConfigureStatePersistenceCommand();
    RegisterGetPersistentStateDataCommand();

    // Section 7 - Gameplay Tag Integration Commands
    RegisterAddGameplayTagToStateCommand();
    RegisterQueryStatesByTagCommand();

    // Section 8 - Runtime Inspection Commands
    RegisterGetActiveStateTreeStatusCommand();
    RegisterGetCurrentActiveStatesCommand();

    // Section 9 - Utility AI Consideration
    RegisterAddConsiderationCommand();

    // Section 10 - Task/Evaluator Modification Commands
    RegisterRemoveTaskFromStateCommand();
    RegisterSetTaskPropertiesCommand();
    RegisterRemoveEvaluatorCommand();
    RegisterSetEvaluatorPropertiesCommand();

    // Section 11 - Condition Removal Commands
    RegisterRemoveConditionFromTransitionCommand();
    RegisterRemoveEnterConditionCommand();

    // Section 12 - Transition Inspection/Modification Commands
    RegisterGetTransitionInfoCommand();
    RegisterSetTransitionPropertiesCommand();
    RegisterGetTransitionConditionsCommand();

    // Section 13 - State Event Handler Commands
    RegisterAddStateEventHandlerCommand();
    RegisterConfigureStateNotificationsCommand();

    // Section 14 - Linked State Configuration Commands
    RegisterGetLinkedStateInfoCommand();
    RegisterSetLinkedStateParametersCommand();
    RegisterSetStateSelectionWeightCommand();

    // Section 15 - Batch Operations Commands
    RegisterBatchAddStatesCommand();
    RegisterBatchAddTransitionsCommand();

    // Section 16 - Validation and Debugging Commands
    RegisterValidateAllBindingsCommand();
    RegisterGetStateExecutionHistoryCommand();

    UE_LOG(LogTemp, Log, TEXT("FStateTreeCommandRegistration::RegisterAllStateTreeCommands: Registered %d StateTree commands"),
        RegisteredCommandNames.Num());
}

void FStateTreeCommandRegistration::UnregisterAllStateTreeCommands()
{
    UE_LOG(LogTemp, Log, TEXT("FStateTreeCommandRegistration::UnregisterAllStateTreeCommands: Starting StateTree command unregistration"));

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();

    int32 UnregisteredCount = 0;
    for (const FString& CommandName : RegisteredCommandNames)
    {
        if (Registry.UnregisterCommand(CommandName))
        {
            UnregisteredCount++;
        }
    }

    RegisteredCommandNames.Empty();

    UE_LOG(LogTemp, Log, TEXT("FStateTreeCommandRegistration::UnregisterAllStateTreeCommands: Unregistered %d StateTree commands"),
        UnregisteredCount);
}

// Tier 1 - Essential Commands

void FStateTreeCommandRegistration::RegisterCreateStateTreeCommand()
{
    TSharedPtr<FCreateStateTreeCommand> Command = MakeShared<FCreateStateTreeCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterAddStateCommand()
{
    TSharedPtr<FAddStateCommand> Command = MakeShared<FAddStateCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterAddTransitionCommand()
{
    TSharedPtr<FAddTransitionCommand> Command = MakeShared<FAddTransitionCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterAddTaskToStateCommand()
{
    TSharedPtr<FAddTaskToStateCommand> Command = MakeShared<FAddTaskToStateCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterCompileStateTreeCommand()
{
    TSharedPtr<FCompileStateTreeCommand> Command = MakeShared<FCompileStateTreeCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetStateTreeMetadataCommand()
{
    TSharedPtr<FGetStateTreeMetadataCommand> Command = MakeShared<FGetStateTreeMetadataCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Tier 2 - Advanced Commands

void FStateTreeCommandRegistration::RegisterAddConditionToTransitionCommand()
{
    TSharedPtr<FAddConditionToTransitionCommand> Command = MakeShared<FAddConditionToTransitionCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterAddEnterConditionCommand()
{
    TSharedPtr<FAddEnterConditionCommand> Command = MakeShared<FAddEnterConditionCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterAddEvaluatorCommand()
{
    TSharedPtr<FAddEvaluatorCommand> Command = MakeShared<FAddEvaluatorCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetStateParametersCommand()
{
    TSharedPtr<FSetStateParametersCommand> Command = MakeShared<FSetStateParametersCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterRemoveStateCommand()
{
    TSharedPtr<FRemoveStateCommand> Command = MakeShared<FRemoveStateCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterRemoveTransitionCommand()
{
    TSharedPtr<FRemoveTransitionCommand> Command = MakeShared<FRemoveTransitionCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterDuplicateStateTreeCommand()
{
    TSharedPtr<FDuplicateStateTreeCommand> Command = MakeShared<FDuplicateStateTreeCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Tier 3 - Introspection Commands

void FStateTreeCommandRegistration::RegisterGetStateTreeDiagnosticsCommand()
{
    TSharedPtr<FGetStateTreeDiagnosticsCommand> Command = MakeShared<FGetStateTreeDiagnosticsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetAvailableTasksCommand()
{
    TSharedPtr<FGetAvailableTasksCommand> Command = MakeShared<FGetAvailableTasksCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetAvailableConditionsCommand()
{
    TSharedPtr<FGetAvailableConditionsCommand> Command = MakeShared<FGetAvailableConditionsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetAvailableEvaluatorsCommand()
{
    TSharedPtr<FGetAvailableEvaluatorsCommand> Command = MakeShared<FGetAvailableEvaluatorsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 1 - Property Binding Commands

void FStateTreeCommandRegistration::RegisterBindPropertyCommand()
{
    TSharedPtr<FBindPropertyCommand> Command = MakeShared<FBindPropertyCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterRemoveBindingCommand()
{
    TSharedPtr<FRemoveBindingCommand> Command = MakeShared<FRemoveBindingCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetNodeBindableInputsCommand()
{
    TSharedPtr<FGetNodeBindableInputsCommand> Command = MakeShared<FGetNodeBindableInputsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetNodeExposedOutputsCommand()
{
    TSharedPtr<FGetNodeExposedOutputsCommand> Command = MakeShared<FGetNodeExposedOutputsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 2 - Schema/Context Configuration Commands

void FStateTreeCommandRegistration::RegisterGetSchemaContextPropertiesCommand()
{
    TSharedPtr<FGetSchemaContextPropertiesCommand> Command = MakeShared<FGetSchemaContextPropertiesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetContextRequirementsCommand()
{
    TSharedPtr<FSetContextRequirementsCommand> Command = MakeShared<FSetContextRequirementsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 3 - Blueprint Type Support

void FStateTreeCommandRegistration::RegisterGetBlueprintStateTreeTypesCommand()
{
    TSharedPtr<FGetBlueprintStateTreeTypesCommand> Command = MakeShared<FGetBlueprintStateTreeTypesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 4 - Global Tasks Commands

void FStateTreeCommandRegistration::RegisterAddGlobalTaskCommand()
{
    TSharedPtr<FAddGlobalTaskCommand> Command = MakeShared<FAddGlobalTaskCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterRemoveGlobalTaskCommand()
{
    TSharedPtr<FRemoveGlobalTaskCommand> Command = MakeShared<FRemoveGlobalTaskCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 5 - State Completion Configuration Commands

void FStateTreeCommandRegistration::RegisterSetStateCompletionModeCommand()
{
    TSharedPtr<FSetStateCompletionModeCommand> Command = MakeShared<FSetStateCompletionModeCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetTaskRequiredCommand()
{
    TSharedPtr<FSetTaskRequiredCommand> Command = MakeShared<FSetTaskRequiredCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetLinkedStateAssetCommand()
{
    TSharedPtr<FSetLinkedStateAssetCommand> Command = MakeShared<FSetLinkedStateAssetCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 6 - Quest Persistence Commands

void FStateTreeCommandRegistration::RegisterConfigureStatePersistenceCommand()
{
    TSharedPtr<FConfigureStatePersistenceCommand> Command = MakeShared<FConfigureStatePersistenceCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetPersistentStateDataCommand()
{
    TSharedPtr<FGetPersistentStateDataCommand> Command = MakeShared<FGetPersistentStateDataCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 7 - Gameplay Tag Integration Commands

void FStateTreeCommandRegistration::RegisterAddGameplayTagToStateCommand()
{
    TSharedPtr<FAddGameplayTagToStateCommand> Command = MakeShared<FAddGameplayTagToStateCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterQueryStatesByTagCommand()
{
    TSharedPtr<FQueryStatesByTagCommand> Command = MakeShared<FQueryStatesByTagCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 8 - Runtime Inspection Commands

void FStateTreeCommandRegistration::RegisterGetActiveStateTreeStatusCommand()
{
    TSharedPtr<FGetActiveStateTreeStatusCommand> Command = MakeShared<FGetActiveStateTreeStatusCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetCurrentActiveStatesCommand()
{
    TSharedPtr<FGetCurrentActiveStatesCommand> Command = MakeShared<FGetCurrentActiveStatesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 9 - Utility AI Consideration

void FStateTreeCommandRegistration::RegisterAddConsiderationCommand()
{
    TSharedPtr<FAddConsiderationCommand> Command = MakeShared<FAddConsiderationCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 10 - Task/Evaluator Modification Commands

void FStateTreeCommandRegistration::RegisterRemoveTaskFromStateCommand()
{
    TSharedPtr<FRemoveTaskFromStateCommand> Command = MakeShared<FRemoveTaskFromStateCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetTaskPropertiesCommand()
{
    TSharedPtr<FSetTaskPropertiesCommand> Command = MakeShared<FSetTaskPropertiesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterRemoveEvaluatorCommand()
{
    TSharedPtr<FRemoveEvaluatorCommand> Command = MakeShared<FRemoveEvaluatorCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetEvaluatorPropertiesCommand()
{
    TSharedPtr<FSetEvaluatorPropertiesCommand> Command = MakeShared<FSetEvaluatorPropertiesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 11 - Condition Removal Commands

void FStateTreeCommandRegistration::RegisterRemoveConditionFromTransitionCommand()
{
    TSharedPtr<FRemoveConditionFromTransitionCommand> Command = MakeShared<FRemoveConditionFromTransitionCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterRemoveEnterConditionCommand()
{
    TSharedPtr<FRemoveEnterConditionCommand> Command = MakeShared<FRemoveEnterConditionCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 12 - Transition Inspection/Modification Commands

void FStateTreeCommandRegistration::RegisterGetTransitionInfoCommand()
{
    TSharedPtr<FGetTransitionInfoCommand> Command = MakeShared<FGetTransitionInfoCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetTransitionPropertiesCommand()
{
    TSharedPtr<FSetTransitionPropertiesCommand> Command = MakeShared<FSetTransitionPropertiesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetTransitionConditionsCommand()
{
    TSharedPtr<FGetTransitionConditionsCommand> Command = MakeShared<FGetTransitionConditionsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 13 - State Event Handler Commands

void FStateTreeCommandRegistration::RegisterAddStateEventHandlerCommand()
{
    TSharedPtr<FAddStateEventHandlerCommand> Command = MakeShared<FAddStateEventHandlerCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterConfigureStateNotificationsCommand()
{
    TSharedPtr<FConfigureStateNotificationsCommand> Command = MakeShared<FConfigureStateNotificationsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 14 - Linked State Configuration Commands

void FStateTreeCommandRegistration::RegisterGetLinkedStateInfoCommand()
{
    TSharedPtr<FGetLinkedStateInfoCommand> Command = MakeShared<FGetLinkedStateInfoCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetLinkedStateParametersCommand()
{
    TSharedPtr<FSetLinkedStateParametersCommand> Command = MakeShared<FSetLinkedStateParametersCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterSetStateSelectionWeightCommand()
{
    TSharedPtr<FSetStateSelectionWeightCommand> Command = MakeShared<FSetStateSelectionWeightCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 15 - Batch Operations Commands

void FStateTreeCommandRegistration::RegisterBatchAddStatesCommand()
{
    TSharedPtr<FBatchAddStatesCommand> Command = MakeShared<FBatchAddStatesCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterBatchAddTransitionsCommand()
{
    TSharedPtr<FBatchAddTransitionsCommand> Command = MakeShared<FBatchAddTransitionsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

// Section 16 - Validation and Debugging Commands

void FStateTreeCommandRegistration::RegisterValidateAllBindingsCommand()
{
    TSharedPtr<FValidateAllBindingsCommand> Command = MakeShared<FValidateAllBindingsCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterGetStateExecutionHistoryCommand()
{
    TSharedPtr<FGetStateExecutionHistoryCommand> Command = MakeShared<FGetStateExecutionHistoryCommand>(FStateTreeService::Get());
    RegisterAndTrackCommand(Command);
}

void FStateTreeCommandRegistration::RegisterAndTrackCommand(TSharedPtr<IUnrealMCPCommand> Command)
{
    if (!Command.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FStateTreeCommandRegistration::RegisterAndTrackCommand: Invalid command"));
        return;
    }

    FString CommandName = Command->GetCommandName();
    if (CommandName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("FStateTreeCommandRegistration::RegisterAndTrackCommand: Command has empty name"));
        return;
    }

    FUnrealMCPCommandRegistry& Registry = FUnrealMCPCommandRegistry::Get();
    if (Registry.RegisterCommand(Command))
    {
        RegisteredCommandNames.Add(CommandName);
        UE_LOG(LogTemp, Verbose, TEXT("FStateTreeCommandRegistration::RegisterAndTrackCommand: Registered and tracked command '%s'"), *CommandName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FStateTreeCommandRegistration::RegisterAndTrackCommand: Failed to register command '%s'"), *CommandName);
    }
}
