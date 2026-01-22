#!/usr/bin/env python3
"""
Material MCP Server - Material and Material Instance tools for Unreal Engine.
Includes: create_material, create_material_instance, set parameters, batch ops.
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
# Material Creation
# ============================================================================

@app.tool()
async def create_material(
    name: str,
    path: str = "/Game/Materials",
    blend_mode: str = "Translucent",
    shading_model: str = "Unlit",
    used_with_niagara_sprites: bool = False,
    used_with_niagara_ribbons: bool = False,
    used_with_niagara_mesh_particles: bool = False,
    used_with_particle_sprites: bool = False,
    used_with_mesh_particles: bool = False,
    used_with_skeletal_mesh: bool = False,
    used_with_static_lighting: bool = False
) -> Dict[str, Any]:
    """
    Create a new base Material asset.

    This creates a new material that can be used as a parent for material instances
    or assigned directly to meshes/particles.

    Args:
        name: Name of the material (e.g., "M_EmberParticle")
        path: Folder path for the material (default: "/Game/Materials")
        blend_mode: Blend mode - "Opaque", "Masked", "Translucent", "Additive" (default: "Translucent")
        shading_model: Shading model - "DefaultLit", "Unlit", "SubsurfaceProfile" (default: "Unlit")
        used_with_niagara_sprites: Enable for Niagara sprite particles (default: False)
        used_with_niagara_ribbons: Enable for Niagara ribbon particles (default: False)
        used_with_niagara_mesh_particles: Enable for Niagara mesh particles (default: False)
        used_with_particle_sprites: Enable for legacy Cascade sprite particles (default: False)
        used_with_mesh_particles: Enable for legacy Cascade mesh particles (default: False)
        used_with_skeletal_mesh: Enable for skeletal meshes (default: False)
        used_with_static_lighting: Enable for static lighting (default: False)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - material_path: Full path to the created material
        - message: Success/error message

    Example:
        create_material(
            name="M_EmberParticle",
            path="/Game/Effects/Materials",
            blend_mode="Additive",
            shading_model="Unlit",
            used_with_niagara_sprites=True
        )
    """
    params = {
        "name": name,
        "path": path,
        "blend_mode": blend_mode,
        "shading_model": shading_model,
        "used_with_niagara_sprites": used_with_niagara_sprites,
        "used_with_niagara_ribbons": used_with_niagara_ribbons,
        "used_with_niagara_mesh_particles": used_with_niagara_mesh_particles,
        "used_with_particle_sprites": used_with_particle_sprites,
        "used_with_mesh_particles": used_with_mesh_particles,
        "used_with_skeletal_mesh": used_with_skeletal_mesh,
        "used_with_static_lighting": used_with_static_lighting
    }

    return await send_tcp_command("create_material", params)


# ============================================================================
# Material Instance Creation
# ============================================================================

@app.tool()
async def create_material_instance(
    name: str,
    parent_material: str,
    folder_path: str = "",
    scalar_params: dict = None,
    vector_params: dict = None,
    texture_params: dict = None
) -> Dict[str, Any]:
    """
    Create a new Material Instance Constant from a parent material.

    This is the primary way to create variations of materials with different
    parameter values without modifying the original master material.

    Args:
        name: Name of the Material Instance (e.g., "MI_Crystal_Red")
        parent_material: Path or name of the parent material (e.g., "/Game/Materials/M_Crystal" or "M_Crystal")
        folder_path: Optional folder path for the new asset (e.g., "/Game/Materials/Instances")
        scalar_params: Optional dictionary of scalar parameters {"ParamName": 0.5, ...}
        vector_params: Optional dictionary of vector parameters {"ParamName": [R, G, B, A], ...}
        texture_params: Optional dictionary of texture parameters {"ParamName": "/Game/Textures/T_Name", ...}

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
            scalar_params={"Metallic": 0.8, "Roughness": 0.2},
            vector_params={"BaseColor": [1.0, 0.0, 0.0, 1.0]}
        )
    """
    params = {
        "name": name,
        "parent_material": parent_material
    }
    if folder_path:
        params["folder_path"] = folder_path
    # Convert dict to JSON string for C++ side (which expects JSON strings)
    if scalar_params:
        params["scalar_params"] = json.dumps(scalar_params) if isinstance(scalar_params, dict) else scalar_params
    if vector_params:
        params["vector_params"] = json.dumps(vector_params) if isinstance(vector_params, dict) else vector_params
    if texture_params:
        params["texture_params"] = json.dumps(texture_params) if isinstance(texture_params, dict) else texture_params

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
    scalar_params: dict = None,
    vector_params: dict = None,
    texture_params: dict = None
) -> Dict[str, Any]:
    """
    Set multiple parameters on a Material Instance in a single operation.

    This is more efficient than setting parameters one at a time when
    configuring multiple values. All parameter types can be mixed in one call.

    Args:
        material_instance: Path or name of the Material Instance
        scalar_params: Dictionary of scalar parameters {"ParamName": 0.5, ...}
        vector_params: Dictionary of vector parameters {"ParamName": [R, G, B, A], ...}
        texture_params: Dictionary of texture parameters {"ParamName": "/Game/Textures/T_Name", ...}

    Returns:
        Dictionary containing:
        - success: Whether all parameters were set successfully
        - material_instance: Name of the Material Instance
        - results: Object with scalar, vector, texture arrays showing what was set
        - message: Success/error message

    Example:
        batch_set_material_params(
            material_instance="MI_Crystal_Red",
            scalar_params={"Metallic": 0.9, "Roughness": 0.1, "EmissiveIntensity": 2.0},
            vector_params={"BaseColor": [0.8, 0.0, 0.0, 1.0], "EmissiveColor": [1.0, 0.3, 0.0, 1.0]},
            texture_params={"NormalMap": "/Game/Textures/T_Crystal_N"}
        )
    """
    params = {
        "material_instance": material_instance
    }
    # Convert dict to JSON string for C++ side (which expects JSON strings)
    if scalar_params:
        params["scalar_params"] = json.dumps(scalar_params) if isinstance(scalar_params, dict) else scalar_params
    if vector_params:
        params["vector_params"] = json.dumps(vector_params) if isinstance(vector_params, dict) else vector_params
    if texture_params:
        params["texture_params"] = json.dumps(texture_params) if isinstance(texture_params, dict) else texture_params

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
# Material Expression Tools
# ============================================================================

@app.tool()
async def get_material_graph_metadata(
    material_path: str,
    fields: List[str] = None
) -> Dict[str, Any]:
    """
    Get comprehensive metadata about a material's expression graph.

    Retrieves all expressions, connections, material outputs, orphan nodes, and
    data flow paths from a base Material (not Material Instance). Essential for
    understanding material structure before modifying expressions.

    Args:
        material_path: Path to the material (e.g., "/Game/Materials/M_MyMaterial")
        fields: Optional list to filter which fields to return. Available fields:
            - "expressions": All expression nodes with id, type, position, inputs, outputs
            - "connections": All connections between expressions
            - "material_outputs": Which expressions connect to BaseColor, Emissive, etc.
            - "orphans": Expressions with no connected outputs (cleanup candidates)
            - "flow": Traced data flow from sources to material outputs

    Returns:
        Dictionary containing:
        - success: Whether retrieval was successful
        - expressions: Array of expression objects with:
            - expression_id: GUID for the expression
            - expression_type: Type name (e.g., "Constant", "Multiply", "TextureSample")
            - position: [X, Y] position in graph
            - description: Human-readable description
            - inputs: Array of input pins
            - outputs: Array of output pins with connections
        - connections: Array of connection objects
        - material_outputs: Object mapping property names to connected expressions
        - orphans: Array of expressions with unused outputs
        - flow: Traced paths from sources to outputs

    Example:
        # Get full graph metadata
        get_material_graph_metadata(material_path="/Game/VFX/Fireball/M_FireballProjectile_v2")

        # Get only expressions and outputs
        get_material_graph_metadata(
            material_path="/Game/Materials/M_Crystal",
            fields=["expressions", "material_outputs"]
        )
    """
    params = {"material_path": material_path}
    if fields:
        params["fields"] = fields

    return await send_tcp_command("get_material_expression_metadata", params)


@app.tool()
async def set_material_expression_property(
    material_path: str,
    expression_id: str,
    property_name: str,
    property_value: str
) -> Dict[str, Any]:
    """
    Set a property on an existing material expression node.

    Allows modifying properties of expressions already in a material graph,
    such as changing the texture on a TextureSample node or the value of a Constant.

    Args:
        material_path: Path to the material (e.g., "/Game/Materials/M_MyMaterial")
        expression_id: GUID of the expression to modify (from get_material_graph_metadata)
        property_name: Name of the property to set. Common properties:
            - For TextureSample: "Texture" (texture path)
            - For Constant: "R" (float value)
            - For Constant3Vector: "Constant" (color as "R,G,B")
            - For Constant4Vector: "Constant" (color as "R,G,B,A")
            - For TextureCoordinate: "UTiling", "VTiling"
            - For ScalarParameter: "DefaultValue", "ParameterName"
        property_value: Value to set (as string, number, or path depending on property type)

    Returns:
        Dictionary containing:
        - success: Whether the property was set successfully
        - property_name: Name of the property that was modified
        - message: Success/error message

    Example:
        # Change texture on a TextureSample node
        set_material_expression_property(
            material_path="/Game/VFX/Fireball/M_FireballProjectile_v2",
            expression_id="A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6",
            property_name="Texture",
            property_value="/Game/VFX/Fireball/T_Fireball_density"
        )

        # Change a constant value
        set_material_expression_property(
            material_path="/Game/Materials/M_Glow",
            expression_id="X1Y2Z3...",
            property_name="R",
            property_value=2.5
        )
    """
    params = {
        "material_path": material_path,
        "expression_id": expression_id,
        "property_name": property_name,
        "property_value": property_value
    }

    return await send_tcp_command("set_material_expression_property", params)


@app.tool()
async def add_material_expression(
    material_path: str,
    expression_type: str,
    position: List[int] = None,
    properties: dict = None
) -> Dict[str, Any]:
    """
    Add a material expression node to a material graph.

    Args:
        material_path: Path to the material (e.g., "/Game/Materials/M_MyMaterial")
        expression_type: Type of expression (e.g., "Constant", "Multiply", "ParticleColor",
                        "RadialGradientExponential", "VertexColor", "TextureCoordinate")
        position: [X, Y] position in the graph (optional)
        properties: Dictionary of expression properties (optional)

    Returns:
        Dictionary with expression_id, name, and other info

    Example:
        add_material_expression(
            material_path="/Game/Materials/M_Ember",
            expression_type="Constant",
            properties={"R": 1.0}
        )
    """
    params = {
        "material_path": material_path,
        "expression_type": expression_type
    }
    if position:
        params["position"] = position
    if properties:
        params["properties"] = properties

    return await send_tcp_command("add_material_expression", params)


@app.tool()
async def connect_material_expressions(
    material_path: str,
    source_expression_id: str,
    target_expression_id: str,
    target_input_name: str,
    source_output_index: int = 0
) -> Dict[str, Any]:
    """
    Connect two material expressions.

    Args:
        material_path: Path to the material
        source_expression_id: GUID of the source expression
        target_expression_id: GUID of the target expression
        target_input_name: Name of the input on the target (e.g., "A", "B", "Base")
        source_output_index: Output index on source (default 0)

    Returns:
        Success/error message
    """
    params = {
        "material_path": material_path,
        "source_expression_id": source_expression_id,
        "target_expression_id": target_expression_id,
        "target_input_name": target_input_name,
        "source_output_index": source_output_index
    }

    return await send_tcp_command("connect_material_expressions", params)


@app.tool()
async def connect_expression_to_material_output(
    material_path: str,
    expression_id: str,
    material_property: str,
    output_index: int = 0
) -> Dict[str, Any]:
    """
    Connect a material expression to a material output (EmissiveColor, Opacity, etc.).

    Args:
        material_path: Path to the material (e.g., "/Game/Materials/M_MyMaterial")
        expression_id: GUID of the source expression
        material_property: Target material property - one of:
            - "BaseColor": Surface color
            - "Metallic": Metallic value
            - "Specular": Specular value
            - "Roughness": Roughness value
            - "Normal": Normal map
            - "EmissiveColor": Emissive/glow color
            - "Opacity": Opacity for translucent materials
            - "OpacityMask": Opacity mask for masked materials
            - "WorldPositionOffset": Vertex offset
            - "AmbientOcclusion": AO value
            - "Refraction": Refraction for translucent materials
            - "SubsurfaceColor": Subsurface scattering color
        output_index: Output index on source expression (default 0)

    Returns:
        Success/error message with connected property

    Example:
        connect_expression_to_material_output(
            material_path="/Game/Materials/M_Ember",
            expression_id="43E2F0AD4A059DB305340EAB5A873C46",
            material_property="EmissiveColor"
        )
    """
    params = {
        "material_path": material_path,
        "expression_id": expression_id,
        "material_property": material_property,
        "output_index": output_index
    }

    return await send_tcp_command("connect_expression_to_material_output", params)


# ============================================================================
# Material Palette Search
# ============================================================================

@app.tool()
async def compile_material(
    material_path: str
) -> Dict[str, Any]:
    """
    Compile a material to apply changes and trigger shader recompilation.

    Use this after creating a material with usage flags (like used_with_niagara_sprites)
    to ensure shaders are compiled with the correct permutations.

    Args:
        material_path: Path to the material (e.g., "/Game/Materials/M_MyMaterial")

    Returns:
        Dictionary containing:
        - success: Whether compilation was successful
        - message: Success/error message

    Example:
        compile_material(material_path="/Game/Materials/M_FireEmber")
    """
    params = {"material_path": material_path}
    return await send_tcp_command("compile_material", params)


@app.tool()
async def delete_material_expression(
    material_path: str,
    expression_id: str
) -> Dict[str, Any]:
    """
    Delete a material expression from a material graph.

    Use this to remove orphan expressions or clean up unused nodes from materials.

    Args:
        material_path: Path to the material (e.g., "/Game/Materials/M_MyMaterial")
        expression_id: GUID of the expression to delete

    Returns:
        Dictionary containing:
        - success: Whether deletion was successful
        - message: Success/error message

    Example:
        delete_material_expression(
            material_path="/Game/Materials/M_Ember",
            expression_id="E880D7F146304B1CC5ECF2AA1697DF33"
        )
    """
    params = {
        "material_path": material_path,
        "expression_id": expression_id
    }

    return await send_tcp_command("delete_material_expression", params)


@app.tool()
async def search_material_palette(
    search_query: str = "",
    category_filter: str = "",
    type_filter: str = "All",
    max_results: int = 50
) -> Dict[str, Any]:
    """
    Search the Material Palette for expressions and functions.

    Uses UE's built-in MaterialExpressionClasses and Asset Registry to find
    available material nodes that can be added to materials.

    Args:
        search_query: Text to search in names (case-insensitive). Leave empty to list all.
        category_filter: Filter by category name (e.g., "Math", "Texture", "Utility")
        type_filter: Filter by type - "Expression", "Function", or "All" (default)
        max_results: Maximum results to return (default: 50)

    Returns:
        Dictionary containing:
        - success: Whether search was successful
        - results: Array of items with:
            - type: "Expression" or "Function"
            - name: Display name
            - category: Category name(s)
            - class_name: For expressions, the UClass name (use for add_material_expression)
            - path: For functions, the asset path (use for MaterialFunctionCall)
            - description: Tooltip/description if available
        - total_count: Total results before limit
        - returned_count: Number of results returned
        - categories: List of available categories

    Examples:
        # Search for radial gradient related nodes
        search_material_palette(search_query="radial")

        # List all texture-related expressions
        search_material_palette(category_filter="Texture", type_filter="Expression")

        # Find all available material functions
        search_material_palette(type_filter="Function", max_results=100)

        # List all categories (empty search, low limit)
        search_material_palette(max_results=1)
    """
    params = {
        "search_query": search_query,
        "category_filter": category_filter,
        "type_filter": type_filter,
        "max_results": max_results
    }

    return await send_tcp_command("search_material_palette", params)


# ============================================================================
# Run Server
# ============================================================================

if __name__ == "__main__":
    app.run()
