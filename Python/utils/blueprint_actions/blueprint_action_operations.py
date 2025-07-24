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


def create_node_by_action_name(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    class_name: str = "",
    node_position: List[float] = None,
    target_graph: str = None,
    **kwargs
) -> Dict[str, Any]:
    """Implementation for creating a blueprint node by discovered action/function name."""
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

    # Promote target_graph into the top-level params (still needed by the handler)
    # **and** put it inside the json_params object so deeper layers can read it.
    if target_graph is not None:
        params["target_graph"] = target_graph
        extra_params["target_graph"] = target_graph

    # Merge any additional kwargs supplied by the caller
    if kwargs:
        extra_params.update(kwargs)

    # Always provide json_params (even if empty) so the handler code doesn’t hit a
    # missing-field branch and the tool signature stays consistent.
    params["json_params"] = json.dumps(extra_params)
    
    return send_unreal_command("create_node_by_action_name", params) 