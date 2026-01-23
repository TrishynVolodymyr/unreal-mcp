#!/usr/bin/env python3
"""
StateTree MCP Server

This server provides MCP tools for StateTree operations in Unreal Engine,
enabling AI assistants to programmatically create, modify, and manage
StateTree assets for AI behavior trees.
"""

import asyncio
import json
from typing import Any, Dict, List

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("StateTree MCP Server")

# TCP connection settings
TCP_HOST = "127.0.0.1"
TCP_PORT = 55557


async def send_tcp_command(command_type: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to the Unreal Engine TCP server."""
    try:
        # Create the command payload
        command_data = {
            "type": command_type,
            "params": params
        }

        # Convert to JSON string
        json_data = json.dumps(command_data)

        # Connect to TCP server
        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)

        # Send command
        writer.write(json_data.encode('utf-8'))
        writer.write(b'\n')  # Add newline delimiter
        await writer.drain()

        # Read response
        response_data = await reader.read(49152)  # Read up to 48KB
        response_str = response_data.decode('utf-8').strip()

        # Close connection
        writer.close()
        await writer.wait_closed()

        # Parse JSON response
        if response_str:
            try:
                return json.loads(response_str)
            except json.JSONDecodeError as json_err:
                return {"success": False, "error": f"JSON decode error: {str(json_err)}, Response: {response_str[:200]}"}
        else:
            return {"success": False, "error": "Empty response from server"}

    except Exception as e:
        return {"success": False, "error": f"TCP communication error: {str(e)}"}


# ============================================================================
# Tier 1 - Essential Commands
# ============================================================================

@app.tool()
async def create_state_tree(
    name: str,
    path: str = "/Game/AI/StateTrees",
    schema_class: str = "StateTreeComponentSchema",
    compile_on_creation: bool = False
) -> Dict[str, Any]:
    """
    Create a new StateTree asset.

    Args:
        name: Name of the StateTree to create (e.g., "ST_AIBehavior")
        path: Folder path where the StateTree should be created
        schema_class: Schema class name. Options:
            - "StateTreeComponentSchema" (default) - For StateTreeComponent usage
            - "StateTreeAIComponentSchema" - For AI controllers
        compile_on_creation: Whether to compile after creation (default: False)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - name: Name of the created StateTree
        - path: Full path to the created asset
    """
    params = {
        "name": name,
        "path": path,
        "schema_class": schema_class,
        "compile_on_creation": compile_on_creation
    }
    return await send_tcp_command("create_state_tree", params)


@app.tool()
async def add_state(
    state_tree_path: str,
    state_name: str,
    parent_state_name: str = "",
    state_type: str = "State",
    selection_behavior: str = "TrySelectChildrenInOrder",
    enabled: bool = True
) -> Dict[str, Any]:
    """
    Add a state to a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to create (e.g., "Idle", "Patrol", "Combat")
        parent_state_name: Name of the parent state (empty for root-level state)
        state_type: Type of state:
            - "State" (default) - Normal state with tasks
            - "Group" - Container for child states
            - "Linked" - Links to another state
            - "LinkedAsset" - Links to external StateTree
            - "Subtree" - Nested subtree
        selection_behavior: How to select child states:
            - "TrySelectChildrenInOrder" (default)
            - "TrySelectChildrenAtRandom"
            - "None"
        enabled: Whether the state is enabled (default: True)

    Returns:
        Dictionary containing:
        - success: Whether state was added successfully
        - state_name: Name of the added state
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "state_type": state_type,
        "selection_behavior": selection_behavior,
        "enabled": enabled
    }
    if parent_state_name:
        params["parent_state_name"] = parent_state_name

    return await send_tcp_command("add_state", params)


@app.tool()
async def add_transition(
    state_tree_path: str,
    source_state_name: str,
    trigger: str = "OnStateCompleted",
    target_state_name: str = "",
    transition_type: str = "GotoState",
    event_tag: str = "",
    priority: str = "Normal",
    delay_transition: bool = False,
    delay_duration: float = 0.0
) -> Dict[str, Any]:
    """
    Add a transition between states in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the source state
        trigger: When the transition is evaluated:
            - "OnStateCompleted" (default) - When state finishes successfully
            - "OnStateFailed" - When state fails
            - "OnEvent" - When a gameplay event occurs (requires event_tag)
            - "OnTick" - Every tick (use with conditions)
        target_state_name: Name of the destination state (required for GotoState)
        transition_type: What happens when transition fires:
            - "GotoState" (default) - Go to target state
            - "NextState" - Go to next sibling state
            - "Succeeded" - Mark tree as succeeded
            - "Failed" - Mark tree as failed
        event_tag: Gameplay tag for OnEvent trigger (e.g., "AI.Event.PlayerSpotted")
        priority: Transition priority:
            - "Low", "Normal" (default), "High", "Critical"
        delay_transition: Whether to delay the transition
        delay_duration: Delay time in seconds

    Returns:
        Dictionary containing:
        - success: Whether transition was added successfully
        - source_state: Source state name
        - target_state: Target state name
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "trigger": trigger,
        "transition_type": transition_type,
        "priority": priority,
        "delay_transition": delay_transition,
        "delay_duration": delay_duration
    }
    if target_state_name:
        params["target_state_name"] = target_state_name
    if event_tag:
        params["event_tag"] = event_tag

    return await send_tcp_command("add_transition", params)


@app.tool()
async def add_task_to_state(
    state_tree_path: str,
    state_name: str,
    task_struct_path: str,
    task_name: str = "",
    task_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add a task to a state in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to add the task to
        task_struct_path: Full path to the task struct. Common tasks:
            - "/Script/StateTreeModule.StateTreeDelayTask" - Wait for duration
            - "/Script/StateTreeModule.StateTreeRunParallelStateTreeTask" - Run parallel tree
            - "/Script/GameplayStateTreeModule.StateTreeRunEnvQueryTask" - EQS query
            - "/Script/GameplayStateTreeModule.StateTreeBlueprintPropertyRefTask" - Blueprint task
        task_name: Optional display name for the task instance
        task_properties: Optional dict of task-specific properties

    Returns:
        Dictionary containing:
        - success: Whether task was added successfully
        - state_name: Name of the state
        - task_type: Type of task added
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "task_struct_path": task_struct_path
    }
    if task_name:
        params["task_name"] = task_name
    if task_properties:
        params["task_properties"] = task_properties

    return await send_tcp_command("add_task_to_state", params)


@app.tool()
async def compile_state_tree(state_tree_path: str) -> Dict[str, Any]:
    """
    Compile a StateTree for runtime use.

    This validates the StateTree structure and prepares it for execution.
    Should be called after making changes to ensure the tree is valid.

    Args:
        state_tree_path: Path to the StateTree asset

    Returns:
        Dictionary containing:
        - success: Whether compilation was successful
        - state_tree_name: Name of the compiled StateTree
        - message: Success/error message
    """
    params = {"state_tree_path": state_tree_path}
    return await send_tcp_command("compile_state_tree", params)


@app.tool()
async def get_state_tree_metadata(state_tree_path: str) -> Dict[str, Any]:
    """
    Get metadata from a StateTree including its structure, states, tasks, and transitions.

    Args:
        state_tree_path: Path to the StateTree asset

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - metadata: Object containing:
            - name: StateTree name
            - path: Full asset path
            - schema: Schema class name
            - evaluators: Array of global evaluators
            - states: Hierarchical array of states, each with:
                - name: State name
                - id: State GUID
                - enabled: Whether state is enabled
                - type: State type (State, Group, etc.)
                - selection_behavior: Child selection behavior
                - tasks: Array of tasks in this state
                - transitions: Array of transitions from this state
                - children: Nested child states
    """
    params = {"state_tree_path": state_tree_path}
    return await send_tcp_command("get_state_tree_metadata", params)


# ============================================================================
# Tier 2 - Advanced Commands
# ============================================================================

@app.tool()
async def add_condition_to_transition(
    state_tree_path: str,
    source_state_name: str,
    condition_struct_path: str,
    transition_index: int = 0,
    combine_mode: str = "And",
    condition_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add a condition to a transition in a StateTree.

    Conditions determine whether a transition can fire. Multiple conditions
    can be combined using And/Or logic.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the state containing the transition
        condition_struct_path: Full path to the condition struct. Common conditions:
            - "/Script/StateTreeModule.StateTreeCompareIntCondition" - Compare integers
            - "/Script/StateTreeModule.StateTreeCompareFloatCondition" - Compare floats
            - "/Script/StateTreeModule.StateTreeCompareBoolCondition" - Check boolean
            - "/Script/StateTreeModule.StateTreeCompareEnumCondition" - Compare enums
            - "/Script/GameplayStateTreeModule.StateTreeGameplayTagCondition" - Check tags
        transition_index: Which transition to add the condition to (0-based)
        combine_mode: How to combine with existing conditions:
            - "And" (default) - All conditions must be true
            - "Or" - Any condition can be true
        condition_properties: Optional dict of condition-specific properties

    Returns:
        Dictionary containing:
        - success: Whether condition was added successfully
        - state_name: State containing the transition
        - transition_index: Index of the modified transition
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "condition_struct_path": condition_struct_path,
        "transition_index": transition_index,
        "combine_mode": combine_mode
    }
    if condition_properties:
        params["condition_properties"] = condition_properties

    return await send_tcp_command("add_condition_to_transition", params)


@app.tool()
async def add_enter_condition(
    state_tree_path: str,
    state_name: str,
    condition_struct_path: str,
    condition_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add an enter condition to a state in a StateTree.

    Enter conditions determine whether a state can be selected for execution.
    The state will be skipped if enter conditions are not met.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to add the enter condition to
        condition_struct_path: Full path to the condition struct
        condition_properties: Optional dict of condition-specific properties

    Returns:
        Dictionary containing:
        - success: Whether condition was added successfully
        - state_name: State with the new enter condition
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "condition_struct_path": condition_struct_path
    }
    if condition_properties:
        params["condition_properties"] = condition_properties

    return await send_tcp_command("add_enter_condition", params)


@app.tool()
async def add_evaluator(
    state_tree_path: str,
    evaluator_struct_path: str,
    evaluator_name: str = "",
    evaluator_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add a global evaluator to a StateTree.

    Evaluators run every tick and can expose data to all states and tasks.
    Use them for commonly needed runtime data like target actors, distances, etc.

    Args:
        state_tree_path: Path to the StateTree asset
        evaluator_struct_path: Full path to the evaluator struct. Common evaluators:
            - "/Script/StateTreeModule.StateTreeObjectPropertyEvaluator" - Expose object
            - "/Script/GameplayStateTreeModule.StateTreeActorEvaluator" - Actor info
            - "/Script/GameplayStateTreeModule.StateTreeAIEvaluator" - AI controller data
        evaluator_name: Optional display name for the evaluator instance
        evaluator_properties: Optional dict of evaluator-specific properties

    Returns:
        Dictionary containing:
        - success: Whether evaluator was added successfully
        - evaluator_type: Type of evaluator added
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "evaluator_struct_path": evaluator_struct_path
    }
    if evaluator_name:
        params["evaluator_name"] = evaluator_name
    if evaluator_properties:
        params["evaluator_properties"] = evaluator_properties

    return await send_tcp_command("add_evaluator", params)


@app.tool()
async def set_state_parameters(
    state_tree_path: str,
    state_name: str,
    parameters: Dict[str, Any]
) -> Dict[str, Any]:
    """
    Set parameters on a state in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to modify
        parameters: Dict of parameters to set. Common parameters:
            - "name": New name for the state
            - "enabled": Whether state is enabled (bool)
            - "state_type": State type string
            - "selection_behavior": Child selection behavior

    Returns:
        Dictionary containing:
        - success: Whether parameters were set successfully
        - state_name: Name of the modified state
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "parameters": parameters
    }
    return await send_tcp_command("set_state_parameters", params)


@app.tool()
async def remove_state(
    state_tree_path: str,
    state_name: str,
    remove_children: bool = True
) -> Dict[str, Any]:
    """
    Remove a state from a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to remove
        remove_children: Whether to remove child states recursively (default: True)

    Returns:
        Dictionary containing:
        - success: Whether state was removed successfully
        - state_name: Name of the removed state
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "remove_children": remove_children
    }
    return await send_tcp_command("remove_state", params)


@app.tool()
async def remove_transition(
    state_tree_path: str,
    source_state_name: str,
    transition_index: int = 0
) -> Dict[str, Any]:
    """
    Remove a transition from a state in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the state containing the transition
        transition_index: Index of the transition to remove (0-based)

    Returns:
        Dictionary containing:
        - success: Whether transition was removed successfully
        - state_name: State that contained the transition
        - transition_index: Index of the removed transition
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "transition_index": transition_index
    }
    return await send_tcp_command("remove_transition", params)


@app.tool()
async def duplicate_state_tree(
    source_path: str,
    dest_path: str,
    new_name: str
) -> Dict[str, Any]:
    """
    Duplicate an existing StateTree asset.

    Args:
        source_path: Path to the source StateTree asset
        dest_path: Destination folder path
        new_name: Name for the duplicated StateTree

    Returns:
        Dictionary containing:
        - success: Whether duplication was successful
        - name: Name of the duplicated StateTree
        - path: Full path to the new asset
        - message: Success/error message
    """
    params = {
        "source_path": source_path,
        "dest_path": dest_path,
        "new_name": new_name
    }
    return await send_tcp_command("duplicate_state_tree", params)


# ============================================================================
# Tier 3 - Introspection Commands
# ============================================================================

@app.tool()
async def get_state_tree_diagnostics(state_tree_path: str) -> Dict[str, Any]:
    """
    Get validation/diagnostic information from a StateTree.

    Use this to check for compilation errors, warnings, and get statistics
    about the StateTree structure.

    Args:
        state_tree_path: Path to the StateTree asset

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - diagnostics: Object containing:
            - name: StateTree name
            - is_valid: Whether the tree compiles successfully
            - messages: Array of diagnostic messages
            - state_count: Number of states
            - task_count: Number of tasks
            - transition_count: Number of transitions
            - evaluator_count: Number of evaluators
    """
    params = {"state_tree_path": state_tree_path}
    return await send_tcp_command("get_state_tree_diagnostics", params)


@app.tool()
async def get_available_tasks() -> Dict[str, Any]:
    """
    Get a list of all available StateTree task types.

    Use this to discover what tasks can be added to states. The returned
    struct paths can be used with add_task_to_state.

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - tasks: Array of available tasks, each with:
            - path: Full struct path (use this with add_task_to_state)
            - name: Display name
        - count: Total number of available tasks
    """
    return await send_tcp_command("get_available_tasks", {})


@app.tool()
async def get_available_conditions() -> Dict[str, Any]:
    """
    Get a list of all available StateTree condition types.

    Use this to discover what conditions can be added to transitions and states.
    The returned struct paths can be used with add_condition_to_transition
    and add_enter_condition.

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - conditions: Array of available conditions, each with:
            - path: Full struct path
            - name: Display name
        - count: Total number of available conditions
    """
    return await send_tcp_command("get_available_conditions", {})


@app.tool()
async def get_available_evaluators() -> Dict[str, Any]:
    """
    Get a list of all available StateTree evaluator types.

    Use this to discover what evaluators can be added to the StateTree.
    The returned struct paths can be used with add_evaluator.

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - evaluators: Array of available evaluators, each with:
            - path: Full struct path
            - name: Display name
        - count: Total number of available evaluators
    """
    return await send_tcp_command("get_available_evaluators", {})


# ============================================================================
# Section 1 - Property Binding Commands
# ============================================================================

@app.tool()
async def bind_state_tree_property(
    state_tree_path: str,
    source_node_name: str,
    source_property_name: str,
    target_node_name: str,
    target_property_name: str,
    task_index: int = -1,
    transition_index: int = -1,
    condition_index: int = -1
) -> Dict[str, Any]:
    """
    Bind a property from one node to another in a StateTree.

    Property bindings allow data to flow between evaluators, tasks, and context.
    This is essential for passing runtime data to task inputs.

    Args:
        state_tree_path: Path to the StateTree asset
        source_node_name: Source node identifier (evaluator name or "Context" for schema context)
        source_property_name: Property name on the source node to bind from
        target_node_name: Target node identifier (state name for conditions, or state name for tasks)
        target_property_name: Property name on the target node to bind to
        task_index: Index of task within state if binding to a specific task (-1 to ignore)
        transition_index: Index of transition within state if binding to a condition (-1 to ignore)
        condition_index: Index of condition within transition if binding to a condition (-1 to ignore)

    Returns:
        Dictionary containing:
        - success: Whether binding was successful
        - message: Success/error message

    Examples:
        # Bind to a task (task_index >= 0)
        bind_state_tree_property(..., task_index=0)

        # Bind to a condition on a transition (transition_index >= 0 and condition_index >= 0)
        bind_state_tree_property(..., transition_index=0, condition_index=0)
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_node_name": source_node_name,
        "source_property_name": source_property_name,
        "target_node_name": target_node_name,
        "target_property_name": target_property_name,
        "task_index": task_index,
        "transition_index": transition_index,
        "condition_index": condition_index
    }
    return await send_tcp_command("bind_state_tree_property", params)


@app.tool()
async def remove_state_tree_binding(
    state_tree_path: str,
    target_node_name: str,
    target_property_name: str,
    task_index: int = -1,
    transition_index: int = -1,
    condition_index: int = -1
) -> Dict[str, Any]:
    """
    Remove a property binding from a target node in a StateTree.

    Use this to clear bindings that are invalid or no longer needed.

    Args:
        state_tree_path: Path to the StateTree asset
        target_node_name: Target node identifier (state name or evaluator name)
        target_property_name: Property name on the target node with the binding to remove
        task_index: Index of task within state (-1 to ignore, default)
        transition_index: Index of transition (-1 to ignore, default)
        condition_index: Index of condition within transition (-1 to ignore, default)

    Returns:
        Dictionary containing:
        - success: Whether binding was removed successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "target_node_name": target_node_name,
        "target_property_name": target_property_name,
        "task_index": task_index,
        "transition_index": transition_index,
        "condition_index": condition_index
    }
    return await send_tcp_command("remove_state_tree_binding", params)


@app.tool()
async def get_node_bindable_inputs(
    state_tree_path: str,
    node_identifier: str,
    task_index: int = -1
) -> Dict[str, Any]:
    """
    Get bindable input properties for a node in a StateTree.

    Use this to discover what properties can be bound on tasks, states, or evaluators.

    Args:
        state_tree_path: Path to the StateTree asset
        node_identifier: Node identifier (state name, evaluator name, etc.)
        task_index: Task index if querying a specific task (-1 for state/evaluator level)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - data: Object containing:
            - node: Node identifier
            - task_index: Task index
            - inputs: Array of bindable inputs, each with name and type
    """
    params = {
        "state_tree_path": state_tree_path,
        "node_identifier": node_identifier,
        "task_index": task_index
    }
    return await send_tcp_command("get_node_bindable_inputs", params)


@app.tool()
async def get_node_exposed_outputs(
    state_tree_path: str,
    node_identifier: str
) -> Dict[str, Any]:
    """
    Get exposed output properties from a node (evaluator or context).

    Use this to discover what properties can be bound FROM an evaluator or context.

    Args:
        state_tree_path: Path to the StateTree asset
        node_identifier: Node identifier (evaluator name or "Context" for schema context)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - data: Object containing:
            - node: Node identifier
            - schema: Schema name if querying Context
            - outputs: Array of exposed outputs, each with name and type
    """
    params = {
        "state_tree_path": state_tree_path,
        "node_identifier": node_identifier
    }
    return await send_tcp_command("get_node_exposed_outputs", params)


# ============================================================================
# Section 2 - Schema/Context Configuration Commands
# ============================================================================

@app.tool()
async def get_schema_context_properties(state_tree_path: str) -> Dict[str, Any]:
    """
    Get schema context properties available in a StateTree.

    The schema defines what context data is available (e.g., Actor, AIController).
    Use this to understand what bindings are possible from the context.

    Args:
        state_tree_path: Path to the StateTree asset

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - data: Object containing:
            - schema_class: Name of the schema class
            - schema_path: Path to the schema class
            - context_properties: Array of available context properties
    """
    params = {"state_tree_path": state_tree_path}
    return await send_tcp_command("get_schema_context_properties", params)


@app.tool()
async def set_context_requirements(
    state_tree_path: str,
    requirements: Dict[str, Any]
) -> Dict[str, Any]:
    """
    Set context requirements for a StateTree schema.

    Context requirements define what data the StateTree expects at runtime.

    Args:
        state_tree_path: Path to the StateTree asset
        requirements: Dict of requirement settings (schema-specific)

    Returns:
        Dictionary containing:
        - success: Whether requirements were set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "requirements": requirements
    }
    return await send_tcp_command("set_context_requirements", params)


# ============================================================================
# Section 3 - Blueprint Type Support
# ============================================================================

@app.tool()
async def get_blueprint_state_tree_types() -> Dict[str, Any]:
    """
    Get Blueprint-based StateTree types (tasks, conditions, evaluators) in the project.

    Use this to discover custom Blueprint tasks/conditions/evaluators that can be used
    in addition to the built-in C++ types.

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - data: Object containing:
            - blueprint_tasks: Array of Blueprint task types
            - blueprint_conditions: Array of Blueprint condition types
            - blueprint_evaluators: Array of Blueprint evaluator types
    """
    return await send_tcp_command("get_blueprint_state_tree_types", {})


# ============================================================================
# Section 4 - Global Tasks Commands
# ============================================================================

@app.tool()
async def add_global_task(
    state_tree_path: str,
    task_struct_path: str,
    task_name: str = "",
    task_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add a global task to a StateTree.

    Global tasks run at the tree level, independent of the current state.
    Use them for persistent behaviors like monitoring, logging, or resource management.

    Args:
        state_tree_path: Path to the StateTree asset
        task_struct_path: Full path to the task struct
        task_name: Optional display name for the task instance
        task_properties: Optional dict of task-specific properties

    Returns:
        Dictionary containing:
        - success: Whether task was added successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "task_struct_path": task_struct_path
    }
    if task_name:
        params["task_name"] = task_name
    if task_properties:
        params["task_properties"] = task_properties

    return await send_tcp_command("add_global_task", params)


@app.tool()
async def remove_global_task(
    state_tree_path: str,
    task_index: int = 0
) -> Dict[str, Any]:
    """
    Remove a global task from a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        task_index: Index of the global task to remove (0-based)

    Returns:
        Dictionary containing:
        - success: Whether task was removed successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "task_index": task_index
    }
    return await send_tcp_command("remove_global_task", params)


# ============================================================================
# Section 5 - State Completion Configuration Commands
# ============================================================================

@app.tool()
async def set_state_completion_mode(
    state_tree_path: str,
    state_name: str,
    completion_mode: str = "AllTasks"
) -> Dict[str, Any]:
    """
    Set how a state determines completion.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to configure
        completion_mode: How the state determines completion:
            - "AllTasks" (default) - Complete when all tasks complete
            - "AnyTask" - Complete when any task completes
            - "Explicit" - Only complete via explicit transition

    Returns:
        Dictionary containing:
        - success: Whether mode was set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "completion_mode": completion_mode
    }
    return await send_tcp_command("set_state_completion_mode", params)


@app.tool()
async def set_task_required(
    state_tree_path: str,
    state_name: str,
    task_index: int = 0,
    required: bool = True
) -> Dict[str, Any]:
    """
    Set whether a task is required (failure causes state failure).

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state containing the task
        task_index: Index of the task within the state (0-based)
        required: Whether task failure should cause state failure (default: True)

    Returns:
        Dictionary containing:
        - success: Whether setting was successful
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "task_index": task_index,
        "required": required
    }
    return await send_tcp_command("set_task_required", params)


@app.tool()
async def set_linked_state_asset(
    state_tree_path: str,
    state_name: str,
    linked_asset_path: str
) -> Dict[str, Any]:
    """
    Set the linked asset for a LinkedAsset state type.

    Use this to connect a LinkedAsset state to an external StateTree.
    The state must have been created with state_type="LinkedAsset".

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the LinkedAsset state to configure
        linked_asset_path: Path to the external StateTree asset to link

    Returns:
        Dictionary containing:
        - success: Whether asset was linked successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "linked_asset_path": linked_asset_path
    }
    return await send_tcp_command("set_linked_state_asset", params)


# ============================================================================
# Section 6 - Quest Persistence Commands
# ============================================================================

@app.tool()
async def configure_state_persistence(
    state_tree_path: str,
    state_name: str,
    persistent: bool = True,
    persistence_key: str = ""
) -> Dict[str, Any]:
    """
    Configure persistence settings for a state.

    Use this to mark states that should be saved/loaded for quest systems
    or other persistent behaviors.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to configure
        persistent: Whether this state should be persisted (default: True)
        persistence_key: Optional key for save/load identification

    Returns:
        Dictionary containing:
        - success: Whether configuration was successful
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "persistent": persistent
    }
    if persistence_key:
        params["persistence_key"] = persistence_key

    return await send_tcp_command("configure_state_persistence", params)


@app.tool()
async def get_persistent_state_data(state_tree_path: str) -> Dict[str, Any]:
    """
    Get persistent state data for save/load operations.

    Use this to retrieve information about which states are marked for
    persistence and their current configuration.

    Args:
        state_tree_path: Path to the StateTree asset

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - data: Object containing:
            - state_tree: StateTree name
            - persistent_states: Array of persistent state info
    """
    params = {"state_tree_path": state_tree_path}
    return await send_tcp_command("get_persistent_state_data", params)


# ============================================================================
# Section 7 - Gameplay Tag Integration Commands
# ============================================================================

@app.tool()
async def add_gameplay_tag_to_state(
    state_tree_path: str,
    state_name: str,
    gameplay_tag: str
) -> Dict[str, Any]:
    """
    Add a gameplay tag to a state for external querying.

    Gameplay tags can be used to identify states for external systems,
    like quest tracking, achievements, or analytics.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to tag
        gameplay_tag: Gameplay tag to add (e.g., "Quest.MainQuest.Active")

    Returns:
        Dictionary containing:
        - success: Whether tag was added successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "gameplay_tag": gameplay_tag
    }
    return await send_tcp_command("add_gameplay_tag_to_state", params)


@app.tool()
async def query_states_by_tag(
    state_tree_path: str,
    gameplay_tag: str,
    exact_match: bool = False
) -> Dict[str, Any]:
    """
    Query states by gameplay tag.

    Find all states that have a specific tag or match a tag hierarchy.

    Args:
        state_tree_path: Path to the StateTree asset
        gameplay_tag: Gameplay tag to search for
        exact_match: If True, only match exact tag; if False, include child tags (default: False)

    Returns:
        Dictionary containing:
        - success: Whether query was successful
        - states: Array of state names matching the tag
        - count: Number of matching states
    """
    params = {
        "state_tree_path": state_tree_path,
        "gameplay_tag": gameplay_tag,
        "exact_match": exact_match
    }
    return await send_tcp_command("query_states_by_tag", params)


# ============================================================================
# Section 8 - Runtime Inspection Commands (PIE)
# ============================================================================

@app.tool()
async def get_active_state_tree_status(
    state_tree_path: str,
    actor_path: str = ""
) -> Dict[str, Any]:
    """
    Get the status of an active StateTree during PIE.

    Use this to inspect running StateTree instances for debugging.

    Args:
        state_tree_path: Path to the StateTree asset
        actor_path: Path to the actor running the StateTree (optional, for multiple instances)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - data: Object containing:
            - state_tree_path: Path to the StateTree
            - actor_path: Actor running the tree
            - is_running: Whether the tree is currently executing
            - additional runtime info
    """
    params = {"state_tree_path": state_tree_path}
    if actor_path:
        params["actor_path"] = actor_path

    return await send_tcp_command("get_active_state_tree_status", params)


@app.tool()
async def get_current_active_states(
    state_tree_path: str,
    actor_path: str = ""
) -> Dict[str, Any]:
    """
    Get currently active states during PIE.

    Use this to see which states are currently being executed in a running StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        actor_path: Path to the actor running the StateTree (optional)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - active_states: Array of currently active state names
        - count: Number of active states
    """
    params = {"state_tree_path": state_tree_path}
    if actor_path:
        params["actor_path"] = actor_path

    return await send_tcp_command("get_current_active_states", params)


# ============================================================================
# Section 9 - Utility AI Consideration Commands
# ============================================================================

@app.tool()
async def add_consideration(
    state_tree_path: str,
    state_name: str,
    consideration_struct_path: str,
    weight: float = 1.0,
    consideration_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add a consideration for utility-based state selection.

    Considerations are used with Utility AI schemas to score states
    based on various factors like distance, health, priorities, etc.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to add consideration to
        consideration_struct_path: Full path to the consideration struct
        weight: Weight for this consideration in utility scoring (default: 1.0)
        consideration_properties: Optional dict of consideration-specific properties

    Returns:
        Dictionary containing:
        - success: Whether consideration was added successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "consideration_struct_path": consideration_struct_path,
        "weight": weight
    }
    if consideration_properties:
        params["consideration_properties"] = consideration_properties

    return await send_tcp_command("add_consideration", params)


# ============================================================================
# Section 10 - Task/Evaluator Modification Commands
# ============================================================================

@app.tool()
async def remove_task_from_state(
    state_tree_path: str,
    state_name: str,
    task_index: int = 0
) -> Dict[str, Any]:
    """
    Remove a task from a state in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state containing the task
        task_index: Index of the task to remove (0-based)

    Returns:
        Dictionary containing:
        - success: Whether task was removed successfully
        - state_name: State that contained the task
        - task_index: Index of the removed task
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "task_index": task_index
    }
    return await send_tcp_command("remove_task_from_state", params)


@app.tool()
async def set_task_properties(
    state_tree_path: str,
    state_name: str,
    task_index: int = 0,
    properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Set properties on a task in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state containing the task
        task_index: Index of the task to modify (0-based)
        properties: Dict of properties to set on the task

    Returns:
        Dictionary containing:
        - success: Whether properties were set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "task_index": task_index
    }
    if properties:
        params["properties"] = properties

    return await send_tcp_command("set_task_properties", params)


@app.tool()
async def remove_evaluator(
    state_tree_path: str,
    evaluator_index: int = 0
) -> Dict[str, Any]:
    """
    Remove an evaluator from a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        evaluator_index: Index of the evaluator to remove (0-based)

    Returns:
        Dictionary containing:
        - success: Whether evaluator was removed successfully
        - evaluator_index: Index of the removed evaluator
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "evaluator_index": evaluator_index
    }
    return await send_tcp_command("remove_evaluator", params)


@app.tool()
async def set_evaluator_properties(
    state_tree_path: str,
    evaluator_index: int = 0,
    properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Set properties on an evaluator in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        evaluator_index: Index of the evaluator to modify (0-based)
        properties: Dict of properties to set on the evaluator

    Returns:
        Dictionary containing:
        - success: Whether properties were set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "evaluator_index": evaluator_index
    }
    if properties:
        params["properties"] = properties

    return await send_tcp_command("set_evaluator_properties", params)


# ============================================================================
# Section 11 - Condition Removal Commands
# ============================================================================

@app.tool()
async def remove_condition_from_transition(
    state_tree_path: str,
    source_state_name: str,
    transition_index: int = 0,
    condition_index: int = 0
) -> Dict[str, Any]:
    """
    Remove a condition from a transition in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the state containing the transition
        transition_index: Index of the transition (0-based)
        condition_index: Index of the condition to remove (0-based)

    Returns:
        Dictionary containing:
        - success: Whether condition was removed successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "transition_index": transition_index,
        "condition_index": condition_index
    }
    return await send_tcp_command("remove_condition_from_transition", params)


@app.tool()
async def remove_enter_condition(
    state_tree_path: str,
    state_name: str,
    condition_index: int = 0
) -> Dict[str, Any]:
    """
    Remove an enter condition from a state in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state containing the enter condition
        condition_index: Index of the enter condition to remove (0-based)

    Returns:
        Dictionary containing:
        - success: Whether condition was removed successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "condition_index": condition_index
    }
    return await send_tcp_command("remove_enter_condition", params)


# ============================================================================
# Section 12 - Transition Inspection/Modification Commands
# ============================================================================

@app.tool()
async def get_transition_info(
    state_tree_path: str,
    source_state_name: str,
    transition_index: int = 0
) -> Dict[str, Any]:
    """
    Get detailed information about a transition in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the state containing the transition
        transition_index: Index of the transition (0-based)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - info: Object containing transition details:
            - source_state: Source state name
            - target_state: Target state name (if applicable)
            - trigger: Transition trigger type
            - transition_type: Type of transition
            - priority: Transition priority
            - delay_transition: Whether transition is delayed
            - delay_duration: Delay duration if applicable
            - condition_count: Number of conditions
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "transition_index": transition_index
    }
    return await send_tcp_command("get_transition_info", params)


@app.tool()
async def set_transition_properties(
    state_tree_path: str,
    source_state_name: str,
    transition_index: int = 0,
    target_state_name: str = None,
    trigger: str = None,
    transition_type: str = None,
    priority: str = None,
    delay_transition: bool = None,
    delay_duration: float = None
) -> Dict[str, Any]:
    """
    Set properties on a transition in a StateTree.

    Only properties that are explicitly provided will be changed.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the state containing the transition
        transition_index: Index of the transition (0-based)
        target_state_name: New target state name (optional)
        trigger: New trigger type (optional)
        transition_type: New transition type (optional)
        priority: New priority (optional)
        delay_transition: Whether to delay transition (optional)
        delay_duration: Delay duration in seconds (optional)

    Returns:
        Dictionary containing:
        - success: Whether properties were set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "transition_index": transition_index
    }
    if target_state_name is not None:
        params["target_state_name"] = target_state_name
    if trigger is not None:
        params["trigger"] = trigger
    if transition_type is not None:
        params["transition_type"] = transition_type
    if priority is not None:
        params["priority"] = priority
    if delay_transition is not None:
        params["delay_transition"] = delay_transition
    if delay_duration is not None:
        params["delay_duration"] = delay_duration

    return await send_tcp_command("set_transition_properties", params)


@app.tool()
async def get_transition_conditions(
    state_tree_path: str,
    source_state_name: str,
    transition_index: int = 0
) -> Dict[str, Any]:
    """
    Get all conditions on a transition in a StateTree.

    Args:
        state_tree_path: Path to the StateTree asset
        source_state_name: Name of the state containing the transition
        transition_index: Index of the transition (0-based)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - conditions: Array of conditions, each with:
            - index: Condition index
            - type: Condition struct type
            - combine_mode: How combined with other conditions
        - count: Total number of conditions
    """
    params = {
        "state_tree_path": state_tree_path,
        "source_state_name": source_state_name,
        "transition_index": transition_index
    }
    return await send_tcp_command("get_transition_conditions", params)


# ============================================================================
# Section 13 - State Event Handler Commands
# ============================================================================

@app.tool()
async def add_state_event_handler(
    state_tree_path: str,
    state_name: str,
    event_tag: str,
    handler_type: str = "Custom"
) -> Dict[str, Any]:
    """
    Add an event handler to a state for custom event processing.

    Event handlers allow states to respond to gameplay events without
    necessarily triggering a transition.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to add the handler to
        event_tag: Gameplay tag that triggers this handler (e.g., "AI.Event.DamageReceived")
        handler_type: Type of handler behavior:
            - "Custom" (default) - Custom handling logic
            - "Interrupt" - Interrupt current task
            - "Reset" - Reset state execution

    Returns:
        Dictionary containing:
        - success: Whether handler was added successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "event_tag": event_tag,
        "handler_type": handler_type
    }
    return await send_tcp_command("add_state_event_handler", params)


@app.tool()
async def configure_state_notifications(
    state_tree_path: str,
    state_name: str,
    notify_on_enter: bool = True,
    notify_on_exit: bool = True,
    notification_tag: str = ""
) -> Dict[str, Any]:
    """
    Configure notification settings for state enter/exit events.

    Use this to set up gameplay events that fire when a state is entered or exited.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to configure
        notify_on_enter: Whether to fire notification on state enter (default: True)
        notify_on_exit: Whether to fire notification on state exit (default: True)
        notification_tag: Base gameplay tag for notifications (e.g., "AI.StateNotify")

    Returns:
        Dictionary containing:
        - success: Whether configuration was successful
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "notify_on_enter": notify_on_enter,
        "notify_on_exit": notify_on_exit
    }
    if notification_tag:
        params["notification_tag"] = notification_tag

    return await send_tcp_command("configure_state_notifications", params)


# ============================================================================
# Section 14 - Linked State Configuration Commands
# ============================================================================

@app.tool()
async def get_linked_state_info(
    state_tree_path: str,
    state_name: str
) -> Dict[str, Any]:
    """
    Get information about a linked state (Linked or LinkedAsset type).

    Use this to inspect which external StateTree or state a linked state points to.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the linked state

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - info: Object containing:
            - state_name: Name of the linked state
            - state_type: Type of link (Linked, LinkedAsset)
            - linked_asset_path: Path to linked StateTree (for LinkedAsset)
            - linked_state_name: Name of linked state (for Linked type)
            - parameters: Any parameter overrides
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name
    }
    return await send_tcp_command("get_linked_state_info", params)


@app.tool()
async def set_linked_state_parameters(
    state_tree_path: str,
    state_name: str,
    parameters: Dict[str, Any]
) -> Dict[str, Any]:
    """
    Set parameter overrides for a linked state.

    Use this to pass custom parameter values to linked StateTrees.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the linked state
        parameters: Dict of parameter name to value overrides

    Returns:
        Dictionary containing:
        - success: Whether parameters were set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "parameters": parameters
    }
    return await send_tcp_command("set_linked_state_parameters", params)


@app.tool()
async def set_state_selection_weight(
    state_tree_path: str,
    state_name: str,
    weight: float = 1.0
) -> Dict[str, Any]:
    """
    Set the selection weight for a state (used with random selection).

    When a parent state uses TrySelectChildrenAtRandom, this weight influences
    how likely this state is to be selected relative to siblings.

    Args:
        state_tree_path: Path to the StateTree asset
        state_name: Name of the state to configure
        weight: Selection weight (default: 1.0, higher = more likely to be selected)

    Returns:
        Dictionary containing:
        - success: Whether weight was set successfully
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "state_name": state_name,
        "weight": weight
    }
    return await send_tcp_command("set_state_selection_weight", params)


# ============================================================================
# Section 15 - Batch Operations Commands
# ============================================================================

@app.tool()
async def batch_add_states(
    state_tree_path: str,
    states: List[Dict[str, Any]]
) -> Dict[str, Any]:
    """
    Add multiple states to a StateTree in a single operation.

    Use this to efficiently create complex state hierarchies.

    Args:
        state_tree_path: Path to the StateTree asset
        states: Array of state definitions, each containing:
            - state_name: Name of the state (required)
            - parent_state_name: Parent state name (optional, empty for root)
            - state_type: Type of state (optional, default: "State")
            - selection_behavior: Child selection behavior (optional)
            - enabled: Whether state is enabled (optional, default: True)

    Returns:
        Dictionary containing:
        - success: Whether batch operation was successful
        - states_added: Number of states successfully added
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "states": states
    }
    return await send_tcp_command("batch_add_states", params)


@app.tool()
async def batch_add_transitions(
    state_tree_path: str,
    transitions: List[Dict[str, Any]]
) -> Dict[str, Any]:
    """
    Add multiple transitions to a StateTree in a single operation.

    Use this to efficiently set up complex transition networks.

    Args:
        state_tree_path: Path to the StateTree asset
        transitions: Array of transition definitions, each containing:
            - source_state_name: Source state name (required)
            - target_state_name: Target state name (optional, for GotoState)
            - trigger: Trigger type (optional, default: "OnStateCompleted")
            - transition_type: Transition type (optional, default: "GotoState")
            - priority: Priority level (optional, default: "Normal")

    Returns:
        Dictionary containing:
        - success: Whether batch operation was successful
        - transitions_added: Number of transitions successfully added
        - message: Success/error message
    """
    params = {
        "state_tree_path": state_tree_path,
        "transitions": transitions
    }
    return await send_tcp_command("batch_add_transitions", params)


# ============================================================================
# Section 16 - Validation and Debugging Commands
# ============================================================================

@app.tool()
async def validate_all_bindings(state_tree_path: str) -> Dict[str, Any]:
    """
    Validate all property bindings in a StateTree.

    Use this to check for broken or invalid bindings that could cause runtime errors.

    Args:
        state_tree_path: Path to the StateTree asset

    Returns:
        Dictionary containing:
        - success: Whether validation completed
        - validation_results: Object containing:
            - total_bindings: Total number of bindings checked
            - valid_bindings: Number of valid bindings
            - invalid_bindings: Number of invalid bindings
            - errors: Array of binding error details
            - warnings: Array of binding warnings
    """
    params = {"state_tree_path": state_tree_path}
    return await send_tcp_command("validate_all_bindings", params)


@app.tool()
async def get_state_execution_history(
    state_tree_path: str,
    actor_path: str = "",
    max_entries: int = 100
) -> Dict[str, Any]:
    """
    Get state execution history from a running StateTree (PIE debugging).

    Use this to inspect the sequence of states that have been executed during
    a play session for debugging AI behavior.

    Args:
        state_tree_path: Path to the StateTree asset
        actor_path: Path to the actor running the StateTree (optional)
        max_entries: Maximum number of history entries to return (default: 100)

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - execution_history: Object containing:
            - state_tree_path: Path to the StateTree
            - actor_path: Actor running the tree
            - entries: Array of execution entries, each with:
                - state_name: Name of executed state
                - timestamp: When state was entered
                - duration: How long state was active (if exited)
                - exit_reason: Why state was exited (if applicable)
    """
    params = {
        "state_tree_path": state_tree_path,
        "max_entries": max_entries
    }
    if actor_path:
        params["actor_path"] = actor_path

    return await send_tcp_command("get_state_execution_history", params)


# ============================================================================
# Run Server
# ============================================================================

if __name__ == "__main__":
    app.run()
