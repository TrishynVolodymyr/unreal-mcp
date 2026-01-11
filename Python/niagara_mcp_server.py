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

            # Handle both wrapped (MCP) and unwrapped (raw) response formats
            # MCP wraps responses as {"status": "success", "result": {...}}
            inner_result = create_result.get("result", create_result)
            create_success = inner_result.get("success", False) or create_result.get("status") == "success"

            if create_success:
                # Now try adding the newly created emitter
                # C++ returns "emitter_path", fallback to constructed path
                created_path = inner_result.get("emitter_path") or inner_result.get("path") or f"{emitter_folder}/{emitter_name}"
                params["emitter_path"] = created_path
                result = await send_tcp_command("add_emitter_to_system", params)
                result["created"] = True
                result["created_emitter_path"] = created_path
            else:
                # Return the creation failure
                error_msg_create = inner_result.get("error") or create_result.get("error", "Unknown error")
                result = {
                    "success": False,
                    "error": f"Failed to create emitter: {error_msg_create}",
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


@app.tool()
async def set_emitter_property(
    system: str,
    emitter: str,
    property_name: str,
    property_value: str
) -> Dict[str, Any]:
    """
    Set a property on an emitter within a Niagara System.

    This allows configuring emitter-level settings like simulation target (CPU/GPU),
    local space mode, determinism, and other properties that affect the entire emitter.

    Args:
        system: Path or name of the target Niagara System
        emitter: Name of the emitter to configure
        property_name: Property to set. Supported properties:
            - "LocalSpace": Whether particles simulate in local space (true/false)
            - "Determinism": Whether simulation is deterministic (true/false)
            - "RandomSeed": Seed for deterministic random (integer)
            - "SimTarget": Simulation target - "CPU" or "GPU"
            - "RequiresPersistentIDs": Enable persistent particle IDs (true/false)
            - "MaxGPUParticlesSpawnPerFrame": Max GPU particles per frame (integer)
        property_value: Value to set as string:
            - Boolean: "true" or "false"
            - Integer: "12345"
            - SimTarget: "CPU" or "GPU"

    Returns:
        Dictionary containing:
        - success: Whether the property was set successfully
        - property_name: Name of the property that was set
        - property_value: The value that was set
        - message: Success/error message

    Examples:
        # Switch emitter to GPU simulation
        set_emitter_property(
            system="NS_FireExplosion",
            emitter="Sparks",
            property_name="SimTarget",
            property_value="GPU"
        )

        # Enable local space simulation
        set_emitter_property(
            system="NS_FireExplosion",
            emitter="Sparks",
            property_name="LocalSpace",
            property_value="true"
        )

        # Set deterministic seed
        set_emitter_property(
            system="NS_FireExplosion",
            emitter="Sparks",
            property_name="RandomSeed",
            property_value="42"
        )
    """
    params = {
        "system_path": system,
        "emitter_name": emitter,
        "property_name": property_name,
        "property_value": str(property_value)
    }
    return await send_tcp_command("set_emitter_property", params)


@app.tool()
async def get_emitter_properties(
    system: str,
    emitter: str
) -> Dict[str, Any]:
    """
    Get properties from an emitter within a Niagara System.

    Retrieves emitter-level configuration including simulation target,
    local space mode, determinism settings, and bounds mode.

    Args:
        system: Path or name of the target Niagara System
        emitter: Name of the emitter to query

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - emitter_name: Name of the emitter
        - system_path: Path to the system
        - properties: Object with emitter properties:
            - LocalSpace: Whether particles simulate in local space (bool)
            - Determinism: Whether simulation is deterministic (bool)
            - RandomSeed: Seed for deterministic random (int)
            - SimTarget: Simulation target - "CPU" or "GPU"
            - RequiresPersistentIDs: Whether persistent IDs enabled (bool)
            - MaxGPUParticlesSpawnPerFrame: Max GPU spawn per frame (int)
            - CalculateBoundsMode: Bounds mode - "Dynamic" or "Fixed"

    Example:
        get_emitter_properties(
            system="NS_FireExplosion",
            emitter="Sparks"
        )
        # Returns:
        # {
        #   "success": true,
        #   "emitter_name": "Sparks",
        #   "properties": {
        #     "LocalSpace": false,
        #     "Determinism": false,
        #     "RandomSeed": 0,
        #     "SimTarget": "GPU",
        #     "RequiresPersistentIDs": false,
        #     "MaxGPUParticlesSpawnPerFrame": 0,
        #     "CalculateBoundsMode": "Dynamic"
        #   }
        # }
    """
    params = {
        "system_path": system,
        "emitter_name": emitter
    }
    return await send_tcp_command("get_emitter_properties", params)


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
        stage: Stage to add module to - "Spawn", "Update", "Event", "EmitterSpawn", or "EmitterUpdate"
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
async def remove_module_from_emitter(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str
) -> Dict[str, Any]:
    """
    Remove a module from an emitter stage.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to remove (e.g., "SpawnRate", "Gravity Force")
        stage: Stage containing the module - "Spawn", "Update", "Event", "EmitterSpawn", or "EmitterUpdate"

    Returns:
        Dictionary containing:
        - success: Whether module was removed
        - module_name: Name of removed module
        - message: Success/error message

    Example:
        remove_module_from_emitter(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="Gravity Force",
            stage="Update"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage
    }
    return await send_tcp_command("remove_module_from_emitter", params)


@app.tool()
async def set_module_input(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    input_name: str = "",
    value: str = "",
    value_type: str = "auto",
    enabled: bool = None
) -> Dict[str, Any]:
    """
    Set an input value on a module, and/or enable/disable the module.

    This tool can be used to:
    1. Set a module input value (provide input_name and value)
    2. Enable/disable a module (provide enabled=True/False)
    3. Both at once (provide all parameters)

    Args:
        system_path: Path to the Niagara System
        emitter_name: Emitter name within the system
        module_name: Name of the module to configure
        stage: Stage containing the module - "Spawn", "Update", "Event", "EmitterSpawn", or "EmitterUpdate"
        input_name: Input parameter name (e.g., "SpawnRate", "Velocity"). Optional if only setting enabled.
        value: Value to set as string (e.g., "100", "0,0,500", "1,0.5,0,1"). Optional if only setting enabled.
        value_type: Type hint - "float", "vector", "color", "int", "bool", or "auto"
        enabled: Optional. Set to True to enable the module, False to disable it.

    Returns:
        Dictionary containing:
        - success: Whether operation was successful
        - module_name: Module that was modified
        - input_name: Input that was set (if applicable)
        - message: Success/error message

    Examples:
        # Set a module input value
        set_module_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="SpawnRate",
            stage="Spawn",
            input_name="SpawnRate",
            value="50",
            value_type="float"
        )

        # Disable a module
        set_module_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="Gravity Force",
            stage="Update",
            enabled=False
        )

        # Enable a module and set an input
        set_module_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="Scale Color",
            stage="Update",
            input_name="ScaleFactor",
            value="0.5",
            enabled=True
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
    }
    if input_name:
        params["input_name"] = input_name
    if value:
        params["value"] = str(value)
    if value_type != "auto":
        params["value_type"] = value_type
    if enabled is not None:
        params["enabled"] = enabled
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
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"
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
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"
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
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"
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
async def set_module_random_input(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    input_name: str,
    min_value: str,
    max_value: str
) -> Dict[str, Any]:
    """
    Set a random range input on a module (uniform random between min and max).

    This creates a UniformRangedFloat/Vector dynamic input that generates
    random values per particle, enabling natural variation in size, lifetime,
    velocity, and other properties.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to configure
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"
        input_name: Input parameter name (e.g., "SpriteSize", "Lifetime")
        min_value: Minimum value as string:
            - Float: "1.0"
            - Vector: "0,0,100" or "(X=0,Y=0,Z=100)"
            - Color: "1,0.5,0,1" (RGBA)
        max_value: Maximum value as string (same format as min_value)

    Returns:
        Dictionary containing:
        - success: Whether random input was set
        - module_name: Module that was modified
        - input_name: Input that was set
        - min_value: The minimum value
        - max_value: The maximum value
        - message: Success/error message

    Supported input types:
        - Float: Scalar random (e.g., particle size 5-20)
        - Int: Integer random
        - Vector2D: 2D vector random
        - Vector (Vector3): 3D vector random (e.g., velocity)
        - Vector4: 4D vector random
        - LinearColor: Color random

    Examples:
        # Random particle lifetime between 1-3 seconds
        set_module_random_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="InitializeParticle",
            stage="Spawn",
            input_name="Lifetime",
            min_value="1.0",
            max_value="3.0"
        )

        # Random sprite size between 5-20
        set_module_random_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="InitializeParticle",
            stage="Spawn",
            input_name="SpriteSize",
            min_value="5.0",
            max_value="20.0"
        )

        # Random initial velocity (upward with spread)
        set_module_random_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="InitializeParticle",
            stage="Spawn",
            input_name="Velocity",
            min_value="-50,-50,100",
            max_value="50,50,300"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "input_name": input_name,
        "min_value": str(min_value),
        "max_value": str(max_value)
    }
    return await send_tcp_command("set_module_random_input", params)


@app.tool()
async def set_module_linked_input(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    input_name: str,
    linked_value: str
) -> Dict[str, Any]:
    """
    Set a linked input on a module (binding to a particle attribute like Particles.NormalizedAge).

    This binds a module input to read from a particle attribute, enabling time-based
    animations like alpha fade over particle lifetime where the curve is sampled
    based on the particle's normalized age.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to configure
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"
        input_name: Input parameter name (e.g., "Curve Index", "Scale")
        linked_value: Particle attribute to link to (e.g., "Particles.NormalizedAge", "Particles.Velocity")

    Returns:
        Dictionary containing:
        - success: Whether linked input was set
        - module_name: Module that was modified
        - input_name: Input that was set
        - linked_value: The attribute linked to
        - message: Success/error message

    Common Linked Values:
        - Particles.NormalizedAge: 0-1 value over particle lifetime (most common for curves)
        - Particles.Age: Actual age in seconds
        - Particles.Lifetime: Total lifetime value
        - Particles.Position: Current position
        - Particles.Velocity: Current velocity
        - Particles.Color: Current color
        - Particles.Mass: Particle mass

    Example:
        # Link ScaleColor's Curve Index to NormalizedAge for alpha fade over lifetime
        set_module_linked_input(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="ScaleColor",
            stage="Update",
            input_name="Curve Index",
            linked_value="Particles.NormalizedAge"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "input_name": input_name,
        "linked_value": linked_value
    }
    return await send_tcp_command("set_module_linked_input", params)


@app.tool()
async def set_module_static_switch(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    switch_name: str,
    value: str
) -> Dict[str, Any]:
    """
    Set a static switch on a Niagara module.

    Static switches control compile-time branching in modules, enabling different
    behavior modes (e.g., "Direct Set" vs "Scale Over Life" for color modules).
    Unlike regular inputs, static switches affect which code path is compiled
    into the shader.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to configure (e.g., "Scale Color")
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"
        switch_name: Name of the static switch (e.g., "Scale Color Mode", "ScaleRGB", "ScaleA")
        value: New value - can be:
            - Display name (e.g., "Scale Over Life", "Direct Set")
            - Internal name (e.g., "NewEnumerator0", "NewEnumerator1")
            - Index (e.g., "0", "1")
            - Bool (e.g., "true", "false", "0", "1")

    Returns:
        Dictionary containing:
        - success: Whether static switch was set successfully
        - switch_name: Name of the switch that was set
        - value: The value that was set
        - message: Success/error message

    Examples:
        # Switch ScaleColor module from Direct mode to Curve mode
        set_module_static_switch(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="Scale Color",
            stage="Update",
            switch_name="Scale Color Mode",
            value="Scale Over Life"  # Or "1" for the same option
        )

        # Enable alpha scaling
        set_module_static_switch(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="Scale Color",
            stage="Update",
            switch_name="ScaleA",
            value="true"
        )

        # Disable RGB scaling
        set_module_static_switch(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="Scale Color",
            stage="Update",
            switch_name="ScaleRGB",
            value="false"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage,
        "switch_name": switch_name,
        "value": value
    }
    return await send_tcp_command("set_module_static_switch", params)


@app.tool()
async def get_module_inputs(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str
) -> Dict[str, Any]:
    """
    Get all inputs (parameters) for a module with their types and current values.

    Use this to discover what inputs a module accepts and their types before
    calling set_module_input. This helps avoid trial-and-error when setting values.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        module_name: Name of the module to query (e.g., "InitializeParticle", "Color")
        stage: Stage containing the module - "Spawn", "Update", "EmitterSpawn", or "EmitterUpdate"

    Returns:
        Dictionary containing:
        - success: Whether query was successful
        - module_name: Name of the module
        - emitter_name: Name of the emitter
        - stage: Stage the module is in
        - inputs: Array of input info objects with:
            - name: Simple input name (e.g., "Lifetime", "Color")
            - full_name: Full qualified name (e.g., "Module.Lifetime")
            - type: Niagara type name (e.g., "NiagaraFloat", "Vector3f", "LinearColor")
            - value: Current value as string

    Example:
        get_module_inputs(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            module_name="InitializeParticle",
            stage="Spawn"
        )
        # Returns:
        # {
        #   "success": true,
        #   "module_name": "Initialize Particle",
        #   "inputs": [
        #     {"name": "Lifetime", "type": "NiagaraFloat", "value": "2.0"},
        #     {"name": "Color", "type": "LinearColor", "value": "(R=1,G=1,B=1,A=1)"},
        #     {"name": "SpriteSize", "type": "Vector2f", "value": "(X=10,Y=10)"},
        #     ...
        #   ]
        # }
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "module_name": module_name,
        "stage": stage
    }
    return await send_tcp_command("get_module_inputs", params)


@app.tool()
async def get_emitter_modules(
    system_path: str,
    emitter_name: str
) -> Dict[str, Any]:
    """
    Get all modules in an emitter organized by stage.

    This is essential for discovering what modules exist in an emitter before
    using get_module_inputs or set_module_input. Returns modules grouped by
    stage (EmitterSpawn, EmitterUpdate, ParticleSpawn, ParticleUpdate, Event)
    with their names, indices, and enabled states.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")

    Returns:
        Dictionary containing:
        - success: Whether query was successful
        - emitter_name: Name of the emitter
        - system_path: Path to the system
        - stages: Object with stage arrays:
            - EmitterSpawn: Array of modules in emitter spawn stage
            - EmitterUpdate: Array of modules in emitter update stage (spawn rate, etc.)
            - ParticleSpawn: Array of modules in particle spawn stage
            - ParticleUpdate: Array of modules in particle update stage
            - Event: Array of modules in event handlers (if any)
        - Each module has:
            - name: Module name (e.g., "Initialize Particle")
            - index: Position in the stack (0-based)
            - enabled: Whether the module is enabled
            - script_path: Path to the module script asset (if available)
        - total_module_count: Total number of modules
        - emitter_spawn_count: Number of emitter spawn modules
        - emitter_update_count: Number of emitter update modules
        - spawn_count: Number of particle spawn modules
        - update_count: Number of particle update modules
        - event_count: Number of event modules (if any)

    Example:
        get_emitter_modules(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks"
        )
        # Returns:
        # {
        #   "success": true,
        #   "emitter_name": "NE_Sparks",
        #   "stages": {
        #     "EmitterSpawn": [...],
        #     "EmitterUpdate": [
        #       {"name": "Spawn Rate", "index": 0, "enabled": true}
        #     ],
        #     "ParticleSpawn": [
        #       {"name": "Initialize Particle", "index": 0, "enabled": true},
        #       {"name": "Add Velocity in Cone", "index": 1, "enabled": true}
        #     ],
        #     "ParticleUpdate": [
        #       {"name": "Gravity Force", "index": 0, "enabled": true},
        #       {"name": "Drag", "index": 1, "enabled": true},
        #       {"name": "Scale Color", "index": 2, "enabled": true}
        #     ]
        #   },
        #   "total_module_count": 6,
        #   "emitter_spawn_count": 0,
        #   "emitter_update_count": 1,
        #   "spawn_count": 2,
        #   "update_count": 3
        # }
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name
    }
    return await send_tcp_command("get_emitter_modules", params)


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
async def get_renderer_properties(
    system_path: str,
    emitter_name: str,
    renderer_name: str = "Renderer"
) -> Dict[str, Any]:
    """
    Get all properties and bindings from a renderer.

    This is useful for debugging renderer configuration, verifying that
    properties were set correctly, and understanding available settings.

    Args:
        system_path: Path to the Niagara System (e.g., "/Game/Effects/NS_Fire")
        emitter_name: Emitter name within the system (e.g., "NE_Sparks")
        renderer_name: Name of the renderer (default: "Renderer")

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - renderer_type: Type of renderer (e.g., "NiagaraSpriteRendererProperties")
        - properties: Object with all renderer properties:
            - Material: Asset path to the material
            - Alignment: Sprite alignment mode (e.g., "VelocityAligned")
            - FacingMode: Sprite facing mode (e.g., "FaceCamera")
            - SortMode: Particle sort mode (e.g., "ViewDepth")
            - SubImageSize: [X, Y] sub-image dimensions
            - bSubImageBlend: Whether sub-image blending is enabled
            - PivotInUVSpace: [X, Y] pivot point in UV space
            - MacroUVRadius: Macro UV radius
            - (many more depending on renderer type)
        - bindings: Object with attribute bindings:
            - PositionBinding: Bound attribute (e.g., "Particles.Position")
            - ColorBinding: Bound attribute (e.g., "Particles.Color")
            - SizeBinding: Bound attribute (e.g., "Particles.SpriteSize")
            - (etc.)

    Example:
        get_renderer_properties(
            system_path="/Game/Effects/NS_Fire",
            emitter_name="NE_Sparks",
            renderer_name="Renderer"
        )
    """
    params = {
        "system_path": system_path,
        "emitter_name": emitter_name,
        "renderer_name": renderer_name
    }
    return await send_tcp_command("get_renderer_properties", params)


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
