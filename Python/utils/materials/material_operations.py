"""
Material Operations for Unreal MCP.

This module provides utilities for working with Unreal Engine materials.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")


def create_material(
    ctx: Context,
    name: str,
    path: str = "/Game/Materials",
    blend_mode: str = "Opaque",
    shading_model: str = "DefaultLit"
) -> Dict[str, Any]:
    """Implementation for creating a new Material asset."""
    params = {
        "name": name,
        "path": path,
        "blend_mode": blend_mode,
        "shading_model": shading_model
    }

    logger.info(f"Creating material '{name}' at '{path}' with blend_mode='{blend_mode}', shading_model='{shading_model}'")
    return send_unreal_command("create_material", params)


def create_material_instance(
    ctx: Context,
    name: str,
    parent_material: str,
    path: str = "/Game/Materials",
    is_dynamic: bool = False
) -> Dict[str, Any]:
    """Implementation for creating a Material Instance."""
    params = {
        "name": name,
        "parent_material": parent_material,
        "path": path,
        "is_dynamic": is_dynamic
    }

    instance_type = "dynamic" if is_dynamic else "static"
    logger.info(f"Creating {instance_type} material instance '{name}' from parent '{parent_material}'")
    return send_unreal_command("create_material_instance", params)


def get_material_metadata(
    ctx: Context,
    material_path: str,
    fields: List[str] = None
) -> Dict[str, Any]:
    """Implementation for getting material metadata."""
    params = {
        "material_path": material_path
    }

    if fields is not None:
        params["fields"] = fields

    logger.info(f"Getting metadata for material '{material_path}' with fields: {fields}")
    return send_unreal_command("get_material_metadata", params)


def set_material_parameter(
    ctx: Context,
    material_path: str,
    parameter_name: str,
    parameter_type: str,
    value
) -> Dict[str, Any]:
    """Implementation for setting a material parameter."""
    params = {
        "material_path": material_path,
        "parameter_name": parameter_name,
        "parameter_type": parameter_type,
        "value": value
    }

    logger.info(f"Setting {parameter_type} parameter '{parameter_name}' on '{material_path}' to {value}")
    return send_unreal_command("set_material_parameter", params)


def get_material_parameter(
    ctx: Context,
    material_path: str,
    parameter_name: str,
    parameter_type: str
) -> Dict[str, Any]:
    """Implementation for getting a material parameter value."""
    params = {
        "material_path": material_path,
        "parameter_name": parameter_name,
        "parameter_type": parameter_type
    }

    logger.info(f"Getting {parameter_type} parameter '{parameter_name}' from '{material_path}'")
    return send_unreal_command("get_material_parameter", params)


def apply_material_to_actor(
    ctx: Context,
    actor_name: str,
    material_path: str,
    slot_index: int = 0,
    component_name: str = None
) -> Dict[str, Any]:
    """Implementation for applying a material to an actor."""
    params = {
        "actor_name": actor_name,
        "material_path": material_path,
        "slot_index": slot_index
    }

    if component_name is not None:
        params["component_name"] = component_name

    logger.info(f"Applying material '{material_path}' to actor '{actor_name}' slot {slot_index}")
    return send_unreal_command("apply_material_to_actor", params)


# ============================================================================
# Material Expression Operations
# ============================================================================

def add_material_expression(
    ctx: Context,
    material_path: str,
    expression_type: str,
    position: List[float] = None,
    properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """Implementation for adding a material expression to a material's graph."""
    params = {
        "material_path": material_path,
        "expression_type": expression_type
    }

    if position is not None:
        params["position"] = position

    if properties is not None:
        params["properties"] = properties

    logger.info(f"Adding expression '{expression_type}' to material '{material_path}'")
    return send_unreal_command("add_material_expression", params)


def connect_material_expressions(
    ctx: Context,
    material_path: str,
    source_expression_id: str = "",
    source_output_index: int = 0,
    target_expression_id: str = "",
    target_input_name: str = "",
    connections: list = None
) -> Dict[str, Any]:
    """Implementation for connecting material expressions (single or batch mode)."""
    params = {"material_path": material_path}

    if connections:
        # Batch mode - pass connections array
        params["connections"] = connections
        logger.info(f"Connecting {len(connections)} material expressions in batch mode")
    else:
        # Single connection mode - backward compatible
        params["source_expression_id"] = source_expression_id
        params["source_output_index"] = source_output_index
        params["target_expression_id"] = target_expression_id
        params["target_input_name"] = target_input_name
        logger.info(f"Connecting expression {source_expression_id}[{source_output_index}] -> {target_expression_id}.{target_input_name}")

    return send_unreal_command("connect_material_expressions", params)


def connect_expression_to_material_output(
    ctx: Context,
    material_path: str,
    expression_id: str,
    output_index: int,
    material_property: str
) -> Dict[str, Any]:
    """Implementation for connecting an expression to a material output property."""
    params = {
        "material_path": material_path,
        "expression_id": expression_id,
        "output_index": output_index,
        "material_property": material_property
    }

    logger.info(f"Connecting expression {expression_id}[{output_index}] to material output '{material_property}'")
    return send_unreal_command("connect_expression_to_material_output", params)


def get_material_graph_metadata(
    ctx: Context,
    material_path: str,
    fields: List[str] = None
) -> Dict[str, Any]:
    """Implementation for getting material graph/expression metadata."""
    params = {
        "material_path": material_path
    }

    if fields is not None:
        params["fields"] = fields

    logger.info(f"Getting graph metadata for material '{material_path}' with fields: {fields}")
    return send_unreal_command("get_material_expression_metadata", params)


def delete_material_expression(
    ctx: Context,
    material_path: str,
    expression_id: str
) -> Dict[str, Any]:
    """Implementation for deleting a material expression."""
    params = {
        "material_path": material_path,
        "expression_id": expression_id
    }

    logger.info(f"Deleting expression {expression_id} from material '{material_path}'")
    return send_unreal_command("delete_material_expression", params)


def set_material_expression_property(
    ctx: Context,
    material_path: str,
    expression_id: str,
    property_name: str,
    property_value: Any
) -> Dict[str, Any]:
    """Implementation for setting a property on a material expression."""
    params = {
        "material_path": material_path,
        "expression_id": expression_id,
        "property_name": property_name,
        "property_value": property_value
    }

    logger.info(f"Setting property '{property_name}' on expression {expression_id} to {property_value}")
    return send_unreal_command("set_material_expression_property", params)
