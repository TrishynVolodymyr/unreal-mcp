#!/usr/bin/env python3
"""
Niagara MCP Server

This server provides MCP tools for Niagara VFX operations in Unreal Engine,
including creating systems, managing emitters, setting parameters, and
configuring renderers.
"""

import asyncio
import json
from typing import Any, Dict, List

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("Niagara MCP Server")

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
# Niagara System Creation
# ============================================================================

@app.tool()
async def create_niagara_system(
    name: str,
    folder_path: str = "",
    base_system: str = "",
    auto_activate: bool = True
) -> Dict[str, Any]:
    """
    Create a new Niagara System.

    Niagara Systems are the top-level containers for visual effects, containing
    one or more emitters that define particle behavior and appearance.

    Args:
        name: Name of the Niagara System (e.g., "NS_FireExplosion")
        folder_path: Optional folder path for the asset (e.g., "/Game/Effects/Fire")
        base_system: Optional path to an existing system to duplicate from
        auto_activate: Whether the system auto-activates when spawned (default: True)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - name: Name of the created system
        - path: Full asset path
        - auto_activate: Whether auto-activate is enabled
        - message: Success/error message

    Example:
        create_niagara_system(
            name="NS_FireExplosion",
            folder_path="/Game/Effects/Fire",
            auto_activate=True
        )
    """
    params = {"name": name}
    if folder_path:
        params["folder_path"] = folder_path
    if base_system:
        params["base_system"] = base_system
    params["auto_activate"] = auto_activate

    return await send_tcp_command("create_niagara_system", params)


@app.tool()
async def duplicate_niagara_system(
    source_system: str,
    new_name: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """
    Duplicate an existing Niagara System to create a variation.

    This is useful for creating variations of effects while preserving
    all emitters, parameters, and configuration.

    Args:
        source_system: Path or name of the source Niagara System
        new_name: Name for the new duplicated system
        folder_path: Optional folder path for the new asset

    Returns:
        Dictionary containing:
        - success: Whether duplication was successful
        - name: Name of the duplicated system
        - path: Full asset path
        - message: Success/error message

    Example:
        duplicate_niagara_system(
            source_system="NS_FireExplosion",
            new_name="NS_FireExplosion_Blue",
            folder_path="/Game/Effects/Fire"
        )
    """
    params = {
        "source_system": source_system,
        "new_name": new_name
    }
    if folder_path:
        params["folder_path"] = folder_path

    return await send_tcp_command("duplicate_niagara_system", params)


@app.tool()
async def get_niagara_system_metadata(
    system: str
) -> Dict[str, Any]:
    """
    Get metadata and configuration from a Niagara System.

    Retrieves information about the system including all emitters,
    their enabled states, and exposed parameters.

    Args:
        system: Path or name of the Niagara System

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - name: System name
        - path: Full asset path
        - auto_activate: Whether auto-activate is enabled
        - emitters: Array of emitter info objects with:
            - name: Emitter name
            - path: Emitter asset path
            - enabled: Whether the emitter is enabled

    Example:
        get_niagara_system_metadata(system="NS_FireExplosion")
    """
    params = {"system": system}
    return await send_tcp_command("get_niagara_system_metadata", params)


@app.tool()
async def compile_niagara_system(
    system: str
) -> Dict[str, Any]:
    """
    Compile and save a Niagara System.

    Forces recompilation of all emitters and saves the system asset.
    Use this after making changes to ensure they're persisted.

    Args:
        system: Path or name of the Niagara System to compile

    Returns:
        Dictionary containing:
        - success: Whether compilation was successful
        - system: Name of the compiled system
        - message: Success/error message

    Example:
        compile_niagara_system(system="NS_FireExplosion")
    """
    params = {"system": system}
    return await send_tcp_command("compile_niagara_system", params)


# ============================================================================
# Emitter Operations
# ============================================================================

@app.tool()
async def create_niagara_emitter(
    name: str,
    folder_path: str = "",
    template: str = "",
    emitter_type: str = "Sprite"
) -> Dict[str, Any]:
    """
    Create a new standalone Niagara Emitter asset.

    Emitters define individual particle behaviors and can be added to
    Niagara Systems. Creating standalone emitters allows reuse across
    multiple systems.

    Args:
        name: Name of the emitter (e.g., "NE_Sparks", "NE_Smoke")
        folder_path: Optional folder path for the asset (e.g., "/Game/Effects/Emitters")
        template: Optional template emitter path to copy from
        emitter_type: Type of emitter to create:
            - "Sprite": 2D billboard particles (default, most common)
            - "Mesh": 3D mesh-based particles
            - "Ribbon": Connected trail/ribbon particles
            - "Light": Light-emitting particles

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - name: Name of the created emitter
        - path: Full asset path
        - emitter_type: Type of emitter created
        - message: Success/error message

    Example:
        create_niagara_emitter(
            name="NE_FireSparks",
            folder_path="/Game/Effects/Emitters",
            emitter_type="Sprite"
        )
    """
    params = {"name": name, "emitter_type": emitter_type}
    if folder_path:
        params["folder_path"] = folder_path
    if template:
        params["template"] = template

    return await send_tcp_command("create_niagara_emitter", params)


@app.tool()
async def add_emitter_to_system(
    system: str,
    emitter: str,
    create_if_missing: bool = False,
    emitter_folder: str = "",
    emitter_type: str = "Sprite"
) -> Dict[str, Any]:
    """
    Add an emitter to a Niagara System.

    Emitters define individual particle behaviors within a system.
    Multiple emitters can be combined for complex effects.

    Args:
        system: Path or name of the target Niagara System
        emitter: Path or name of the emitter to add
        create_if_missing: If True and emitter doesn't exist, create it first
        emitter_folder: Folder path for creating emitter if create_if_missing=True
                       (defaults to same folder as system)
        emitter_type: Type of emitter to create if create_if_missing=True:
            - "Sprite": 2D billboard particles (default)
            - "Mesh": 3D mesh-based particles
            - "Ribbon": Connected trail/ribbon particles
            - "Light": Light-emitting particles

    Returns:
        Dictionary containing:
        - success: Whether the emitter was added successfully
        - system: Name of the system
        - emitter: Name of the added emitter
        - created: Whether a new emitter was created (only if create_if_missing=True)
        - message: Success/error message

    Example:
        # Add existing emitter
        add_emitter_to_system(
            system="NS_FireExplosion",
            emitter="/Game/Effects/Emitters/Sparks"
        )

        # Create and add emitter if it doesn't exist
        add_emitter_to_system(
            system="/Game/Testing/MCPTest_FireEffect",
            emitter="NE_TestSparks",
            create_if_missing=True,
            emitter_folder="/Game/Testing",
            emitter_type="Sprite"
        )
    """
    params = {
        "system_path": system,
        "emitter_path": emitter
    }

    # First attempt to add the emitter
    result = await send_tcp_command("add_emitter_to_system", params)

    # If failed and create_if_missing is True, try creating the emitter first
    if not result.get("success", False) and create_if_missing:
        error_msg = result.get("error", "").lower()
        # Check if failure was due to emitter not found
        if "not found" in error_msg or "could not find" in error_msg or "does not exist" in error_msg:
            # Extract emitter name from path
            emitter_name = emitter.split("/")[-1] if "/" in emitter else emitter

            # Determine folder path
            if not emitter_folder:
                # Use same folder as system if not specified
                if "/" in system:
                    emitter_folder = "/".join(system.split("/")[:-1])
                else:
                    emitter_folder = "/Game/Niagara/Emitters"

            # Create the emitter
            create_result = await send_tcp_command("create_niagara_emitter", {
                "name": emitter_name,
                "folder_path": emitter_folder,
                "emitter_type": emitter_type
            })

            if create_result.get("success", False):
                # Now try adding the newly created emitter
                created_path = create_result.get("path", f"{emitter_folder}/{emitter_name}")
                params["emitter_path"] = created_path
                result = await send_tcp_command("add_emitter_to_system", params)
                result["created"] = True
                result["created_emitter_path"] = created_path
            else:
                # Return the creation failure
                result = {
                    "success": False,
                    "error": f"Failed to create emitter: {create_result.get('error', 'Unknown error')}",
                    "original_add_error": error_msg
                }

    return result


@app.tool()
async def remove_emitter_from_system(
    system: str,
    emitter: str
) -> Dict[str, Any]:
    """
    Remove an emitter from a Niagara System.

    Args:
        system: Path or name of the target Niagara System
        emitter: Name of the emitter to remove (as it appears in the system)

    Returns:
        Dictionary containing:
        - success: Whether the emitter was removed successfully
        - system: Name of the system
        - emitter: Name of the removed emitter
        - message: Success/error message

    Example:
        remove_emitter_from_system(
            system="NS_FireExplosion",
            emitter="Sparks"
        )
    """
    params = {
        "system": system,
        "emitter": emitter
    }
    return await send_tcp_command("remove_emitter_from_system", params)


@app.tool()
async def set_emitter_enabled(
    system: str,
    emitter: str,
    enabled: bool = True
) -> Dict[str, Any]:
    """
    Enable or disable an emitter in a Niagara System.

    Disabled emitters remain in the system but don't spawn particles.
    Useful for creating effect variants without removing emitters.

    Args:
        system: Path or name of the target Niagara System
        emitter: Name of the emitter to enable/disable
        enabled: Whether to enable (True) or disable (False) the emitter

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - system: Name of the system
        - emitter: Name of the emitter
        - enabled: The new enabled state
        - message: Success/error message

    Example:
        set_emitter_enabled(
            system="NS_FireExplosion",
            emitter="Smoke",
            enabled=False
        )
    """
    params = {
        "system": system,
        "emitter": emitter,
        "enabled": enabled
    }
    return await send_tcp_command("set_emitter_enabled", params)


# ============================================================================
# Parameter Operations
# ============================================================================

@app.tool()
async def set_niagara_float_param(
    system: str,
    param_name: str,
    value: float
) -> Dict[str, Any]:
    """
    Set a float parameter on a Niagara System.

    Float parameters control numeric values like spawn rates, sizes,
    lifetimes, speeds, and other scalar properties.

    Args:
        system: Path or name of the Niagara System
        param_name: Name of the float parameter (e.g., "SpawnRate", "ParticleSize")
        value: Float value to set

    Returns:
        Dictionary containing:
        - success: Whether the parameter was set successfully
        - system: Name of the system
        - param_name: Name of the parameter
        - value: The value that was set
        - message: Success/error message

    Example:
        set_niagara_float_param(
            system="NS_FireExplosion",
            param_name="SpawnRate",
            value=500.0
        )
    """
    params = {
        "system": system,
        "param_name": param_name,
        "value": value
    }
    return await send_tcp_command("set_niagara_float_param", params)


@app.tool()
async def set_niagara_vector_param(
    system: str,
    param_name: str,
    x: float,
    y: float,
    z: float
) -> Dict[str, Any]:
    """
    Set a vector parameter on a Niagara System.

    Vector parameters control 3D values like velocities, positions,
    scales, and other directional properties.

    Args:
        system: Path or name of the Niagara System
        param_name: Name of the vector parameter (e.g., "InitialVelocity", "SpawnLocation")
        x: X component of the vector
        y: Y component of the vector
        z: Z component of the vector

    Returns:
        Dictionary containing:
        - success: Whether the parameter was set successfully
        - system: Name of the system
        - param_name: Name of the parameter
        - value: Array [X, Y, Z] that was set
        - message: Success/error message

    Example:
        set_niagara_vector_param(
            system="NS_FireExplosion",
            param_name="InitialVelocity",
            x=0.0, y=0.0, z=500.0
        )
    """
    params = {
        "system": system,
        "param_name": param_name,
        "x": x,
        "y": y,
        "z": z
    }
    return await send_tcp_command("set_niagara_vector_param", params)


@app.tool()
async def set_niagara_color_param(
    system: str,
    param_name: str,
    r: float,
    g: float,
    b: float,
    a: float = 1.0
) -> Dict[str, Any]:
    """
    Set a color parameter on a Niagara System.

    Color parameters control particle colors, emissive values,
    and other RGBA properties.

    Args:
        system: Path or name of the Niagara System
        param_name: Name of the color parameter (e.g., "ParticleColor", "EmissiveColor")
        r: Red component (0.0-1.0, can exceed 1.0 for HDR/emissive)
        g: Green component
        b: Blue component
        a: Alpha component (default: 1.0)

    Returns:
        Dictionary containing:
        - success: Whether the parameter was set successfully
        - system: Name of the system
        - param_name: Name of the parameter
        - value: Array [R, G, B, A] that was set
        - message: Success/error message

    Example:
        set_niagara_color_param(
            system="NS_FireExplosion",
            param_name="ParticleColor",
            r=1.0, g=0.5, b=0.0, a=1.0
        )
    """
    params = {
        "system": system,
        "param_name": param_name,
        "r": r,
        "g": g,
        "b": b,
        "a": a
    }
    return await send_tcp_command("set_niagara_color_param", params)


@app.tool()
async def get_niagara_parameters(
    system: str
) -> Dict[str, Any]:
    """
    Get all user-exposed parameters from a Niagara System.

    Returns all parameters that have been exposed for external control,
    including their current values.

    Args:
        system: Path or name of the Niagara System

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - system: Name of the system
        - path: Full asset path
        - parameters: Array of parameter info objects with:
            - name: Parameter name
            - value: Current value as string

    Example:
        get_niagara_parameters(system="NS_FireExplosion")
    """
    params = {"system": system}
    return await send_tcp_command("get_niagara_parameters", params)


@app.tool()
async def add_niagara_parameter(
    system: str,
    parameter_name: str,
    parameter_type: str,
    default_value: str | int | float | bool | list = "",
    scope: str = "user"
) -> Dict[str, Any]:
    """
    Add a user-exposed parameter to a Niagara System.

    Creates a new parameter that can be controlled externally via Blueprint,
    C++, or the set_niagara_*_param tools. Parameters must be added before
    they can be set with the parameter setter tools.

    Args:
        system: Path or name of the Niagara System
        parameter_name: Name for the new parameter (e.g., "SpawnRate", "ParticleColor")
        parameter_type: Type of parameter to create:
            - "Float": Scalar float value
            - "Int": Integer value
            - "Bool": Boolean true/false
            - "Vector": 3D vector (X, Y, Z)
            - "LinearColor": RGBA color value
        default_value: Optional default value. Accepts native types or strings:
            - Float: 100.0 or "100.0"
            - Int: 5 or "5"
            - Bool: True or "true"
            - Vector: [0.0, 0.0, 100.0] or "0,0,100"
            - LinearColor: [1.0, 0.5, 0.0, 1.0] or "1.0,0.5,0.0,1.0"
        scope: Parameter scope (default: "user"):
            - "user": User-exposed parameter (prefixed with "User.")
            - "system": System-level parameter (prefixed with "System.")
            - "emitter": Emitter-level parameter (prefixed with "Emitter.")

    Returns:
        Dictionary containing:
        - success: Whether the parameter was added successfully
        - system: Name of the system
        - parameter_name: Full parameter name with scope prefix
        - parameter_type: Type of parameter created
        - scope: The scope that was used
        - message: Success/error message

    Examples:
        # Add a float parameter for spawn rate control
        add_niagara_parameter(
            system="NS_FireExplosion",
            parameter_name="SpawnRate",
            parameter_type="Float",
            default_value=500.0
        )

        # Add a color parameter with default orange
        add_niagara_parameter(
            system="NS_FireExplosion",
            parameter_name="ParticleColor",
            parameter_type="LinearColor",
            default_value=[1.0, 0.5, 0.0, 1.0]
        )

        # Add a vector parameter for velocity
        add_niagara_parameter(
            system="NS_FireExplosion",
            parameter_name="InitialVelocity",
            parameter_type="Vector",
            default_value=[0.0, 0.0, 200.0]
        )

        # Add a boolean toggle
        add_niagara_parameter(
            system="NS_FireExplosion",
            parameter_name="EnableTrails",
            parameter_type="Bool",
            default_value=True
        )
    """
    params = {
        "system_path": system,
        "parameter_name": parameter_name,
        "parameter_type": parameter_type,
        "scope": scope
    }
    if default_value is not None and default_value != "":
        # Convert default_value to string format expected by C++
        if isinstance(default_value, bool):
            params["default_value"] = "true" if default_value else "false"
        elif isinstance(default_value, (list, dict)):
            import json
            params["default_value"] = json.dumps(default_value)
        else:
            params["default_value"] = str(default_value)

    return await send_tcp_command("add_niagara_parameter", params)


# ============================================================================
# Renderer Operations
# ============================================================================

@app.tool()
async def add_renderer_to_emitter(
    system: str,
    emitter: str,
    renderer_type: str
) -> Dict[str, Any]:
    """
    Add a renderer to an emitter in a Niagara System.

    Renderers define how particles are visually displayed. Different
    renderer types support different visual styles.

    Args:
        system: Path or name of the Niagara System
        emitter: Name of the emitter to add the renderer to
        renderer_type: Type of renderer to add:
            - "Sprite": 2D billboard particles (most common)
            - "Mesh": 3D mesh particles
            - "Ribbon": Connected trail/ribbon particles
            - "Light": Light-emitting particles

    Returns:
        Dictionary containing:
        - success: Whether the renderer was added successfully
        - system: Name of the system
        - emitter: Name of the emitter
        - renderer_type: Type of renderer added
        - message: Success/error message

    Example:
        add_renderer_to_emitter(
            system="NS_FireExplosion",
            emitter="Sparks",
            renderer_type="Sprite"
        )
    """
    params = {
        "system": system,
        "emitter": emitter,
        "renderer_type": renderer_type
    }
    return await send_tcp_command("add_renderer_to_emitter", params)


# ============================================================================
# Module Operations
# ============================================================================

@app.tool()
async def search_niagara_modules(
    search_query: str = "",
    stage_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """
    Search available Niagara modules in the asset registry.

    Args:
        search_query: Text to search in module names (e.g., "Spawn", "Velocity", "Color")
        stage_filter: Filter by stage - "Spawn", "Update", "Event", or "" for all
        max_results: Maximum results to return (default: 50)

    Returns:
        Dictionary containing:
        - success: Whether search was successful
        - modules: Array of module info with path, name, description
        - count: Number of modules found

    Example:
        search_niagara_modules(search_query="Velocity", stage_filter="Spawn")
    """
    params = {
        "search_query": search_query,
        "stage_filter": stage_filter,
        "max_results": max_results
    }
    return await send_tcp_command("search_niagara_modules", params)


@app.tool()
async def add_module_to_emitter(
    system_path: str,
    emitter_name: str,
    module_path: str,
    stage: str,
    index: int = -1
) -> Dict[str, Any]:
    """
    Add a module to a specific stage of an emitter.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_path: Path to the module script asset
        stage: Stage to add module to - "Spawn", "Update", or "Event"
        index: Position in stack (-1 = end, 0 = beginning)

    Returns:
        Dictionary containing:
        - success: Whether module was added
        - module_name: Name of added module
        - stage: Stage it was added to
        - message: Success/error message

    Example:
        add_module_to_emitter(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_path="/Niagara/Modules/Spawn/SpawnRate.SpawnRate",
            stage="Spawn"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_path": module_path,
        "stage": stage,
        "index": index
    }
    return await send_tcp_command("add_module_to_emitter", params)


@app.tool()
async def set_module_input(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    input_name: str,
    value: str,
    value_type: str = "auto"
) -> Dict[str, Any]:
    """
    Set an input value on a module.

    Args:
        system_path: Path to the Niagara System
        emitter_name: Emitter name within the system
        module_name: Name of the module to configure
        stage: Stage containing the module - "Spawn", "Update", or "Event"
        input_name: Input parameter name (e.g., "SpawnRate", "Velocity")
        value: Value to set as string (e.g., "100", "0,0,500", "1,0.5,0,1")
        value_type: Type hint - "float", "vector", "color", "int", "bool", or "auto"

    Returns:
        Dictionary containing:
        - success: Whether input was set
        - module_name: Module that was modified
        - input_name: Input that was set
        - message: Success/error message

    Example:
        set_module_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="SpawnRate",
            stage="Spawn",
            input_name="SpawnRate",
            value="50",
            value_type="float"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "input_name": input_name,
        "value": str(value),
        "value_type": value_type
    }
    return await send_tcp_command("set_module_input", params)


@app.tool()
async def move_module(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    new_index: int
) -> Dict[str, Any]:
    """
    Move a module to a new position within its stage.

    Important for execution order - e.g., force modules must be before
    SolveForcesAndVelocity for forces to apply correctly.

    Args:
        system_path: Path to the Niagara System
        emitter_name: Emitter name
        module_name: Module name to move
        stage: Stage containing the module - "Spawn" or "Update"
        new_index: New position index (0-based)

    Returns:
        Dictionary containing:
        - success: Whether move was successful
        - message: Success/error message

    Example:
        move_module(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="CurlNoiseForce",
            stage="Update",
            new_index=3
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "new_index": new_index
    }
    return await send_tcp_command("move_module", params)


@app.tool()
async def set_module_curve_input(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    input_name: str,
    keyframes: List[Dict[str, float]]
) -> Dict[str, Any]:
    """
    Set a float curve input on a module.

    Creates a curve data interface for module inputs that accept curves,
    such as Scale Over Life, Drag, or custom curve-based parameters.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to configure
        stage: Stage containing the module - "Spawn" or "Update"
        input_name: Input parameter name (e.g., "ScaleFactor", "DragCoefficient")
        keyframes: List of keyframe dictionaries, each containing:
            - time: Normalized time (0.0 to 1.0)
            - value: Float value at this time

    Returns:
        Dictionary containing:
        - success: Whether curve was set
        - module_name: Module that was modified
        - input_name: Input that was set
        - keyframe_count: Number of keyframes added
        - message: Success/error message

    Example:
        # Scale particles from 100% at birth to 0% at death
        set_module_curve_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="ScaleSpriteSize",
            stage="Update",
            input_name="ScaleFactor",
            keyframes=[
                {"time": 0.0, "value": 1.0},
                {"time": 0.5, "value": 0.7},
                {"time": 1.0, "value": 0.0}
            ]
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "input_name": input_name,
        "keyframes": keyframes
    }
    return await send_tcp_command("set_module_curve_input", params)


@app.tool()
async def set_module_color_curve_input(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    input_name: str,
    keyframes: List[Dict[str, float]]
) -> Dict[str, Any]:
    """
    Set a color curve (gradient) input on a module.

    Creates a color curve data interface for module inputs that accept
    color gradients, such as Color Over Life or custom color parameters.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to configure
        stage: Stage containing the module - "Spawn" or "Update"
        input_name: Input parameter name (e.g., "ColorScale", "ParticleColor")
        keyframes: List of keyframe dictionaries, each containing:
            - time: Normalized time (0.0 to 1.0)
            - r: Red component (0.0-1.0, can exceed 1.0 for HDR)
            - g: Green component
            - b: Blue component
            - a: Alpha component (default: 1.0)

    Returns:
        Dictionary containing:
        - success: Whether curve was set
        - module_name: Module that was modified
        - input_name: Input that was set
        - keyframe_count: Number of keyframes added
        - message: Success/error message

    Example:
        # Fire ember color gradient: bright yellow -> orange -> dark red -> fade out
        set_module_color_curve_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Embers",
            module_name="ScaleColor",
            stage="Update",
            input_name="ColorScale",
            keyframes=[
                {"time": 0.0, "r": 1.0, "g": 0.9, "b": 0.3, "a": 1.0},
                {"time": 0.3, "r": 1.0, "g": 0.5, "b": 0.1, "a": 1.0},
                {"time": 0.7, "r": 0.8, "g": 0.2, "b": 0.05, "a": 0.8},
                {"time": 1.0, "r": 0.3, "g": 0.05, "b": 0.0, "a": 0.0}
            ]
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "input_name": input_name,
        "keyframes": keyframes
    }
    return await send_tcp_command("set_module_color_curve_input", params)


@app.tool()
async def set_renderer_property(
    system_path: str,
    emitter_name: str,
    renderer_name: str,
    property_name: str,
    property_value: str
) -> Dict[str, Any]:
    """
    Set a property on a renderer.

    Args:
        system_path: Path to the Niagara System
        emitter_name: Emitter name
        renderer_name: Renderer name (e.g., "Renderer" or "Renderer_0")
        property_name: Property to set:
            - Sprite: "Material", "Alignment", "FacingMode", "SubImageSize"
            - Mesh: "ParticleMesh", "Material"
            - Ribbon: "Material", "RibbonWidth"
        property_value: Value (asset path for materials/meshes, or setting value)

    Returns:
        Dictionary containing:
        - success: Whether property was set
        - message: Success/error message

    Example:
        set_renderer_property(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            renderer_name="Renderer",
            property_name="Material",
            property_value="/Game/Materials/M_Ember"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "renderer_name": renderer_name,
        "property_name": property_name,
        "property_value": str(property_value)
    }
    return await send_tcp_command("set_renderer_property", params)


@app.tool()
async def spawn_niagara_actor(
    system_path: str,
    actor_name: str,
    location: List[float] = None,
    rotation: List[float] = None,
    auto_activate: bool = True
) -> Dict[str, Any]:
    """
    Spawn a Niagara System actor in the level.

    Args:
        system_path: Path to the Niagara System asset
        actor_name: Name for the spawned actor
        location: [X, Y, Z] world location (default: [0, 0, 0])
        rotation: [Pitch, Yaw, Roll] in degrees (default: [0, 0, 0])
        auto_activate: Whether to auto-activate on spawn (default: True)

    Returns:
        Dictionary containing:
        - success: Whether spawn was successful
        - actor_name: Name of spawned actor
        - location: World location
        - message: Success/error message

    Example:
        spawn_niagara_actor(
            system_path="/Game/Effects/NS_Fire",
            actor_name="FireEffect_01",
            location=[0, 0, 100],
            auto_activate=True
        )
    """
    params = {
        "system_path": system_path,
        "actor_name": actor_name,
        "auto_activate": auto_activate
    }
    if location is not None:
        params["location"] = location
    if rotation is not None:
        params["rotation"] = rotation
    return await send_tcp_command("spawn_niagara_actor", params)


# ============================================================================
# Run Server
# ============================================================================

if __name__ == "__main__":
    app.run()
