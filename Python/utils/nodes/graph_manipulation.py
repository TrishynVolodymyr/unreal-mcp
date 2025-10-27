"""
Utility functions for graph manipulation operations.
Low-level node manipulation in Blueprint graphs.
"""

import json
import logging
from typing import Dict, Any, List, Optional
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

logger = logging.getLogger(__name__)


def disconnect_node_impl(
    ctx: Context,
    blueprint_name: str,
    node_id: str,
    target_graph: str = "EventGraph",
    disconnect_inputs: bool = True,
    disconnect_outputs: bool = True
) -> Dict[str, Any]:
    """
    Disconnect all connections from a specific node.
    
    Args:
        ctx: MCP context
        blueprint_name: Name of the target Blueprint
        node_id: ID of the node to disconnect
        target_graph: Name of the graph containing the node
        disconnect_inputs: Whether to disconnect input pins
        disconnect_outputs: Whether to disconnect output pins
    
    Returns:
        Dict containing disconnection results
    """
    params = {
        "blueprint_name": blueprint_name,
        "node_id": node_id,
        "target_graph": target_graph,
        "disconnect_inputs": disconnect_inputs,
        "disconnect_outputs": disconnect_outputs
    }
    
    return send_unreal_command("disconnect_node", params)


def delete_node_impl(
    ctx: Context,
    blueprint_name: str,
    node_id: str,
    target_graph: str = "EventGraph"
) -> Dict[str, Any]:
    """
    Delete a node from a Blueprint graph.
    
    Args:
        ctx: MCP context
        blueprint_name: Name of the target Blueprint
        node_id: ID of the node to delete
        target_graph: Name of the graph containing the node
    
    Returns:
        Dict containing deletion results
    """
    params = {
        "blueprint_name": blueprint_name,
        "node_id": node_id,
        "target_graph": target_graph
    }
    
    return send_unreal_command("delete_node", params)


def replace_node_impl(
    ctx: Context,
    blueprint_name: str,
    old_node_id: str,
    new_node_type: str,
    target_graph: str = "EventGraph",
    new_node_config: Optional[Dict[str, Any]] = None
) -> Dict[str, Any]:
    """
    Replace a node in a Blueprint graph.
    Returns stored connection information for reconnection after creating new node.
    
    Args:
        ctx: MCP context
        blueprint_name: Name of the target Blueprint
        old_node_id: ID of the node to replace
        new_node_type: Type of the new node to create
        target_graph: Name of the graph containing the node
        new_node_config: Optional configuration for the new node
    
    Returns:
        Dict containing stored connections and replacement results
    """
    params = {
        "blueprint_name": blueprint_name,
        "old_node_id": old_node_id,
        "new_node_type": new_node_type,
        "target_graph": target_graph
    }
    
    if new_node_config:
        params["new_node_config"] = new_node_config
    
    return send_unreal_command("replace_node", params)
