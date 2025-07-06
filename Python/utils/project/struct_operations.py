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


def show_struct_variables(
    ctx: Context,
    struct_name: str,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    Show variables and types of a struct in Unreal Engine.
    Args:
        ctx: The MCP context
        struct_name: Name of the struct to inspect
        path: Path where the struct exists (default: /Game/Blueprints)
    Returns:
        Dictionary with struct variable info
    """
    params = {
        "struct_name": struct_name,
        "path": path
    }
    logger.info(f"Showing struct variables for: {struct_name} at {path}")
    return send_unreal_command("show_struct_variables", params)


def list_folder_contents(
    ctx: Context,
    folder_path: str
) -> Dict[str, Any]:
    """
    List the contents of a folder in the Unreal project (content or regular folder).
    Args:
        ctx: The MCP context
        folder_path: Path to the folder (e.g., "/Game/Blueprints" or "Content/MyFolder" or "Intermediate/MyTools")
    Returns:
        Dictionary with arrays of subfolders and files/assets
    """
    params = {
        "folder_path": folder_path
    }
    logger.info(f"Listing folder contents for: {folder_path}")
    return send_unreal_command("list_folder_contents", params)