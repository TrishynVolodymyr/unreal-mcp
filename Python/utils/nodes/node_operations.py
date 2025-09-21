"""
Node Operations for Unreal MCP.

This module provides utilities for working with Blueprint nodes in Unreal Engine.
"""

import logging
import json
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

def get_blueprint_graphs_impl(
    ctx: Context,
    blueprint_name: str
) -> Dict[str, Any]:
    """Implementation for getting all graphs in a Blueprint."""
    params = {
        "blueprint_name": blueprint_name
    }
    
    return send_unreal_command("get_blueprint_graphs", params)

def find_nodes(
    ctx: Context,
    blueprint_name: str,
    node_type: str = None,
    event_type: str = None,
    target_graph: str = None
) -> Dict[str, Any]:
    """Implementation for finding nodes in a Blueprint's event graph."""
    params = {
        "blueprint_name": blueprint_name,
        "node_type": node_type if node_type is not None else "All"
    }
    
    if event_type is not None:
        params["event_name"] = event_type
        
    if target_graph is not None:
        params["target_graph"] = target_graph
        
    return send_unreal_command("find_blueprint_nodes", params)

def connect_nodes_impl(
    ctx: Context,
    blueprint_name: str,
    connections: list
) -> Dict[str, Any]:
    """
    Implementation for connecting nodes in a Blueprint's event graph.
    Only supports batch connections mode.
    Each connection dict must have: source_node_id, source_pin, target_node_id, target_pin.
    """
    if not connections:
        return {
            "success": False,
            "error": "Missing 'connections' parameter - only batch connections are supported"
        }
    
    params = {"blueprint_name": blueprint_name, "connections": connections}
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