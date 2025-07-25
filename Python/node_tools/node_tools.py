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
        source_node_id: str = None,
        source_pin: str = None,
        target_node_id: str = None,
        target_pin: str = None,
        connections: list = None
    ) -> Dict[str, Any]:
        """
        Connect nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node (legacy, single connection)
            source_pin: Name of the output pin on the source node (legacy, single connection)
            target_node_id: ID of the target node (legacy, single connection)
            target_pin: Name of the input pin on the target node (legacy, single connection)
            connections: List of connection dicts (batch mode). Each dict must have:
                - source_node_id
                - source_pin
                - target_node_id
                - target_pin

        Returns:
            Response indicating success or failure. In batch mode, returns an array of results.

        Examples:
            # Single connection (legacy)
            connect_blueprint_nodes(ctx, blueprint_name="BP_MyActor", source_node_id="...", source_pin="Exec", target_node_id="...", target_pin="Then")

            # Batch connection
            connect_blueprint_nodes(ctx, blueprint_name="BP_MyActor", connections=[
                {"source_node_id": "id1", "source_pin": "Exec", "target_node_id": "id2", "target_pin": "Then"},
                {"source_node_id": "id3", "source_pin": "Out", "target_node_id": "id4", "target_pin": "In"}
            ])
        """
        try:
            return connect_nodes_impl(
                ctx, blueprint_name,
                source_node_id=source_node_id,
                source_pin=source_pin,
                target_node_id=target_node_id,
                target_pin=target_pin,
                connections=connections
            )
        except Exception as e:
            logger.error(f"Error connecting nodes: {e}")
            return {
                "success": False,
                "message": f"Failed to connect nodes: {str(e)}"
            }

    @mcp.tool()
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type: str = None,
        event_type: str = None,
        target_graph: str = None
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            target_graph: Optional specific graph to search in (e.g. "EventGraph", "UpdateDialogueText")
            
        Returns:
            Response containing array of found node IDs and success status
        """
        try:
            return find_nodes_impl(
                ctx, blueprint_name, node_type, event_type, target_graph
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