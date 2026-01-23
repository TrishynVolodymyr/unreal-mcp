# StateTree Tools API Reference

Technical documentation for StateTree MCP tools. These tools enable programmatic creation and modification of UE5 StateTree assets for AI behavior trees.

---

## Overview

StateTree tools provide 54 commands across 16 sections for complete StateTree management:

| Section | Commands | Description |
|---------|----------|-------------|
| Tier 1 | 6 | Essential commands (create, add state/transition/task, compile, metadata) |
| Tier 2 | 7 | Advanced commands (conditions, evaluators, parameters, removal, duplicate) |
| Tier 3 | 4 | Introspection (diagnostics, available tasks/conditions/evaluators) |
| Section 1 | 4 | Property binding |
| Section 2 | 2 | Schema/context configuration |
| Section 3 | 1 | Blueprint type support |
| Section 4 | 2 | Global tasks |
| Section 5 | 3 | State completion configuration |
| Section 6 | 2 | Quest persistence |
| Section 7 | 2 | Gameplay tag integration |
| Section 8 | 2 | Runtime inspection (PIE) |
| Section 9 | 1 | Utility AI considerations |
| Section 10 | 4 | Task/evaluator modification |
| Section 11 | 2 | Condition removal |
| Section 12 | 3 | Transition inspection/modification |
| Section 13 | 2 | State event handlers |
| Section 14 | 3 | Linked state configuration |
| Section 15 | 2 | Batch operations |
| Section 16 | 2 | Validation and debugging |

---

## Tier 1 - Essential Commands

### create_state_tree

Create a new StateTree asset.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Name of the StateTree (e.g., "ST_AIBehavior") |
| `path` | string | No | "/Game/AI/StateTrees" | Folder path for the asset |
| `schema_class` | string | No | "StateTreeComponentSchema" | Schema class name |
| `compile_on_creation` | bool | No | false | Whether to compile after creation |

**Schema Options:**
- `StateTreeComponentSchema` - For StateTreeComponent usage (default)
- `StateTreeAIComponentSchema` - For AI controllers

**Response:**
```json
{
  "success": true,
  "name": "ST_AIBehavior",
  "path": "/Game/AI/StateTrees/ST_AIBehavior"
}
```

---

### add_state

Add a state to a StateTree.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | Name of the state (e.g., "Idle", "Patrol") |
| `parent_state_name` | string | No | "" | Parent state name (empty for root) |
| `state_type` | string | No | "State" | Type of state |
| `selection_behavior` | string | No | "TrySelectChildrenInOrder" | Child selection behavior |
| `enabled` | bool | No | true | Whether state is enabled |

**State Types:**
- `State` - Normal state with tasks (default)
- `Group` - Container for child states
- `Linked` - Links to another state
- `LinkedAsset` - Links to external StateTree
- `Subtree` - Nested subtree

**Selection Behaviors:**
- `TrySelectChildrenInOrder` (default)
- `TrySelectChildrenAtRandom`
- `None`

---

### add_transition

Add a transition between states.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | Name of source state |
| `trigger` | string | No | "OnStateCompleted" | When transition evaluates |
| `target_state_name` | string | No | "" | Destination state (for GotoState) |
| `transition_type` | string | No | "GotoState" | What happens when transition fires |
| `event_tag` | string | No | "" | Gameplay tag for OnEvent trigger |
| `priority` | string | No | "Normal" | Transition priority |
| `delay_transition` | bool | No | false | Whether to delay transition |
| `delay_duration` | float | No | 0.0 | Delay time in seconds |

**Trigger Types:**
- `OnStateCompleted` - When state finishes successfully (default)
- `OnStateFailed` - When state fails
- `OnEvent` - When a gameplay event occurs (requires event_tag)
- `OnTick` - Every tick (use with conditions)

**Transition Types:**
- `GotoState` - Go to target state (default)
- `NextState` - Go to next sibling state
- `Succeeded` - Mark tree as succeeded
- `Failed` - Mark tree as failed

**Priority Levels:**
- `Low`, `Normal` (default), `High`, `Critical`

---

### add_task_to_state

Add a task to a state.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | Name of the state |
| `task_struct_path` | string | Yes | - | Full path to the task struct |
| `task_name` | string | No | "" | Display name for the task |
| `task_properties` | object | No | null | Task-specific properties |

**Common Task Struct Paths:**
- `/Script/StateTreeModule.StateTreeDelayTask` - Wait for duration
- `/Script/StateTreeModule.StateTreeRunParallelStateTreeTask` - Run parallel tree
- `/Script/GameplayStateTreeModule.StateTreeRunEnvQueryTask` - EQS query
- `/Script/GameplayStateTreeModule.StateTreeBlueprintPropertyRefTask` - Blueprint task

---

### compile_state_tree

Compile a StateTree for runtime use.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |

---

### get_state_tree_metadata

Get metadata from a StateTree including structure, states, tasks, and transitions.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |

**Response:**
```json
{
  "success": true,
  "metadata": {
    "name": "ST_AIBehavior",
    "path": "/Game/AI/StateTrees/ST_AIBehavior",
    "schema": "StateTreeComponentSchema",
    "evaluators": [...],
    "states": [
      {
        "name": "Root",
        "id": "GUID...",
        "enabled": true,
        "type": "State",
        "selection_behavior": "TrySelectChildrenInOrder",
        "tasks": [...],
        "transitions": [...],
        "children": [...]
      }
    ]
  }
}
```

---

## Tier 2 - Advanced Commands

### add_condition_to_transition

Add a condition to a transition.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | State containing the transition |
| `condition_struct_path` | string | Yes | - | Full path to condition struct |
| `transition_index` | int | No | 0 | Which transition (0-based) |
| `combine_mode` | string | No | "And" | How to combine conditions |
| `condition_properties` | object | No | null | Condition-specific properties |

**Common Condition Struct Paths:**
- `/Script/StateTreeModule.StateTreeCompareIntCondition`
- `/Script/StateTreeModule.StateTreeCompareFloatCondition`
- `/Script/StateTreeModule.StateTreeCompareBoolCondition`
- `/Script/GameplayStateTreeModule.StateTreeGameplayTagCondition`

**Combine Modes:**
- `And` - All conditions must be true (default)
- `Or` - Any condition can be true

---

### add_enter_condition

Add an enter condition to a state.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | Name of the state |
| `condition_struct_path` | string | Yes | - | Full path to condition struct |
| `condition_properties` | object | No | null | Condition-specific properties |

---

### add_evaluator

Add a global evaluator to a StateTree.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `evaluator_struct_path` | string | Yes | - | Full path to evaluator struct |
| `evaluator_name` | string | No | "" | Display name for the evaluator |
| `evaluator_properties` | object | No | null | Evaluator-specific properties |

**Common Evaluator Struct Paths:**
- `/Script/StateTreeModule.StateTreeObjectPropertyEvaluator`
- `/Script/GameplayStateTreeModule.StateTreeActorEvaluator`
- `/Script/GameplayStateTreeModule.StateTreeAIEvaluator`

---

### set_state_parameters

Set parameters on a state.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |
| `state_name` | string | Yes | Name of the state |
| `parameters` | object | Yes | Parameters to set |

---

### remove_state

Remove a state from a StateTree.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | Name of the state |
| `remove_children` | bool | No | true | Remove child states recursively |

---

### remove_transition

Remove a transition from a state.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | State containing the transition |
| `transition_index` | int | No | 0 | Index of transition (0-based) |

---

### duplicate_state_tree

Duplicate an existing StateTree asset.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `source_path` | string | Yes | Path to source StateTree |
| `dest_path` | string | Yes | Destination folder path |
| `new_name` | string | Yes | Name for the duplicate |

---

## Tier 3 - Introspection Commands

### get_state_tree_diagnostics

Get validation/diagnostic information.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |

**Response:**
```json
{
  "success": true,
  "diagnostics": {
    "name": "ST_AIBehavior",
    "is_valid": true,
    "messages": [...],
    "state_count": 5,
    "task_count": 3,
    "transition_count": 4,
    "evaluator_count": 1
  }
}
```

---

### get_available_tasks

Get all available StateTree task types.

**Response:**
```json
{
  "success": true,
  "tasks": [
    {"path": "/Script/StateTreeModule.StateTreeDelayTask", "name": "Delay Task"},
    ...
  ],
  "count": 25
}
```

---

### get_available_conditions

Get all available StateTree condition types.

---

### get_available_evaluators

Get all available StateTree evaluator types.

---

## Section 1 - Property Binding

### bind_state_tree_property

Bind a property from one node to another.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_node_name` | string | Yes | - | Source node (evaluator name or "Context") |
| `source_property_name` | string | Yes | - | Property to bind from |
| `target_node_name` | string | Yes | - | Target node (state name) |
| `target_property_name` | string | Yes | - | Property to bind to |
| `task_index` | int | No | -1 | Task index if binding to specific task |

---

### remove_state_tree_binding

Remove a property binding from a target node.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `target_node_name` | string | Yes | - | Target node (state name or evaluator name) |
| `target_property_name` | string | Yes | - | Property with binding to remove |
| `task_index` | int | No | -1 | Task index if removing from specific task |
| `transition_index` | int | No | -1 | Transition index if removing from condition |
| `condition_index` | int | No | -1 | Condition index within transition |

---

### get_node_bindable_inputs

Get bindable input properties for a node.

---

### get_node_exposed_outputs

Get exposed output properties from a node.

---

## Section 10 - Task/Evaluator Modification

### remove_task_from_state

Remove a task from a state.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | State containing the task |
| `task_index` | int | No | 0 | Index of task (0-based) |

---

### set_task_properties

Set properties on a task.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | State containing the task |
| `task_index` | int | No | 0 | Index of task (0-based) |
| `properties` | object | No | null | Properties to set |

---

### remove_evaluator

Remove an evaluator from a StateTree.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `evaluator_index` | int | No | 0 | Index of evaluator (0-based) |

---

### set_evaluator_properties

Set properties on an evaluator.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `evaluator_index` | int | No | 0 | Index of evaluator (0-based) |
| `properties` | object | No | null | Properties to set |

---

## Section 11 - Condition Removal

### remove_condition_from_transition

Remove a condition from a transition.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | State containing the transition |
| `transition_index` | int | No | 0 | Index of transition (0-based) |
| `condition_index` | int | No | 0 | Index of condition (0-based) |

---

### remove_enter_condition

Remove an enter condition from a state.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `state_name` | string | Yes | - | State containing the condition |
| `condition_index` | int | No | 0 | Index of condition (0-based) |

---

## Section 12 - Transition Inspection/Modification

### get_transition_info

Get detailed information about a transition.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | State containing the transition |
| `transition_index` | int | No | 0 | Index of transition (0-based) |

---

### set_transition_properties

Set properties on a transition. Only provided properties are changed.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | State containing the transition |
| `transition_index` | int | No | 0 | Index of transition (0-based) |
| `target_state_name` | string | No | null | New target state |
| `trigger` | string | No | null | New trigger type |
| `transition_type` | string | No | null | New transition type |
| `priority` | string | No | null | New priority |
| `delay_transition` | bool | No | null | Whether to delay |
| `delay_duration` | float | No | null | Delay duration |

---

### get_transition_conditions

Get all conditions on a transition.

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `source_state_name` | string | Yes | - | State containing the transition |
| `transition_index` | int | No | 0 | Index of transition (0-based) |

---

## Section 15 - Batch Operations

### batch_add_states

Add multiple states in a single operation.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |
| `states` | array | Yes | Array of state definitions |

**State Definition:**
```json
{
  "state_name": "Patrol",
  "parent_state_name": "Root",
  "state_type": "State",
  "selection_behavior": "TrySelectChildrenInOrder",
  "enabled": true
}
```

---

### batch_add_transitions

Add multiple transitions in a single operation.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |
| `transitions` | array | Yes | Array of transition definitions |

**Transition Definition:**
```json
{
  "source_state_name": "Idle",
  "target_state_name": "Patrol",
  "trigger": "OnStateCompleted",
  "transition_type": "GotoState",
  "priority": "Normal"
}
```

---

## Section 16 - Validation and Debugging

### validate_all_bindings

Validate all property bindings in a StateTree.

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `state_tree_path` | string | Yes | Path to the StateTree asset |

**Response:**
```json
{
  "success": true,
  "validation_results": {
    "total_bindings": 10,
    "valid_bindings": 9,
    "invalid_bindings": 1,
    "errors": [...],
    "warnings": [...]
  }
}
```

---

### get_state_execution_history

Get state execution history from a running StateTree (PIE debugging).

**Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `state_tree_path` | string | Yes | - | Path to the StateTree asset |
| `actor_path` | string | No | "" | Actor running the StateTree |
| `max_entries` | int | No | 100 | Maximum history entries |

**Response:**
```json
{
  "success": true,
  "execution_history": {
    "state_tree_path": "/Game/AI/ST_AIBehavior",
    "actor_path": "/Game/Levels/TestMap.TestMap:PersistentLevel.BP_AICharacter_0",
    "entries": [
      {
        "state_name": "Idle",
        "timestamp": "2024-01-15T10:30:00",
        "duration": 2.5,
        "exit_reason": "OnStateCompleted"
      }
    ]
  }
}
```

---

## Example Workflows

### Create Basic AI Behavior Tree

```python
# 1. Create StateTree
await create_state_tree(name="ST_BasicAI", path="/Game/AI/StateTrees")

# 2. Add states
await add_state(state_tree_path="/Game/AI/StateTrees/ST_BasicAI", state_name="Root")
await add_state(state_tree_path="/Game/AI/StateTrees/ST_BasicAI", state_name="Idle", parent_state_name="Root")
await add_state(state_tree_path="/Game/AI/StateTrees/ST_BasicAI", state_name="Patrol", parent_state_name="Root")

# 3. Add delay task to Idle
await add_task_to_state(
    state_tree_path="/Game/AI/StateTrees/ST_BasicAI",
    state_name="Idle",
    task_struct_path="/Script/StateTreeModule.StateTreeDelayTask",
    task_properties={"Duration": 3.0}
)

# 4. Add transition Idle -> Patrol
await add_transition(
    state_tree_path="/Game/AI/StateTrees/ST_BasicAI",
    source_state_name="Idle",
    trigger="OnStateCompleted",
    target_state_name="Patrol"
)

# 5. Compile
await compile_state_tree(state_tree_path="/Game/AI/StateTrees/ST_BasicAI")
```

### Batch Create Complex State Hierarchy

```python
# Create many states at once
await batch_add_states(
    state_tree_path="/Game/AI/StateTrees/ST_ComplexAI",
    states=[
        {"state_name": "Root"},
        {"state_name": "Combat", "parent_state_name": "Root", "state_type": "Group"},
        {"state_name": "Attack", "parent_state_name": "Combat"},
        {"state_name": "Defend", "parent_state_name": "Combat"},
        {"state_name": "Exploration", "parent_state_name": "Root", "state_type": "Group"},
        {"state_name": "Patrol", "parent_state_name": "Exploration"},
        {"state_name": "Investigate", "parent_state_name": "Exploration"}
    ]
)

# Create transitions
await batch_add_transitions(
    state_tree_path="/Game/AI/StateTrees/ST_ComplexAI",
    transitions=[
        {"source_state_name": "Patrol", "target_state_name": "Investigate", "trigger": "OnEvent", "event_tag": "AI.Event.NoiseHeard"},
        {"source_state_name": "Investigate", "target_state_name": "Attack", "trigger": "OnEvent", "event_tag": "AI.Event.EnemySpotted"},
        {"source_state_name": "Attack", "target_state_name": "Defend", "trigger": "OnEvent", "event_tag": "AI.Event.LowHealth"}
    ]
)
```

---

## Related Documentation

- **[Architecture Guide](../../MCPGameProject/Plugins/UnrealMCP/Documentation/Architecture_Guide.md)** - C++ plugin architecture
- **[CLAUDE.md](../../CLAUDE.md)** - Development workflow and guidelines
- **[UE5 StateTree Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/state-tree-in-unreal-engine)** - Official Unreal docs
