"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context
from utils.nodes.node_operations import (
    # add_event_node as add_event_node_impl,  # REMOVED: Use create_node_by_action_name instead
    add_input_action_node as add_input_action_node_impl,
    # add_function_node as add_function_node_impl,  # REMOVED: Use create_node_by_action_name instead
    connect_nodes_impl,
    add_self_reference as add_self_reference_impl,
    find_nodes as find_nodes_impl,
    get_variable_info_impl
)
from utils.unreal_connection_utils import send_unreal_command

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def register_blueprint_node_tools(mcp: FastMCP):
    """Register all blueprint node tools with the MCP server."""
    
    # REMOVED: Event nodes now handled by create_node_by_action_name
    # @mcp.tool()
    # def add_blueprint_event_node(
    #     ctx: Context,
    #     blueprint_name: str,
    #     event_name: str,
    #     node_position: List[float] = None
    # ) -> Dict[str, Any]:
    #     """Add an event node to a Blueprint's event graph."""
    
    @mcp.tool()
    def add_blueprint_input_action_node(
        ctx: Context,
        blueprint_name: str,
        action_name: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add an input action event node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            action_name: Name of the input action to respond to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        try:
            return add_input_action_node_impl(
                ctx, blueprint_name, action_name, node_position
            )
        except Exception as e:
            logger.error(f"Error adding input action node: {e}")
            return {
                "success": False,
                "message": f"Failed to add input action node: {str(e)}"
            }

    # REMOVED: Function nodes now handled by create_node_by_action_name  
    # @mcp.tool()
    # def add_blueprint_function_node(
    #     ctx: Context,
    #     blueprint_name: str,
    #     function_name: str,
    #     function_library: str = "",
    #     node_position: List[float] = None
    # ) -> Dict[str, Any]:
    #     """Add a function call node to a Blueprint's event graph."""
    
    @mcp.tool()
    def connect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str
    ) -> Dict[str, Any]:
        """
        Connect two nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node
            source_pin: Name of the output pin on the source node
            target_node_id: ID of the target node
            target_pin: Name of the input pin on the target node
            
        Returns:
            Response indicating success or failure
        """
        try:
            return connect_nodes_impl(
                ctx, blueprint_name, source_node_id, source_pin, target_node_id, target_pin
            )
        except Exception as e:
            logger.error(f"Error connecting nodes: {e}")
            return {
                "success": False,
                "message": f"Failed to connect nodes: {str(e)}"
            }

    @mcp.tool()
    def add_blueprint_self_reference(
        ctx: Context,
        blueprint_name: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.

        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        try:
            return add_self_reference_impl(
                ctx, blueprint_name, node_position
            )
        except Exception as e:
            logger.error(f"Error adding self reference: {e}")
            return {
                "success": False,
                "message": f"Failed to add self reference: {str(e)}"
            }

    @mcp.tool()
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type: str = None,
        event_type: str = None
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            
        Returns:
            Response containing array of found node IDs and success status
        """
        try:
            return find_nodes_impl(
                ctx, blueprint_name, node_type, event_type
            )
        except Exception as e:
            logger.error(f"Error finding nodes: {e}")
            return {
                "success": False,
                "message": f"Failed to find nodes: {str(e)}"
            }

    @mcp.tool()
    def get_variable_info(
        ctx: Context,
        blueprint_name: str,
        variable_name: str
    ) -> Dict[str, Any]:
        """
        Get the type information for a variable in a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to query
        Returns:
            Dict with at least 'variable_type' (e.g., 'Vector', 'Float', 'MyStruct', etc.)

        Usage for struct/array automation:
            - Use this tool to get the struct type of a variable.
            - Pass the returned struct type as 'struct_type' to create_node_by_action_name with function_name='BreakStruct'.
        """
        try:
            return get_variable_info_impl(
                ctx, blueprint_name, variable_name
            )
        except Exception as e:
            logger.error(f"Error getting variable info: {e}")
            return {
                "success": False,
                "message": f"Failed to get variable info: {str(e)}"
            }