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
    """Implementation for inspecting Blueprint node pin connection details and compatibility."""
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
    target_graph: str = "EventGraph",
    **kwargs
) -> Dict[str, Any]:
    """Implementation for creating a blueprint node by discovered action/function name."""
    
    # Flatten if kwargs is double-wrapped (i.e., kwargs={'kwargs': {...}})
    if (
        len(kwargs) == 1 and
        'kwargs' in kwargs and
        isinstance(kwargs['kwargs'], dict)
    ):
        kwargs = kwargs['kwargs']
    
    # --- PATCH START ---
    if function_name in ("Get", "Set") and "variable_name" in kwargs:
        function_name = f"{function_name} {kwargs['variable_name']}"
        kwargs.pop("variable_name")
    # --- PATCH END ---
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name
    }
    
    if class_name:
        params["class_name"] = class_name
        
    if node_position is not None:
        # Convert List[float] to JSON string that C++ can parse
        params["node_position"] = json.dumps(node_position)
    
    # Build json_params payload that will be passed through to the C++ side.  It carries
    # any extra keyword arguments **and** the optional target_graph so the service layer
    # can pick it up in one place.

    extra_params: Dict[str, Any] = {}

    # IMPORTANT: target_graph must be in json_params for C++ to see it
    # Handle both direct parameter and kwargs versions
    final_target_graph = target_graph
    if "target_graph" in kwargs:
        final_target_graph = kwargs.pop("target_graph")
    
    # Always include target_graph (defaults to "EventGraph")
    params["target_graph"] = final_target_graph  # For handler compatibility
    extra_params["target_graph"] = final_target_graph  # For C++ service layer

    # Merge any additional kwargs supplied by the caller
    if kwargs:
        extra_params.update(kwargs)

    # Always provide json_params (even if empty) so the handler code doesn’t hit a
    # missing-field branch and the tool signature stays consistent.
    params["json_params"] = json.dumps(extra_params)
    
    return send_unreal_command("create_node_by_action_name", params) 