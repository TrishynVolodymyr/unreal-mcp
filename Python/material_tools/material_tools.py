"""Material Tools for Unreal MCP."""

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
    add_material_expression as add_material_expression_impl,
    connect_material_expressions as connect_material_expressions_impl,
    connect_expression_to_material_output as connect_expression_to_material_output_impl,
    get_material_graph_metadata as get_material_graph_metadata_impl,
    delete_material_expression as delete_material_expression_impl,
    set_material_expression_property as set_material_expression_property_impl,
    compile_material as compile_material_impl
)

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
            name: Material name (e.g., "M_MyMaterial")
            path: Content path for the material
            blend_mode: "Opaque"|"Masked"|"Translucent"|"Additive"|"Modulate"
            shading_model: "DefaultLit"|"Unlit"|"Subsurface"|"ClearCoat"|"TwoSidedFoliage"
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

        Args:
            name: Instance name (e.g., "MI_WoodFloor_Dark")
            parent_material: Path to parent material
            path: Content path for the instance
            is_dynamic: True for runtime-modifiable MaterialInstanceDynamic
        """
        return create_material_instance_impl(ctx, name, parent_material, path, is_dynamic)

    @mcp.tool()
    def get_material_metadata(
        ctx: Context,
        material_path: str,
        fields: List[str] = None
    ) -> Dict[str, Any]:
        """
        Get metadata about a material including parameters.

        Args:
            material_path: Full path to the material
            fields: Filter fields: "basic"|"parameters"|"properties"|"*" (default all)

        Returns:
            Dict with: name, path, type, parent_material, scalar_parameters[],
            vector_parameters[], texture_parameters[], blend_mode, shading_model

        Example:
            get_material_metadata("/Game/Materials/M_WoodFloor")
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

        Args:
            material_path: Path to material instance
            parameter_name: Parameter name (must exist on parent)
            parameter_type: "scalar"|"vector"|"texture"
            value: float for scalar, [R,G,B,A] for vector, path string for texture
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
        Get current value of a material parameter.

        Args:
            material_path: Path to material or instance
            parameter_name: Parameter name
            parameter_type: "scalar"|"vector"|"texture"

        Returns:
            Dict with: parameter_name, parameter_type, value, is_overridden

        Example:
            get_material_parameter("/Game/Materials/MI_Metal", "Roughness", "scalar")
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
            actor_name: Actor name in level
            material_path: Path to material
            slot_index: Material slot index (default 0)
            component_name: Specific component name (default: first mesh)
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
        Add a material expression node to a material's graph.

        Args:
            material_path: Path to material
            expression_type: Node type - Constants: "Constant"|"Constant2Vector"|"Constant3Vector"|"Constant4Vector",
                Math: "Add"|"Multiply"|"Divide"|"Subtract"|"Power"|"Abs"|"Clamp"|"Lerp",
                Textures: "TextureSample"|"TextureSampleParameter2D",
                Parameters: "ScalarParameter"|"VectorParameter"|"TextureParameter",
                Utilities: "AppendVector"|"ComponentMask"|"Time"|"Panner"|"TexCoord"
            position: [X, Y] editor position
            properties: Expression properties (e.g., {"Constant": [1.0, 0.5, 0.0]} for Constant3Vector)
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
        Connect material expressions. Supports single or batch mode.

        Args:
            material_path: Path to material
            source_expression_id: Source expression GUID (single mode)
            source_output_index: Output pin index (usually 0)
            target_expression_id: Target expression GUID (single mode)
            target_input_name: Target input name ("A", "B", etc.)
            connections: Batch mode - list of {source_expression_id, source_output_index, target_expression_id, target_input_name}
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

        Args:
            material_path: Path to material
            expression_id: Expression GUID to connect
            output_index: Output pin index (usually 0)
            material_property: "BaseColor"|"Metallic"|"Specular"|"Roughness"|"Normal"|"EmissiveColor"|"Opacity"|"OpacityMask"|"WorldPositionOffset"|"AmbientOcclusion"
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

        Args:
            material_path: Path to material
            fields: Filter fields to include:
                - "expressions": All expression nodes
                - "connections": All connections between nodes
                - "material_outputs": Which nodes connect to material outputs
                - "orphans": Nodes whose outputs are not connected (dead-end nodes)
                - "flow": Full path trace from each material output back to source nodes
                - "*": All fields except flow (default)

        Returns:
            Dict with:
            - expressions[] (id, type, position, inputs, outputs)
            - connections[] (source_id, source_output, target_id, target_input)
            - material_outputs{} (which expressions connect to which outputs)
            - orphans[] (expression_id, expression_type, description) - nodes not used
            - has_orphans (bool), orphan_count (int)
            - flow{} (per-output path showing all nodes in the chain)

        Example:
            get_material_graph_metadata("/Game/Materials/M_Fire", ["orphans"])
            get_material_graph_metadata("/Game/Materials/M_Fire", ["flow"])
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

        Args:
            material_path: Path to material
            expression_id: Expression GUID to delete
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

        Args:
            material_path: Path to material
            expression_id: Expression GUID to modify
            property_name: Property name ("Constant", "ParameterName", "DefaultValue", "Texture", "SpeedX", etc.)
            property_value: New value (float, [R,G,B], string depending on property)
        """
        return set_material_expression_property_impl(
            ctx, material_path, expression_id, property_name, property_value
        )

    @mcp.tool()
    def compile_material(
        ctx: Context,
        material_path: str
    ) -> Dict[str, Any]:
        """
        Compile/apply a material and return validation info including orphan detection.

        This triggers shader compilation and returns information about the material graph,
        including any orphan nodes (expressions whose outputs are not connected to anything).

        Args:
            material_path: Path to material

        Returns:
            Dict with: success, material_path, has_orphans, orphan_count, expression_count,
            orphans[] (expression_id, expression_type, description), message

        Example:
            compile_material("/Game/VFX/Materials/M_Fire")
        """
        return compile_material_impl(ctx, material_path)

    logger.info("Material tools registered successfully")
