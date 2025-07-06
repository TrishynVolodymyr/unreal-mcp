"""
Blueprint Action Operations for Unreal MCP.

This module provides utility functions for discovering Blueprint actions using the FBlueprintActionDatabase.
"""

import logging
import json
from typing import Dict, Any, List
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def get_actions_for_pin(
    ctx: Context,
    pin_type: str,
    pin_subcategory: str = "",
    search_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """Implementation for getting actions for a specific pin type with search filtering."""
    params = {
        "pin_type": pin_type,
        "pin_subcategory": pin_subcategory,
        "search_filter": search_filter,
        "max_results": max_results
    }
    return send_unreal_command("get_actions_for_pin", params)

def get_actions_for_class(
    ctx: Context,
    class_name: str,
    search_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """Implementation for getting actions for a specific class with search filtering."""
    params = {
        "class_name": class_name,
        "search_filter": search_filter,
        "max_results": max_results
    }
    return send_unreal_command("get_actions_for_class", params)

def get_actions_for_class_hierarchy(
    ctx: Context,
    class_name: str,
    search_filter: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """Implementation for getting actions for a class hierarchy with search filtering."""
    params = {
        "class_name": class_name,
        "search_filter": search_filter,
        "max_results": max_results
    }
    return send_unreal_command("get_actions_for_class_hierarchy", params)

def search_blueprint_actions(
    ctx: Context,
    search_query: str,
    category: str = "",
    max_results: int = 50,
    blueprint_name: str = None
) -> Dict[str, Any]:
    """Implementation for searching Blueprint actions using keywords."""
    params = {
        "search_query": search_query,
        "category": category,
        "max_results": max_results
    }
    if blueprint_name:
        params["blueprint_name"] = blueprint_name
    return send_unreal_command("search_blueprint_actions", params)

def get_node_pin_info(
    ctx: Context,
    node_name: str,
    pin_name: str
) -> Dict[str, Any]:
    """Implementation for getting specific node pin information."""
    params = {
        "node_name": node_name,
        "pin_name": pin_name
    }
    return send_unreal_command("get_node_pin_info", params)

def create_node_by_action_name(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    class_name: str = "",
    node_position: List[float] = None,
    **kwargs
) -> Dict[str, Any]:
    """Implementation for creating a blueprint node by discovered action/function name."""
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name
    }
    
    if class_name:
        params["class_name"] = class_name
        
    if node_position is not None:
        # Convert List[float] to JSON string that C++ can parse
        params["node_position"] = json.dumps(node_position)
    
    if kwargs:
        # Convert kwargs to JSON string for C++ side
        params["json_params"] = json.dumps(kwargs)
    
    return send_unreal_command("create_node_by_action_name", params) 