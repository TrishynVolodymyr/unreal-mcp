"""
Node Operations for Unreal MCP.

This module provides utilities for working with Blueprint nodes in Unreal Engine.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def connect_nodes(
    ctx: Context,
    blueprint_path: str,
    function_name: str,
    source_node_name: str,
    source_pin_name: str,
    target_node_name: str,
    target_pin_name: str
) -> Dict[str, Any]:
    """Implementation for connecting two nodes in a blueprint graph."""
    params = {
        "blueprint_path": blueprint_path,
        "function_name": function_name,
        "source_node_name": source_node_name,
        "source_pin_name": source_pin_name,
        "target_node_name": target_node_name,
        "target_pin_name": target_pin_name
    }
    
    return send_unreal_command("connect_blueprint_nodes", params)

def add_event(
    ctx: Context,
    blueprint_path: str,
    event_type: str,
    event_name: str = None,
    x: float = 0.0,
    y: float = 0.0
) -> Dict[str, Any]:
    """Implementation for adding an event node to a blueprint graph."""
    params = {
        "blueprint_path": blueprint_path,
        "event_type": event_type,
        "x": x,
        "y": y
    }
    
    if event_name is not None:
        params["event_name"] = event_name
    
    return send_unreal_command("add_event", params)

def add_event_node(
    ctx: Context,
    blueprint_name: str,
    event_name: str,
    node_position: List[float] = None
) -> Dict[str, Any]:
    """Implementation for adding an event node to a Blueprint's event graph."""
    params = {
        "blueprint_name": blueprint_name,
        "event_name": event_name
    }
    
    if node_position is not None:
        params["node_position"] = node_position
        
    return send_unreal_command("add_blueprint_event_node", params)

def add_input_action_node(
    ctx: Context,
    blueprint_name: str,
    action_name: str,
    node_position: List[float] = None
) -> Dict[str, Any]:
    """Implementation for adding an Enhanced Input action event node to a Blueprint's event graph."""
    # For Enhanced Input Actions, we need to provide the full path to the asset
    # If action_name doesn't start with /Game/, assume it's just the name and construct the path
    if not action_name.startswith("/Game/"):
        # Try common Enhanced Input Action paths
        action_path = f"/Game/Input/Actions/{action_name}"
    else:
        action_path = action_name
    
    params = {
        "blueprint_name": blueprint_name,
        "action_path": action_path
    }
    
    if node_position is not None:
        params["node_position"] = node_position
        
    return send_unreal_command("add_enhanced_input_action_node", params)

def add_function_node(
    ctx: Context,
    blueprint_name: str,
    target: str,
    function_name: str,
    params: Dict[str, Any] = None,
    node_position: List[float] = None
) -> Dict[str, Any]:
    """Implementation for adding a function call node to a Blueprint's event graph."""
    command_params = {
        "blueprint_name": blueprint_name,
        "target": target,
        "function_name": function_name
    }
    
    if params is not None:
        command_params["params"] = params
        
    if node_position is not None:
        command_params["node_position"] = node_position
        
    return send_unreal_command("add_blueprint_function_node", command_params)

def add_self_reference(
    ctx: Context,
    blueprint_name: str,
    node_position: List[float] = None
) -> Dict[str, Any]:
    """Implementation for adding a 'Get Self' node to a Blueprint's event graph."""
    params = {
        "blueprint_name": blueprint_name
    }
    
    if node_position is not None:
        params["node_position"] = node_position
        
    return send_unreal_command("add_blueprint_self_reference", params)

def find_nodes(
    ctx: Context,
    blueprint_name: str,
    node_type: str = None,
    event_type: str = None
) -> Dict[str, Any]:
    """Implementation for finding nodes in a Blueprint's event graph."""
    params = {
        "blueprint_name": blueprint_name,
        "node_type": node_type if node_type is not None else "All"
    }
    
    if event_type is not None:
        params["event_name"] = event_type
        
    return send_unreal_command("find_blueprint_nodes", params)

def connect_nodes_impl(
    ctx: Context,
    blueprint_name: str,
    source_node_id: str,
    source_pin: str,
    target_node_id: str,
    target_pin: str
) -> Dict[str, Any]:
    """Implementation for connecting two nodes in a Blueprint's event graph."""
    params = {
        "blueprint_name": blueprint_name,
        "source_node_id": source_node_id,
        "source_pin": source_pin,
        "target_node_id": target_node_id,
        "target_pin": target_pin
    }
    
    return send_unreal_command("connect_blueprint_nodes", params)

def get_variable_info_impl(
    ctx: Context,
    blueprint_name: str,
    variable_name: str
) -> Dict[str, Any]:
    """Implementation for getting variable type information from a Blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "variable_name": variable_name
    }
    
    return send_unreal_command("get_variable_info", params)