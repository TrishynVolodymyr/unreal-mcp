#!/usr/bin/env python3
"""
Material MCP Server

This server provides MCP tools for Material Instance operations in Unreal Engine,
including creating material instances, setting parameters (scalar, vector, texture),
batch operations, and metadata retrieval.
"""

import asyncio
import json
from typing import Any, Dict, List

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("Material MCP Server")

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
# Material Instance Creation
# ============================================================================

@app.tool()
async def create_material_instance(
    name: str,
    parent_material: str,
    folder_path: str = "",
    scalar_params: str = "",
    vector_params: str = "",
    texture_params: str = ""
) -> Dict[str, Any]:
    """
    Create a new Material Instance Constant from a parent material.

    This is the primary way to create variations of materials with different
    parameter values without modifying the original master material.

    Args:
        name: Name of the Material Instance (e.g., "MI_Crystal_Red")
        parent_material: Path or name of the parent material (e.g., "/Game/Materials/M_Crystal" or "M_Crystal")
        folder_path: Optional folder path for the new asset (e.g., "/Game/Materials/Instances")
        scalar_params: Optional JSON string of scalar parameters {"ParamName": 0.5, ...}
        vector_params: Optional JSON string of vector parameters {"ParamName": [R, G, B, A], ...}
        texture_params: Optional JSON string of texture parameters {"ParamName": "/Game/Textures/T_Name", ...}

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - name: Name of the created Material Instance
        - path: Full path to the created asset
        - parent: Path to the parent material
        - message: Success/error message

    Example:
        create_material_instance(
            name="MI_Crystal_Red",
            parent_material="/Game/Materials/M_Crystal",
            folder_path="/Game/Materials/Instances",
            scalar_params='{"Metallic": 0.8, "Roughness": 0.2}',
            vector_params='{"BaseColor": [1.0, 0.0, 0.0, 1.0]}'
        )
    """
    params = {
        "name": name,
        "parent_material": parent_material
    }
    if folder_path:
        params["folder_path"] = folder_path
    if scalar_params:
        params["scalar_params"] = scalar_params
    if vector_params:
        params["vector_params"] = vector_params
    if texture_params:
        params["texture_params"] = texture_params

    return await send_tcp_command("create_material_instance", params)


@app.tool()
async def duplicate_material_instance(
    source_material_instance: str,
    new_name: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """
    Duplicate an existing Material Instance to create a variation.

    This is useful for creating variations of an existing material instance
    while preserving all current parameter values.

    Args:
        source_material_instance: Path or name of the source Material Instance
        new_name: Name for the new duplicated Material Instance
        folder_path: Optional folder path for the new asset

    Returns:
        Dictionary containing:
        - success: Whether duplication was successful
        - name: Name of the duplicated Material Instance
        - path: Full path to the new asset
        - parent: Path to the parent material
        - message: Success/error message

    Example:
        duplicate_material_instance(
            source_material_instance="MI_Crystal_Red",
            new_name="MI_Crystal_DarkRed",
            folder_path="/Game/Materials/Instances"
        )
    """
    params = {
        "source_material_instance": source_material_instance,
        "new_name": new_name
    }
    if folder_path:
        params["folder_path"] = folder_path

    return await send_tcp_command("duplicate_material_instance", params)


# ============================================================================
# Scalar Parameter Operations
# ============================================================================

@app.tool()
async def set_material_scalar_param(
    material_instance: str,
    param_name: str,
    value: float
) -> Dict[str, Any]:
    """
    Set a scalar (float) parameter on a Material Instance.

    Scalar parameters control single float values like metallic, roughness,
    opacity, emissive intensity, tiling, etc.

    Args:
        material_instance: Path or name of the Material Instance
        param_name: Name of the scalar parameter (e.g., "Metallic", "Roughness")
        value: Float value to set (e.g., 0.5, 1.0)

    Returns:
        Dictionary containing:
        - success: Whether the parameter was set successfully
        - material_instance: Name of the Material Instance
        - param_name: Name of the parameter
        - value: The value that was set
        - message: Success/error message

    Example:
        set_material_scalar_param(
            material_instance="MI_Crystal_Red",
            param_name="Roughness",
            value=0.3
        )
    """
    params = {
        "material_instance": material_instance,
        "parameter_name": param_name,
        "value": value
    }
    return await send_tcp_command("set_material_scalar_param", params)


# ============================================================================
# Vector Parameter Operations
# ============================================================================

@app.tool()
async def set_material_vector_param(
    material_instance: str,
    param_name: str,
    r: float,
    g: float,
    b: float,
    a: float = 1.0
) -> Dict[str, Any]:
    """
    Set a vector (color/4D vector) parameter on a Material Instance.

    Vector parameters control color values and 4-component vectors like
    BaseColor, EmissiveColor, tint colors, UV offsets, etc.

    Args:
        material_instance: Path or name of the Material Instance
        param_name: Name of the vector parameter (e.g., "BaseColor", "EmissiveColor")
        r: Red component (0.0-1.0 for colors, can be any value for vectors)
        g: Green component
        b: Blue component
        a: Alpha component (default: 1.0)

    Returns:
        Dictionary containing:
        - success: Whether the parameter was set successfully
        - material_instance: Name of the Material Instance
        - param_name: Name of the parameter
        - value: Array [R, G, B, A] that was set
        - message: Success/error message

    Example:
        set_material_vector_param(
            material_instance="MI_Crystal_Red",
            param_name="BaseColor",
            r=1.0, g=0.0, b=0.0, a=1.0
        )
    """
    params = {
        "material_instance": material_instance,
        "parameter_name": param_name,
        "value": [r, g, b, a]
    }
    return await send_tcp_command("set_material_vector_param", params)


# ============================================================================
# Texture Parameter Operations
# ============================================================================

@app.tool()
async def set_material_texture_param(
    material_instance: str,
    param_name: str,
    texture_path: str
) -> Dict[str, Any]:
    """
    Set a texture parameter on a Material Instance.

    Texture parameters allow assigning different textures for diffuse, normal,
    roughness maps, masks, and other texture slots.

    Args:
        material_instance: Path or name of the Material Instance
        param_name: Name of the texture parameter (e.g., "BaseColorMap", "NormalMap")
        texture_path: Path to the texture asset (e.g., "/Game/Textures/T_Rock_D")

    Returns:
        Dictionary containing:
        - success: Whether the parameter was set successfully
        - material_instance: Name of the Material Instance
        - param_name: Name of the parameter
        - texture: Path to the texture that was set
        - message: Success/error message

    Example:
        set_material_texture_param(
            material_instance="MI_Crystal_Red",
            param_name="NormalMap",
            texture_path="/Game/Textures/T_Crystal_N"
        )
    """
    params = {
        "material_instance": material_instance,
        "parameter_name": param_name,
        "texture_path": texture_path
    }
    return await send_tcp_command("set_material_texture_param", params)


# ============================================================================
# Batch Operations
# ============================================================================

@app.tool()
async def batch_set_material_params(
    material_instance: str,
    scalar_params: str = "",
    vector_params: str = "",
    texture_params: str = ""
) -> Dict[str, Any]:
    """
    Set multiple parameters on a Material Instance in a single operation.

    This is more efficient than setting parameters one at a time when
    configuring multiple values. All parameter types can be mixed in one call.

    Args:
        material_instance: Path or name of the Material Instance
        scalar_params: JSON string of scalar parameters {"ParamName": 0.5, ...}
        vector_params: JSON string of vector parameters {"ParamName": [R, G, B, A], ...}
        texture_params: JSON string of texture parameters {"ParamName": "/Game/Textures/T_Name", ...}

    Returns:
        Dictionary containing:
        - success: Whether all parameters were set successfully
        - material_instance: Name of the Material Instance
        - results: Object with scalar, vector, texture arrays showing what was set
        - message: Success/error message

    Example:
        batch_set_material_params(
            material_instance="MI_Crystal_Red",
            scalar_params='{"Metallic": 0.9, "Roughness": 0.1, "EmissiveIntensity": 2.0}',
            vector_params='{"BaseColor": [0.8, 0.0, 0.0, 1.0], "EmissiveColor": [1.0, 0.3, 0.0, 1.0]}',
            texture_params='{"NormalMap": "/Game/Textures/T_Crystal_N"}'
        )
    """
    params = {
        "material_instance": material_instance
    }
    if scalar_params:
        params["scalar_params"] = scalar_params
    if vector_params:
        params["vector_params"] = vector_params
    if texture_params:
        params["texture_params"] = texture_params

    return await send_tcp_command("batch_set_material_params", params)


# ============================================================================
# Metadata and Discovery
# ============================================================================

@app.tool()
async def get_material_instance_metadata(
    material_instance: str
) -> Dict[str, Any]:
    """
    Get metadata and current parameter values from a Material Instance.

    Retrieves all information about a Material Instance including its parent,
    all overridden parameters, and their current values.

    Args:
        material_instance: Path or name of the Material Instance

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - name: Name of the Material Instance
        - path: Full asset path
        - parent: Parent material path
        - scalar_parameters: Array of {name, value} for scalar params
        - vector_parameters: Array of {name, value: [R,G,B,A]} for vector params
        - texture_parameters: Array of {name, texture_path} for texture params

    Example:
        get_material_instance_metadata(material_instance="MI_Crystal_Red")
    """
    params = {
        "material_instance": material_instance
    }
    return await send_tcp_command("get_material_instance_metadata", params)


@app.tool()
async def get_material_parameters(
    material: str
) -> Dict[str, Any]:
    """
    Get all available parameters from a Material (parent material or instance).

    This is useful for discovering what parameters are available on a material
    before creating instances or setting values. Shows parameter names, types,
    groups, and current/default values.

    Args:
        material: Path or name of the Material or Material Instance

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - material: Path to the material
        - parameters: Array of parameter info objects with:
            - name: Parameter name
            - type: Parameter type (Scalar, Vector, Texture)
            - current_value: Current value as string
            - group: Parameter group for organization

    Example:
        get_material_parameters(material="/Game/Materials/M_Crystal")
    """
    params = {
        "material": material
    }
    return await send_tcp_command("get_material_parameters", params)


# ============================================================================
# Run Server
# ============================================================================

if __name__ == "__main__":
    app.run()
