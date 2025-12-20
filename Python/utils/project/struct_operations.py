"""
Struct operations for Unreal MCP.

This module provides utility functions for managing Unreal Engine struct operations.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")


def create_struct(
    ctx: Context,
    struct_name: str,
    properties: List[Dict[str, str]],
    path: str = "/Game/Blueprints",
    description: str = ""
) -> Dict[str, Any]:
    """
    Create a new Unreal struct.
    
    Args:
        ctx: The MCP context
        struct_name: Name of the struct to create
        properties: List of property dictionaries, each containing:
                    - name: Property name
                    - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
                    - description: (Optional) Property description
        path: Path where to create the struct
        description: Optional description for the struct
        
    Returns:
        Dictionary with the creation status and struct path
    """
    params = {
        "struct_name": struct_name,
        "properties": properties,
        "path": path,
        "description": description
    }
    
    logger.info(f"Creating struct: {struct_name} at {path}")
    return send_unreal_command("create_struct", params)


def update_struct(
    ctx: Context,
    struct_name: str,
    properties: List[Dict[str, str]],
    path: str = "/Game/Blueprints",
    description: str = ""
) -> Dict[str, Any]:
    """
    Update an existing Unreal struct.
    Args:
        ctx: The MCP context
        struct_name: Name of the struct to update
        properties: List of property dictionaries, each containing:
                    - name: Property name
                    - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
                    - description: (Optional) Property description
        path: Path where the struct exists
        description: Optional description for the struct
    Returns:
        Dictionary with the update status and struct path
    """
    params = {
        "struct_name": struct_name,
        "properties": properties,
        "path": path,
        "description": description
    }
    logger.info(f"Updating struct: {struct_name} at {path}")
    return send_unreal_command("update_struct", params)


def get_project_metadata(
    ctx: Context,
    fields: List[str] = None,
    path: str = "/Game",
    folder_path: str = None,
    struct_name: str = None
) -> Dict[str, Any]:
    """
    Get project metadata with selective field querying.

    Consolidates: list_input_actions, list_input_mapping_contexts, show_struct_variables, list_folder_contents

    Args:
        ctx: The MCP context
        fields: List of fields to include. Options:
            - "input_actions": Enhanced Input Action assets
            - "input_contexts": Input Mapping Context assets
            - "structs": Struct variables (requires struct_name)
            - "folder_contents": Folder contents (requires folder_path)
            - "*": All fields (default if None)
        path: Base path for input action/context search (default: /Game)
        folder_path: Path for folder_contents field
        struct_name: Struct name for structs field

    Returns:
        Dictionary with requested project metadata
    """
    params = {
        "path": path
    }

    if fields:
        params["fields"] = fields

    if folder_path:
        params["folder_path"] = folder_path

    if struct_name:
        params["struct_name"] = struct_name

    logger.info(f"Getting project metadata with fields: {fields}")
    return send_unreal_command("get_project_metadata", params)