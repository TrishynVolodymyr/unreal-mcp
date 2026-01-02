"""
Material Tools for Unreal MCP.

This module provides tools for creating and manipulating Unreal Engine materials.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context
from utils.materials.material_operations import (
    create_material as create_material_impl,
    create_material_instance as create_material_instance_impl,
    get_material_metadata as get_material_metadata_impl,
    set_material_parameter as set_material_parameter_impl,
    get_material_parameter as get_material_parameter_impl,
    apply_material_to_actor as apply_material_to_actor_impl,
    # Material expression operations
    add_material_expression as add_material_expression_impl,
    connect_material_expressions as connect_material_expressions_impl,
    connect_expression_to_material_output as connect_expression_to_material_output_impl,
    get_material_graph_metadata as get_material_graph_metadata_impl,
    delete_material_expression as delete_material_expression_impl,
    set_material_expression_property as set_material_expression_property_impl
)

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_material_tools(mcp: FastMCP):
    """Register material tools with the MCP server."""

    @mcp.tool()
    def create_material(
        ctx: Context,
        name: str,
        path: str = "/Game/Materials",
        blend_mode: str = "Opaque",
        shading_model: str = "DefaultLit"
    ) -> Dict[str, Any]:
        """
        Create a new Material asset.

        Args:
            name: Name of the material to create (e.g., "M_MyMaterial")
            path: Content path where the material should be created
            blend_mode: Blend mode for the material. Options:
                - "Opaque": Fully opaque surface (default)
                - "Masked": Binary transparency using opacity mask
                - "Translucent": Partial transparency with alpha blending
                - "Additive": Adds color to background
                - "Modulate": Multiplies against background
            shading_model: Shading model for the material. Options:
                - "DefaultLit": Standard PBR lighting (default)
                - "Unlit": No lighting, emissive only
                - "Subsurface": For skin, wax, etc.
                - "ClearCoat": For car paint, lacquered surfaces
                - "TwoSidedFoliage": For leaves and foliage

        Returns:
            Dict containing:
                - success: Whether the material was created
                - material_path: Full path to the created material
                - message: Status message

        Examples:
            # Create a basic opaque material
            create_material(name="M_WoodFloor")

            # Create a translucent material
            create_material(
                name="M_Glass",
                path="/Game/Materials/Glass",
                blend_mode="Translucent"
            )

            # Create an unlit emissive material
            create_material(
                name="M_Hologram",
                shading_model="Unlit",
                blend_mode="Additive"
            )
        """
        return create_material_impl(ctx, name, path, blend_mode, shading_model)

    @mcp.tool()
    def create_material_instance(
        ctx: Context,
        name: str,
        parent_material: str,
        path: str = "/Game/Materials",
        is_dynamic: bool = False
    ) -> Dict[str, Any]:
        """
        Create a Material Instance from a parent material.

        Material Instances allow you to create variations of a base material
        by overriding its parameters without duplicating the shader.

        Args:
            name: Name of the material instance (e.g., "MI_WoodFloor_Dark")
            parent_material: Path to the parent material (e.g., "/Game/Materials/M_WoodFloor")
            path: Content path where the instance should be created
            is_dynamic: If True, creates a MaterialInstanceDynamic (runtime modifiable).
                       If False (default), creates a MaterialInstanceConstant (editor-time).

        Returns:
            Dict containing:
                - success: Whether the instance was created
                - instance_path: Full path to the created instance
                - instance_type: "MaterialInstanceDynamic" or "MaterialInstanceConstant"
                - parent_material: Path to the parent material
                - message: Status message

        Examples:
            # Create a static material instance
            create_material_instance(
                name="MI_WoodFloor_Oak",
                parent_material="/Game/Materials/M_WoodFloor"
            )

            # Create a dynamic material instance (for runtime changes)
            create_material_instance(
                name="MID_PlayerColor",
                parent_material="/Game/Materials/M_Character",
                is_dynamic=True
            )
        """
        return create_material_instance_impl(ctx, name, parent_material, path, is_dynamic)

    @mcp.tool()
    def get_material_metadata(
        ctx: Context,
        material_path: str,
        fields: List[str] = None
    ) -> Dict[str, Any]:
        """
        Get metadata about a material, including all its parameters.

        Args:
            material_path: Full path to the material (e.g., "/Game/Materials/M_WoodFloor")
            fields: Optional list of fields to retrieve. If None, returns all. Options:
                - "basic": Name, path, type, parent (for instances)
                - "parameters": All scalar, vector, and texture parameters
                - "properties": Blend mode, shading model, two-sided, etc.
                - "*": All fields (default)

        Returns:
            Dict containing:
                - success: Whether the material was found
                - name: Material name
                - path: Full asset path
                - type: "Material", "MaterialInstanceConstant", or "MaterialInstanceDynamic"
                - parent_material: (For instances) Path to parent material
                - scalar_parameters: List of scalar parameter info
                - vector_parameters: List of vector/color parameter info
                - texture_parameters: List of texture parameter info
                - blend_mode: Current blend mode
                - shading_model: Current shading model
                - is_two_sided: Whether the material is two-sided
                - message: Status message

        Examples:
            # Get all metadata for a material
            get_material_metadata(material_path="/Game/Materials/M_WoodFloor")

            # Get only parameter information
            get_material_metadata(
                material_path="/Game/Materials/M_WoodFloor",
                fields=["parameters"]
            )
        """
        return get_material_metadata_impl(ctx, material_path, fields)

    @mcp.tool()
    def set_material_parameter(
        ctx: Context,
        material_path: str,
        parameter_name: str,
        parameter_type: str,
        value
    ) -> Dict[str, Any]:
        """
        Set a parameter value on a material instance.

        Note: This works on MaterialInstanceDynamic and MaterialInstanceConstant.
        For base Materials, you need to modify the material graph directly.

        Args:
            material_path: Full path to the material instance
            parameter_name: Name of the parameter to set (must exist on parent material)
            parameter_type: Type of parameter. Options:
                - "scalar": Float value (e.g., roughness, metallic, opacity)
                - "vector": RGBA color/vector as [R, G, B, A] (values 0.0-1.0)
                - "texture": Path to a texture asset
            value: The value to set:
                - For "scalar": float (e.g., 0.5)
                - For "vector": list of 4 floats [R, G, B, A] (e.g., [1.0, 0.0, 0.0, 1.0])
                - For "texture": string path (e.g., "/Game/Textures/T_Wood_D")

        Returns:
            Dict containing:
                - success: Whether the parameter was set
                - parameter_name: Name of the parameter
                - parameter_type: Type of the parameter
                - previous_value: Value before the change (if available)
                - new_value: The new value that was set
                - message: Status message

        Examples:
            # Set a scalar parameter (roughness)
            set_material_parameter(
                material_path="/Game/Materials/MI_Metal",
                parameter_name="Roughness",
                parameter_type="scalar",
                value=0.2
            )

            # Set a vector/color parameter
            set_material_parameter(
                material_path="/Game/Materials/MI_Glow",
                parameter_name="EmissiveColor",
                parameter_type="vector",
                value=[1.0, 0.5, 0.0, 1.0]  # Orange glow
            )

            # Set a texture parameter
            set_material_parameter(
                material_path="/Game/Materials/MI_Floor",
                parameter_name="DiffuseTexture",
                parameter_type="texture",
                value="/Game/Textures/T_Marble_D"
            )
        """
        return set_material_parameter_impl(ctx, material_path, parameter_name, parameter_type, value)

    @mcp.tool()
    def get_material_parameter(
        ctx: Context,
        material_path: str,
        parameter_name: str,
        parameter_type: str
    ) -> Dict[str, Any]:
        """
        Get the current value of a material parameter.

        Args:
            material_path: Full path to the material or material instance
            parameter_name: Name of the parameter to get
            parameter_type: Type of parameter. Options:
                - "scalar": Float value
                - "vector": RGBA color/vector
                - "texture": Texture asset reference

        Returns:
            Dict containing:
                - success: Whether the parameter was found
                - parameter_name: Name of the parameter
                - parameter_type: Type of the parameter
                - value: Current value:
                    - For "scalar": float
                    - For "vector": [R, G, B, A] list
                    - For "texture": asset path string or None
                - is_overridden: (For instances) Whether this parameter overrides parent
                - message: Status message

        Examples:
            # Get a scalar parameter value
            get_material_parameter(
                material_path="/Game/Materials/MI_Metal",
                parameter_name="Roughness",
                parameter_type="scalar"
            )

            # Get a vector/color parameter value
            get_material_parameter(
                material_path="/Game/Materials/MI_Glow",
                parameter_name="EmissiveColor",
                parameter_type="vector"
            )
        """
        return get_material_parameter_impl(ctx, material_path, parameter_name, parameter_type)

    @mcp.tool()
    def apply_material_to_actor(
        ctx: Context,
        actor_name: str,
        material_path: str,
        slot_index: int = 0,
        component_name: str = None
    ) -> Dict[str, Any]:
        """
        Apply a material to an actor's mesh component.

        Args:
            actor_name: Name of the actor in the level
            material_path: Full path to the material or material instance
            slot_index: Material slot index to apply to (default 0)
            component_name: Optional specific component name. If None, uses the first
                          mesh component found (StaticMeshComponent or SkeletalMeshComponent)

        Returns:
            Dict containing:
                - success: Whether the material was applied
                - actor_name: Name of the actor
                - component_name: Name of the component the material was applied to
                - material_path: Path to the applied material
                - slot_index: The slot index used
                - message: Status message

        Examples:
            # Apply material to the first mesh component, slot 0
            apply_material_to_actor(
                actor_name="Floor_1",
                material_path="/Game/Materials/M_WoodFloor"
            )

            # Apply material to a specific slot
            apply_material_to_actor(
                actor_name="Character_BP",
                material_path="/Game/Materials/MI_Skin",
                slot_index=2
            )

            # Apply to a specific component
            apply_material_to_actor(
                actor_name="Vehicle",
                material_path="/Game/Materials/MI_CarPaint",
                component_name="BodyMesh"
            )
        """
        return apply_material_to_actor_impl(ctx, actor_name, material_path, slot_index, component_name)

    # ========================================================================
    # Material Expression Tools
    # ========================================================================

    @mcp.tool()
    def add_material_expression(
        ctx: Context,
        material_path: str,
        expression_type: str,
        position: List[float] = None,
        properties: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Add a material expression (node) to a material's graph.

        This allows building material logic by adding expression nodes like constants,
        math operations, texture samples, and more.

        Args:
            material_path: Full path to the material (e.g., "/Game/Materials/M_Fire")
            expression_type: Type of expression to add. Supported types:
                Constants:
                - "Constant": Single float value
                - "Constant2Vector": 2D vector (R, G)
                - "Constant3Vector": 3D vector/color (R, G, B)
                - "Constant4Vector": 4D vector/color (R, G, B, A)

                Math:
                - "Add": Add two inputs
                - "Multiply": Multiply two inputs
                - "Divide": Divide A by B
                - "Subtract": Subtract B from A
                - "Power": Raise base to exponent
                - "Abs": Absolute value
                - "Clamp": Clamp value between min/max
                - "Lerp": Linear interpolate between A and B

                Textures:
                - "TextureSample": Sample a texture
                - "TextureSampleParameter2D": Parameterized texture sample

                Parameters:
                - "ScalarParameter": Exposed float parameter
                - "VectorParameter": Exposed vector/color parameter
                - "TextureParameter": Exposed texture parameter

                Utilities:
                - "AppendVector": Combine vectors
                - "ComponentMask": Extract channels (R, G, B, A)
                - "Time": Game time value
                - "Panner": UV panning for scrolling effects
                - "TexCoord": Texture coordinates

            position: Optional [X, Y] editor position for the node
            properties: Optional dict of expression-specific properties. Examples:
                - For Constant3Vector: {"Constant": [1.0, 0.5, 0.0]} (orange color)
                - For ScalarParameter: {"ParameterName": "Intensity", "DefaultValue": 1.0}
                - For TextureSample: {"Texture": "/Game/Textures/T_Fire"}

        Returns:
            Dict containing:
                - success: Whether the expression was added
                - expression_id: GUID string identifying this expression
                - expression_type: Type of expression that was created
                - position: [X, Y] editor position
                - outputs: List of available output pins
                - message: Status message

        Examples:
            # Add an orange constant color
            add_material_expression(
                material_path="/Game/Materials/M_Fire",
                expression_type="Constant3Vector",
                position=[0, 0],
                properties={"Constant": [1.0, 0.5, 0.0]}
            )

            # Add a multiply node
            add_material_expression(
                material_path="/Game/Materials/M_Fire",
                expression_type="Multiply",
                position=[200, 0]
            )

            # Add a time node for animation
            add_material_expression(
                material_path="/Game/Materials/M_Fire",
                expression_type="Time",
                position=[0, 100]
            )
        """
        return add_material_expression_impl(ctx, material_path, expression_type, position, properties)

    @mcp.tool()
    def connect_material_expressions(
        ctx: Context,
        material_path: str,
        source_expression_id: str = "",
        source_output_index: int = 0,
        target_expression_id: str = "",
        target_input_name: str = "",
        connections: list = None
    ) -> Dict[str, Any]:
        """
        Connect two material expressions together.

        Creates a connection from a source expression's output to a target
        expression's input, defining the data flow in the material graph.

        Supports both single connection and batch mode:
        - Single: Provide source_expression_id, source_output_index, target_expression_id, target_input_name
        - Batch: Provide connections array (more efficient - only saves once)

        Args:
            material_path: Full path to the material
            source_expression_id: GUID of the source expression (single mode)
            source_output_index: Index of the output pin on the source (usually 0)
            target_expression_id: GUID of the target expression (single mode)
            target_input_name: Name of the input pin on the target (single mode)
            connections: List of connection dicts for batch mode. Each dict must have:
                - source_expression_id: GUID of source expression
                - source_output_index: Output index (usually 0)
                - target_expression_id: GUID of target expression
                - target_input_name: Name of target input ("A", "B", etc.)

        Returns:
            Dict containing:
                - success: Whether the connection(s) were made
                - results: Array of connection results (batch mode)
                - message: Status message

        Examples:
            # Single connection mode
            connect_material_expressions(
                material_path="/Game/Materials/M_Fire",
                source_expression_id="ABC123...",
                source_output_index=0,
                target_expression_id="DEF456...",
                target_input_name="A"
            )

            # Batch mode - multiple connections, saves once
            connect_material_expressions(
                material_path="/Game/Materials/M_Fire",
                connections=[
                    {"source_expression_id": "ABC123...", "source_output_index": 0,
                     "target_expression_id": "DEF456...", "target_input_name": "A"},
                    {"source_expression_id": "GHI789...", "source_output_index": 0,
                     "target_expression_id": "DEF456...", "target_input_name": "B"}
                ]
            )
        """
        return connect_material_expressions_impl(
            ctx, material_path, source_expression_id, source_output_index,
            target_expression_id, target_input_name, connections
        )

    @mcp.tool()
    def connect_expression_to_material_output(
        ctx: Context,
        material_path: str,
        expression_id: str,
        output_index: int,
        material_property: str
    ) -> Dict[str, Any]:
        """
        Connect an expression to a material output property.

        This is how you wire expressions to the final material outputs like
        BaseColor, Emissive, Roughness, etc.

        Args:
            material_path: Full path to the material
            expression_id: GUID of the expression to connect
            output_index: Index of the output pin on the expression (usually 0)
            material_property: Target material property. Options:
                - "BaseColor": Main surface color
                - "Metallic": Metalness (0=dielectric, 1=metal)
                - "Specular": Specular intensity
                - "Roughness": Surface roughness (0=smooth/shiny, 1=rough)
                - "Normal": Normal map input
                - "EmissiveColor": Self-illumination color/intensity
                - "Opacity": Transparency (for translucent materials)
                - "OpacityMask": Binary transparency (for masked materials)
                - "WorldPositionOffset": Vertex displacement
                - "AmbientOcclusion": Ambient occlusion factor

        Returns:
            Dict containing:
                - success: Whether the connection was made
                - expression_id: Expression that was connected
                - material_property: Property it was connected to
                - message: Status message

        Examples:
            # Connect to EmissiveColor for a glowing fire effect
            connect_expression_to_material_output(
                material_path="/Game/Materials/M_Fire",
                expression_id="ABC123...",
                output_index=0,
                material_property="EmissiveColor"
            )

            # Connect to BaseColor
            connect_expression_to_material_output(
                material_path="/Game/Materials/M_Wood",
                expression_id="DEF456...",
                output_index=0,
                material_property="BaseColor"
            )
        """
        return connect_expression_to_material_output_impl(
            ctx, material_path, expression_id, output_index, material_property
        )

    @mcp.tool()
    def get_material_graph_metadata(
        ctx: Context,
        material_path: str,
        fields: List[str] = None
    ) -> Dict[str, Any]:
        """
        Get metadata about a material's expression graph.

        Returns information about all expressions in the material, their
        connections, and available material outputs.

        Args:
            material_path: Full path to the material
            fields: Optional list of fields to retrieve. Options:
                - "expressions": All expression nodes with types, positions, GUIDs
                - "connections": All connections between expressions
                - "outputs": Material output properties and their connections
                - "*": All fields (default)

        Returns:
            Dict containing:
                - success: Whether the material was found
                - material_path: Path to the material
                - expressions: List of expression info, each with:
                    - expression_id: GUID string
                    - type: Expression class name
                    - position: [X, Y] editor position
                    - properties: Expression-specific values
                    - inputs: Available input pins
                    - outputs: Available output pins
                - connections: List of connections, each with:
                    - source_id: Source expression GUID
                    - source_output: Output index
                    - target_id: Target expression GUID
                    - target_input: Input name
                - outputs: Material output connections
                - message: Status message

        Examples:
            # Get all graph info
            get_material_graph_metadata(material_path="/Game/Materials/M_Fire")

            # Get only expressions
            get_material_graph_metadata(
                material_path="/Game/Materials/M_Fire",
                fields=["expressions"]
            )
        """
        return get_material_graph_metadata_impl(ctx, material_path, fields)

    @mcp.tool()
    def delete_material_expression(
        ctx: Context,
        material_path: str,
        expression_id: str
    ) -> Dict[str, Any]:
        """
        Delete a material expression from the graph.

        Removes the expression and automatically disconnects any connections
        to or from it.

        Args:
            material_path: Full path to the material
            expression_id: GUID of the expression to delete

        Returns:
            Dict containing:
                - success: Whether the expression was deleted
                - message: Status message

        Examples:
            # Delete an expression
            delete_material_expression(
                material_path="/Game/Materials/M_Fire",
                expression_id="ABC123..."
            )
        """
        return delete_material_expression_impl(ctx, material_path, expression_id)

    @mcp.tool()
    def set_material_expression_property(
        ctx: Context,
        material_path: str,
        expression_id: str,
        property_name: str,
        property_value: Any
    ) -> Dict[str, Any]:
        """
        Set a property on an existing material expression.

        Allows modifying expression properties after creation, such as
        changing constant values, parameter names, texture references, etc.

        Args:
            material_path: Full path to the material
            expression_id: GUID of the expression to modify
            property_name: Name of the property to set. Common properties:
                - Constant: "Constant" (for Constant node), "R"/"G"/"B"/"A" (for vector)
                - Constant3Vector: "Constant" (FLinearColor as [R,G,B])
                - Parameters: "ParameterName", "DefaultValue"
                - TextureSample: "Texture" (texture asset path)
                - Panner: "SpeedX", "SpeedY"
            property_value: New value for the property. Type depends on property:
                - Numbers: float or int
                - Vectors: [R, G, B] or [R, G, B, A]
                - Strings: parameter names, asset paths

        Returns:
            Dict containing:
                - success: Whether the property was set
                - property_name: Name of the property
                - message: Status message

        Examples:
            # Change a Constant3Vector color to red
            set_material_expression_property(
                material_path="/Game/Materials/M_Fire",
                expression_id="ABC123...",
                property_name="Constant",
                property_value=[1.0, 0.0, 0.0]
            )

            # Change a parameter name
            set_material_expression_property(
                material_path="/Game/Materials/M_Fire",
                expression_id="DEF456...",
                property_name="ParameterName",
                property_value="FireIntensity"
            )
        """
        return set_material_expression_property_impl(
            ctx, material_path, expression_id, property_name, property_value
        )

    logger.info("Material tools registered successfully")
