# StateTree Tools - Unreal MCP

This document provides comprehensive information about using StateTree tools through the Unreal MCP (Model Context Protocol). These tools allow you to create, configure, and manage StateTree assets — Unreal Engine's hierarchical state machine system for AI, gameplay logic, and quest systems — using natural language commands.

## Overview

StateTree tools enable you to:
- Create and duplicate StateTree assets with different schemas (Component, AI)
- Add states with hierarchical nesting, types (State, Group, Linked, LinkedAsset, Subtree), and selection behaviors
- Configure transitions between states with triggers (OnStateCompleted, OnTick, OnEvent), priorities, and delays
- Add tasks to states (Delay, RunParallelStateTree, EQS queries, Blueprint tasks)
- Add conditions to transitions and state entry (compare int/float/bool/enum, gameplay tags)
- Add global evaluators for runtime data (Actor, AIController, ObjectProperty)
- Bind properties between evaluators, tasks, conditions, and schema context
- Configure state completion modes, task requirements, and persistence
- Add gameplay tags to states for external querying
- Set up global tasks, event handlers, and state notifications
- Batch-add states and transitions for efficient tree construction
- Validate bindings and run diagnostics
- Inspect active StateTrees during Play-In-Editor for debugging
- Discover available task, condition, and evaluator types (including Blueprint-based)

## Natural Language Usage Examples

### Creating StateTrees

```
"Create a new StateTree called 'ST_AIBehavior' with the AI Component schema"

"Duplicate ST_AIBehavior as ST_AIBehavior_Aggressive"

"Create a StateTree for general gameplay logic using the Component schema"
```

### Building State Hierarchies

```
"Add Idle, Patrol, and Combat states to the StateTree"

"Create a Combat group state with Attack and Defend child states"

"Add a LinkedAsset state that references an external patrol StateTree"

"Batch-add states: Root group with Idle, Patrol, and Combat children"
```

### Transitions and Conditions

```
"Add a transition from Idle to Patrol when the state completes"

"Add an OnEvent transition from Patrol to Combat triggered by AI.Event.PlayerSpotted"

"Add a condition to the transition: check if health is below 30%"

"Create a high-priority OnTick transition from any state to Flee when health is critical"
```

### Tasks and Evaluators

```
"Add a 3-second delay task to the Idle state"

"Add an EQS query task to the Patrol state"

"Add an AI evaluator to expose the AI controller data to all states"

"Bind the evaluator's TargetActor property to the MoveTo task's target input"
```

### Debugging and Validation

```
"Run diagnostics on ST_AIBehavior — is it valid?"

"Show me the current active states during play"

"Validate all property bindings in the StateTree"

"What tasks and conditions are available to use?"
```

## Tool Reference

---

### `create_state_tree`

Create a new StateTree asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the StateTree (e.g., `"ST_AIBehavior"`) |
| `path` | string | | Folder path for the asset |
| `schema_class` | string | | `"StateTreeComponentSchema"` (default) or `"StateTreeAIComponentSchema"` |
| `compile_on_creation` | boolean | | Whether to compile after creation (default: False) |

---

### `add_state`

Add a state to a StateTree. States can be nested under parent states.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | Name of the state (e.g., `"Idle"`, `"Combat"`) |
| `parent_state_name` | string | | Parent state name (empty for root-level) |
| `state_type` | string | | `"State"` (default), `"Group"`, `"Linked"`, `"LinkedAsset"`, `"Subtree"` |
| `selection_behavior` | string | | `"TrySelectChildrenInOrder"` (default), `"TrySelectChildrenAtRandom"`, `"None"` |
| `enabled` | boolean | | Whether the state is enabled (default: True) |

---

### `add_transition`

Add a transition between states with configurable triggers and priorities.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `source_state_name` | string | ✅ | Source state name |
| `trigger` | string | | `"OnStateCompleted"` (default), `"OnStateFailed"`, `"OnEvent"`, `"OnTick"` |
| `target_state_name` | string | | Destination state name (required for GotoState) |
| `transition_type` | string | | `"GotoState"` (default), `"NextState"`, `"Succeeded"`, `"Failed"` |
| `event_tag` | string | | Gameplay tag for OnEvent trigger |
| `priority` | string | | `"Low"`, `"Normal"` (default), `"High"`, `"Critical"` |
| `delay_transition` | boolean | | Whether to delay the transition |
| `delay_duration` | number | | Delay time in seconds |

---

### `add_task_to_state`

Add a task to a state.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | State to add the task to |
| `task_struct_path` | string | ✅ | Full path to the task struct (see common tasks below) |
| `task_name` | string | | Display name for the task instance |
| `task_properties` | object | | Dict of task-specific properties |

**Common tasks:**
- `"/Script/StateTreeModule.StateTreeDelayTask"` — Wait for duration
- `"/Script/StateTreeModule.StateTreeRunParallelStateTreeTask"` — Run parallel tree
- `"/Script/GameplayStateTreeModule.StateTreeRunEnvQueryTask"` — EQS query
- `"/Script/GameplayStateTreeModule.StateTreeBlueprintPropertyRefTask"` — Blueprint task

---

### `add_condition_to_transition`

Add a condition to a transition.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `source_state_name` | string | ✅ | State containing the transition |
| `condition_struct_path` | string | ✅ | Full path to the condition struct |
| `transition_index` | number | | Which transition to modify (0-based) |
| `combine_mode` | string | | `"And"` (default) or `"Or"` |
| `condition_properties` | object | | Dict of condition-specific properties |

**Common conditions:**
- `"/Script/StateTreeModule.StateTreeCompareIntCondition"`
- `"/Script/StateTreeModule.StateTreeCompareFloatCondition"`
- `"/Script/StateTreeModule.StateTreeCompareBoolCondition"`
- `"/Script/GameplayStateTreeModule.StateTreeGameplayTagCondition"`

---

### `add_enter_condition`

Add an enter condition to a state — state is skipped if conditions aren't met.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | State to add the enter condition to |
| `condition_struct_path` | string | ✅ | Full path to the condition struct |
| `condition_properties` | object | | Dict of condition-specific properties |

---

### `add_evaluator`

Add a global evaluator to a StateTree. Evaluators run every tick and expose data to all states.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `evaluator_struct_path` | string | ✅ | Full path to the evaluator struct |
| `evaluator_name` | string | | Display name for the evaluator |
| `evaluator_properties` | object | | Dict of evaluator-specific properties |

**Common evaluators:**
- `"/Script/StateTreeModule.StateTreeObjectPropertyEvaluator"` — Expose object property
- `"/Script/GameplayStateTreeModule.StateTreeActorEvaluator"` — Actor info
- `"/Script/GameplayStateTreeModule.StateTreeAIEvaluator"` — AI controller data

---

### `bind_state_tree_property`

Bind a property from one node to another for data flow between evaluators, tasks, and context.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `source_node_name` | string | ✅ | Source node (evaluator name or `"Context"`) |
| `source_property_name` | string | ✅ | Property name on the source |
| `target_node_name` | string | ✅ | Target node (state name) |
| `target_property_name` | string | ✅ | Property name on the target |
| `task_index` | number | | Task index if binding to a specific task |
| `transition_index` | number | | Transition index if binding to a condition |
| `condition_index` | number | | Condition index within transition |

---

### `remove_state_tree_binding`

Remove a property binding from a target node.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `target_node_name` | string | ✅ | Target node identifier |
| `target_property_name` | string | ✅ | Property name with the binding to remove |
| `task_index` | number | | Task index (-1 to ignore) |
| `transition_index` | number | | Transition index (-1 to ignore) |
| `condition_index` | number | | Condition index (-1 to ignore) |

---

### `compile_state_tree`

Compile a StateTree for runtime use. Validates structure and prepares for execution.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |

---

### `get_state_tree_metadata`

Get full metadata — structure, states (hierarchical), tasks, transitions, and evaluators.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |

---

### `get_state_tree_diagnostics`

Get validation/diagnostic information — compilation errors, warnings, and statistics.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |

---

### `duplicate_state_tree`

Duplicate an existing StateTree asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `source_path` | string | ✅ | Path to the source StateTree |
| `dest_path` | string | ✅ | Destination folder path |
| `new_name` | string | ✅ | Name for the duplicate |

---

### `set_state_parameters`

Set parameters on a state (name, enabled, type, selection behavior).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | Name of the state to modify |
| `parameters` | object | ✅ | Dict of parameters to set |

---

### `remove_state` / `remove_transition`

Remove a state (with optional recursive child removal) or a transition.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` / `source_state_name` | string | ✅ | State name |
| `remove_children` | boolean | | Remove children recursively (default: True) |
| `transition_index` | number | | Index of transition to remove (0-based) |

---

### `get_available_tasks` / `get_available_conditions` / `get_available_evaluators`

Discover what tasks, conditions, or evaluators can be used. Returns struct paths for use with add tools.

No parameters required — returns arrays of available types with paths and names.

---

### `get_node_bindable_inputs` / `get_node_exposed_outputs`

Discover what properties can be bound on tasks/states/evaluators, or what properties are exposed from evaluators/context.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `node_identifier` | string | ✅ | Node identifier (state name, evaluator name, or `"Context"`) |
| `task_index` | number | | Task index if querying a specific task |

---

### `get_schema_context_properties`

Get schema context properties available in a StateTree (e.g., Actor, AIController).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |

---

### `set_state_completion_mode`

Set how a state determines completion.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | State name |
| `completion_mode` | string | | `"AllTasks"` (default), `"AnyTask"`, `"Explicit"` |

---

### `add_global_task` / `remove_global_task`

Add or remove global tasks that run at the tree level, independent of current state.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `task_struct_path` | string | ✅ | Full path to the task struct (add only) |
| `task_name` | string | | Display name (add only) |
| `task_index` | number | | Index of task to remove (remove only) |

---

### `add_gameplay_tag_to_state` / `query_states_by_tag`

Tag states for external querying, or find states matching a gameplay tag.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | State name (add only) |
| `gameplay_tag` | string | ✅ | Gameplay tag (e.g., `"Quest.MainQuest.Active"`) |
| `exact_match` | boolean | | Exact match or include child tags (query only, default: False) |

---

### `set_linked_state_asset`

Connect a LinkedAsset state to an external StateTree.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | Name of the LinkedAsset state |
| `linked_asset_path` | string | ✅ | Path to the external StateTree to link |

---

### `configure_state_persistence` / `get_persistent_state_data`

Configure persistence for save/load systems, or retrieve persistence information.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `state_name` | string | ✅ | State name (configure only) |
| `persistent` | boolean | | Whether state should be persisted (default: True) |
| `persistence_key` | string | | Key for save/load identification |

---

### `batch_add_states` / `batch_add_transitions`

Efficiently create complex state hierarchies or transition networks in a single operation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `states` / `transitions` | array | ✅ | Array of state/transition definitions |

---

### `validate_all_bindings`

Validate all property bindings in a StateTree to check for broken or invalid bindings.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |

---

### `get_active_state_tree_status` / `get_current_active_states` / `get_state_execution_history`

PIE debugging tools — inspect running StateTree instances.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | ✅ | Path to the StateTree asset |
| `actor_path` | string | | Path to the actor running the StateTree |
| `max_entries` | number | | Max history entries (default: 100, history only) |

---

### Additional Tools

- **`set_task_required`** — Set whether task failure causes state failure
- **`set_task_properties`** — Set properties on a task in a state
- **`remove_task_from_state`** — Remove a task from a state
- **`remove_evaluator`** / **`set_evaluator_properties`** — Remove or configure evaluators
- **`remove_condition_from_transition`** / **`remove_enter_condition`** — Remove conditions
- **`get_transition_info`** / **`set_transition_properties`** / **`get_transition_conditions`** — Inspect and modify transitions
- **`add_consideration`** — Add utility-based scoring considerations
- **`add_state_event_handler`** — Add event handlers for custom event processing
- **`configure_state_notifications`** — Set up enter/exit notifications
- **`get_linked_state_info`** / **`set_linked_state_parameters`** — Inspect and configure linked states
- **`set_state_selection_weight`** — Configure random selection weights
- **`set_context_requirements`** — Set schema context requirements
- **`get_blueprint_state_tree_types`** — Discover custom Blueprint tasks/conditions/evaluators

## Advanced Usage Patterns

### Building a Complete AI Behavior Tree

```
"Create a full AI behavior StateTree:
1. Create a StateTree 'ST_EnemyAI' with the AI Component schema
2. Add an AI evaluator to expose AI controller data
3. Batch-add states: Root (Group) with children Idle, Patrol, Combat (Group)
4. Add Attack and Defend as children of the Combat group
5. Add a 3-second delay task to Idle
6. Add an EQS query task to Patrol for finding patrol points
7. Add a transition from Idle to Patrol on state completion
8. Add an OnEvent transition from Patrol to Combat triggered by 'AI.Event.PlayerSpotted'
9. Add an enter condition on Combat: check if health is above 20%
10. Add a high-priority OnTick transition to a Flee state when health is critical
11. Bind the evaluator's TargetActor to the EQS task input
12. Compile the StateTree"
```

### Creating a Quest System with Linked StateTrees

```
"Build a modular quest StateTree:
1. Create ST_QuestSystem with Component schema
2. Add a Root group state with MainQuest and SideQuests children
3. Add MainQuest children: QuestIntro, QuestActive, QuestComplete
4. Make QuestActive a LinkedAsset state referencing an external quest-specific StateTree
5. Add gameplay tags 'Quest.MainQuest.Active' and 'Quest.MainQuest.Complete' to the relevant states
6. Add OnEvent transitions triggered by gameplay events ('Quest.ObjectiveComplete')
7. Configure persistence on QuestActive with key 'MainQuest_Progress' for save/load
8. Set up state notifications for enter/exit to trigger UI updates
9. Compile and validate all bindings"
```

### Setting Up Utility-Based AI Decision Making

```
"Create a utility AI StateTree:
1. Create ST_UtilityAI with AI Component schema
2. Add a Decision group state with TrySelectChildrenAtRandom selection
3. Add children: SeekHealth, AttackEnemy, FindCover, Patrol
4. Set selection weights: SeekHealth 2.0 when health low, AttackEnemy 1.5, FindCover 1.0, Patrol 0.5
5. Add considerations for scoring each option based on game state
6. Add enter conditions on SeekHealth: health below 50%
7. Add enter conditions on AttackEnemy: enemy within range
8. Add global tasks for continuous threat assessment
9. Validate bindings and compile"
```

## Best Practices for Natural Language Commands

### Use Batch Operations for Complex Trees
Instead of adding states one at a time, use: *"Batch-add states: Root group with Idle, Patrol, and Combat children"* — this is faster and maintains the hierarchy in a single operation.

### Specify Full Struct Paths for Tasks and Conditions
Instead of: *"Add a delay task"*
Use: *"Add a task with struct path /Script/StateTreeModule.StateTreeDelayTask to the Idle state"* — full paths prevent ambiguity. Use `get_available_tasks` to discover paths.

### Inspect Before Binding Properties
Before creating bindings: *"Get the exposed outputs from the AI evaluator"* and *"Get the bindable inputs for the Patrol state"* to verify property names and types match.

### Compile and Validate Regularly
After significant changes: *"Compile ST_EnemyAI and then validate all bindings"* — catching errors early is much easier than debugging a complex tree after many modifications.

### Use Gameplay Tags for External Communication
Tag states with gameplay tags: *"Add tag 'AI.State.Combat' to the Combat state"* — then external systems can query what state an AI is in without tight coupling.

## Common Use Cases

### AI Behavior Control
Building hierarchical behavior systems for NPCs with state groups for locomotion, combat, and social interactions — driven by evaluators that expose AI controller and perception data.

### Quest and Objective Systems
Creating quest progression trees with linked sub-trees for modular quest content, persistence for save/load, and gameplay tag integration for UI and journal updates.

### Game State Management
Managing game flow states (MainMenu, Loading, Gameplay, Paused, GameOver) with transitions triggered by events and conditions, plus global tasks for persistent logic.

### Dialogue and Interaction Systems
Building branching dialogue trees with states for each conversation node, conditions for player choices, and transitions driven by events from the dialogue UI.

### Spawning and Wave Systems
Creating enemy wave controllers with delay tasks between waves, conditions for wave progression, and linked sub-trees for different wave compositions.

### Tutorial and Onboarding
Designing tutorial flows with sequential states for each step, enter conditions that check player progress, and event-driven advancement when objectives are met.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"State not found"**: State names are case-sensitive. Use `get_state_tree_metadata` to list all states and their exact names, including nested children within group states.
2. **Binding validation failures**: Property names must match exactly between source and target. Use `get_node_exposed_outputs` and `get_node_bindable_inputs` to discover available properties before creating bindings. Type mismatches (e.g., binding Float to Int) also cause failures.
3. **Compilation errors after adding tasks**: Some tasks require specific schema contexts (e.g., AI tasks need `StateTreeAIComponentSchema`). Use `get_available_tasks` to see which tasks are available for your schema type.
4. **Transitions not firing**: Verify the trigger type matches your intent — `OnStateCompleted` only fires when tasks finish, `OnTick` evaluates every frame, and `OnEvent` requires the exact gameplay tag. Check conditions with `get_transition_conditions`.
5. **LinkedAsset state not working**: The linked StateTree must use a compatible schema. Use `get_linked_state_info` to verify the connection, and ensure the external StateTree asset exists at the specified path.

## Performance Considerations

- **Prefer OnStateCompleted and OnEvent over OnTick transitions**: `OnTick` transitions evaluate their conditions every frame, which adds up with complex condition checks. Use event-driven transitions where possible for reactive behavior without per-frame cost.
- **Use batch operations for tree construction**: `batch_add_states` and `batch_add_transitions` are significantly faster than individual calls when building complex hierarchies, reducing round-trip overhead.
- **Limit evaluator count**: Each global evaluator runs every tick regardless of the current state. Keep evaluators minimal — expose only the data that multiple states actually need, and prefer task-local data access for state-specific information.
- **Use LinkedAsset states for modular trees**: Instead of building massive monolithic StateTrees, break behavior into linked sub-trees. This improves compilation time, enables reuse across different AI types, and makes individual trees easier to debug and validate.
