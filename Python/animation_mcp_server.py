#!/usr/bin/env python3
"""
Animation Blueprint MCP Server

This server provides MCP tools for Animation Blueprint operations in Unreal Engine,
including creating animation blueprints, state machines, states, transitions,
animation layers, and animation variables.
"""

import asyncio
import json
from typing import Any, Dict, List

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("Animation Blueprint MCP Server")

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
# Animation Blueprint Creation
# ============================================================================

@app.tool()
async def create_animation_blueprint(
    name: str,
    skeleton_path: str,
    folder_path: str = "",
    parent_class: str = "",
    compile_on_creation: bool = True
) -> Dict[str, Any]:
    """
    Create a new Animation Blueprint.

    Args:
        name: Name of the Animation Blueprint (e.g., "ABP_Player")
        skeleton_path: Path to the target skeleton asset (e.g., "/Game/Characters/Player/Skeleton")
        folder_path: Optional folder path where the blueprint should be created
        parent_class: Optional parent AnimInstance class (default: UAnimInstance)
        compile_on_creation: Whether to compile after creation (default: True)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - name: Name of the created Animation Blueprint
        - path: Full path to the created asset
        - skeleton: Path to the associated skeleton
    """
    params = {
        "name": name,
        "skeleton_path": skeleton_path
    }
    if folder_path:
        params["folder_path"] = folder_path
    if parent_class:
        params["parent_class"] = parent_class
    params["compile_on_creation"] = compile_on_creation

    return await send_tcp_command("create_animation_blueprint", params)


@app.tool()
async def get_anim_blueprint_metadata(anim_blueprint_name: str) -> Dict[str, Any]:
    """
    Get metadata from an Animation Blueprint.

    Args:
        anim_blueprint_name: Name of the Animation Blueprint

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - metadata: Object containing:
            - name: Blueprint name
            - path: Full asset path
            - skeleton: Associated skeleton path
            - parent_class: Parent AnimInstance class
            - variables: List of animation variables with types
            - linked_layers: List of linked animation layers
            - has_root_connection: Boolean indicating if AnimGraph has valid root connection
            - animgraph_nodes: Array of all AnimGraph nodes, each with:
                - node_id: Unique node GUID
                - node_class: Node class name
                - node_title: Display title
                - position_x: X position in graph
                - position_y: Y position in graph
                - connected_to_root: Whether node connects to root output
            - state_machines: Array of state machines with enhanced details:
                - name: State machine name
                - node_id: Unique node GUID
                - position_x: X position in graph
                - position_y: Y position in graph
                - connected_to_root: Whether connected to root output
                - entry_state: Name of the entry/default state
                - transitions: Array of transitions, each with:
                    - from_state: Source state name
                    - to_state: Destination state name
                    - blend_duration: Blend time in seconds
                    - rule_type: Transition rule type (StandardBlend, Inertialization, Custom)
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name
    }
    return await send_tcp_command("get_anim_blueprint_metadata", params)


# ============================================================================
# Animation Layers
# ============================================================================

@app.tool()
async def link_animation_layer(
    anim_blueprint_name: str,
    layer_interface: str,
    layer_class: str = ""
) -> Dict[str, Any]:
    """
    Link an animation layer to an Animation Blueprint.

    Animation layers allow modular animation logic through layer interfaces.
    The layer interface defines the contract, and the layer class provides
    the implementation.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        layer_interface: Name of the layer interface (e.g., "IAnimLayerInterface_Combat")
        layer_class: Optional layer class implementing the interface

    Returns:
        Dictionary containing:
        - success: Whether linking was successful
        - layer: Name of the linked layer
        - message: Success/error message
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "layer_interface": layer_interface
    }
    if layer_class:
        params["layer_class"] = layer_class

    return await send_tcp_command("link_animation_layer", params)


# ============================================================================
# State Machines
# ============================================================================

@app.tool()
async def create_anim_state_machine(
    anim_blueprint_name: str,
    state_machine_name: str
) -> Dict[str, Any]:
    """
    Create a state machine in an Animation Blueprint's AnimGraph.

    State machines are the primary way to organize animation states and
    their transitions in Unreal Engine.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        state_machine_name: Name for the new state machine (e.g., "Locomotion")

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - state_machine: Name of the created state machine
        - message: Success/error message
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "state_machine_name": state_machine_name
    }
    return await send_tcp_command("create_anim_state_machine", params)


@app.tool()
async def add_anim_state(
    anim_blueprint_name: str,
    state_machine_name: str,
    state_name: str,
    animation_asset_path: str = "",
    is_default_state: bool = False,
    node_position_x: float = 0.0,
    node_position_y: float = 0.0
) -> Dict[str, Any]:
    """
    Add a state to a state machine in an Animation Blueprint.

    States represent distinct animation poses or sequences within a state machine.
    Each state can play an animation asset and be connected to other states
    via transitions.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        state_machine_name: Name of the state machine to add the state to
        state_name: Name for the new state (e.g., "Idle", "Walk", "Run")
        animation_asset_path: Optional path to the animation asset (sequence, blend space, etc.)
        is_default_state: Whether this should be the default/entry state
        node_position_x: X position for the state node in the graph
        node_position_y: Y position for the state node in the graph

    Returns:
        Dictionary containing:
        - success: Whether state was added successfully
        - state: Name of the added state
        - state_machine: Name of the parent state machine
        - message: Success/error message
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "state_machine_name": state_machine_name,
        "state_name": state_name
    }
    if animation_asset_path:
        params["animation_asset_path"] = animation_asset_path
    if is_default_state:
        params["is_default_state"] = is_default_state
    if node_position_x != 0.0 or node_position_y != 0.0:
        params["node_position_x"] = node_position_x
        params["node_position_y"] = node_position_y

    return await send_tcp_command("add_anim_state", params)


@app.tool()
async def add_anim_transition(
    anim_blueprint_name: str,
    state_machine_name: str,
    from_state: str,
    to_state: str,
    transition_rule_type: str = "CrossfadeBlend",
    blend_duration: float = 0.2,
    condition_variable: str = ""
) -> Dict[str, Any]:
    """
    Add a transition between two states in a state machine.

    Transitions define how the animation system moves between states.
    Different rule types control when and how transitions occur.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        state_machine_name: Name of the state machine
        from_state: Source state name
        to_state: Destination state name
        transition_rule_type: Type of transition rule:
            - "TimeRemaining": Transitions when animation time remaining is below threshold
            - "BoolVariable": Transitions when a bool variable is true (requires manual graph setup)
            - "CrossfadeBlend": Simple crossfade blend (default)
            - "Inertialization": Use inertialization for smoother state transitions
            - "Custom": Custom transition logic (requires manual setup)
        blend_duration: Duration of the blend transition in seconds (default: 0.2)
        condition_variable: Variable name for bool-based transitions

    Returns:
        Dictionary containing:
        - success: Whether transition was added successfully
        - from_state: Source state name
        - to_state: Destination state name
        - message: Success/error message
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "state_machine_name": state_machine_name,
        "from_state": from_state,
        "to_state": to_state
    }
    if transition_rule_type:
        params["transition_rule_type"] = transition_rule_type
    if blend_duration != 0.2:
        params["blend_duration"] = blend_duration
    if condition_variable:
        params["condition_variable"] = condition_variable

    return await send_tcp_command("add_anim_transition", params)


# ============================================================================
# Animation Variables
# ============================================================================

@app.tool()
async def add_anim_variable(
    anim_blueprint_name: str,
    variable_name: str,
    variable_type: str,
    default_value: str = ""
) -> Dict[str, Any]:
    """
    Add a variable to an Animation Blueprint.

    Animation variables are used to control animation logic, state transitions,
    blend weights, and other animation parameters.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        variable_name: Name of the variable (e.g., "Speed", "IsInAir", "Direction")
        variable_type: Type of the variable:
            - "Bool": Boolean true/false
            - "Float": Floating-point number
            - "Int": Integer number
            - "Vector": 3D vector
            - "Rotator": Rotation
        default_value: Optional default value as string (e.g., "0.0", "true")

    Returns:
        Dictionary containing:
        - success: Whether variable was added successfully
        - variable: Name of the added variable
        - message: Success/error message
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "variable_name": variable_name,
        "variable_type": variable_type
    }
    if default_value:
        params["default_value"] = default_value

    return await send_tcp_command("add_anim_variable", params)


# ============================================================================
# Animation Slots
# ============================================================================

@app.tool()
async def configure_anim_slot(
    anim_blueprint_name: str,
    slot_name: str,
    slot_group: str = ""
) -> Dict[str, Any]:
    """
    Configure an animation slot in an Animation Blueprint.

    Animation slots allow montages and other dynamic animations to be
    played on specific body parts or layers without disrupting the
    base animation state machine.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        slot_name: Name of the slot (e.g., "UpperBody", "FullBody", "DefaultSlot")
        slot_group: Optional slot group name for organizing related slots

    Returns:
        Dictionary containing:
        - success: Whether slot was configured successfully
        - slot: Name of the configured slot
        - message: Success/error message
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "slot_name": slot_name
    }
    if slot_group:
        params["slot_group"] = slot_group

    return await send_tcp_command("configure_anim_slot", params)


# ============================================================================
# AnimGraph Connections
# ============================================================================

@app.tool()
async def connect_anim_graph_nodes(
    anim_blueprint_name: str,
    source_node_name: str,
    target_node_name: str = "",
    source_pin_name: str = "Pose",
    target_pin_name: str = "Result"
) -> Dict[str, Any]:
    """
    Connect nodes in an Animation Blueprint's AnimGraph.

    This is typically used to connect a state machine's output pose to the
    AnimGraph's root node (output pose), enabling the state machine to
    drive the final animation output.

    Args:
        anim_blueprint_name: Name of the target Animation Blueprint
        source_node_name: Name of the source node (e.g., state machine name like "Locomotion")
        target_node_name: Name of the target node. Empty string, "OutputPose", or "Root"
                         connects to the AnimGraph's root output pose node.
        source_pin_name: Name of the source output pin (default: "Pose")
        target_pin_name: Name of the target input pin (default: "Result")

    Returns:
        Dictionary containing:
        - success: Whether connection was successful
        - source_node: Name of the source node
        - target_node: Name of the target node
        - source_pin: Name of the source pin
        - target_pin: Name of the target pin
        - message: Success/error message

    Examples:
        # Connect a state machine to the output pose
        connect_anim_graph_nodes(
            anim_blueprint_name="ABP_Player",
            source_node_name="Locomotion"  # State machine name
        )

        # Connect with explicit target
        connect_anim_graph_nodes(
            anim_blueprint_name="ABP_Player",
            source_node_name="Locomotion",
            target_node_name="OutputPose",
            source_pin_name="Pose",
            target_pin_name="Result"
        )
    """
    params = {
        "anim_blueprint_name": anim_blueprint_name,
        "source_node_name": source_node_name
    }
    if target_node_name:
        params["target_node_name"] = target_node_name
    if source_pin_name != "Pose":
        params["source_pin_name"] = source_pin_name
    if target_pin_name != "Result":
        params["target_pin_name"] = target_pin_name

    return await send_tcp_command("connect_anim_graph_nodes", params)


# ============================================================================
# Run Server
# ============================================================================

if __name__ == "__main__":
    app.run()
