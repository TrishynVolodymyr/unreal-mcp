"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context
from utils.nodes.node_operations import (
    # add_event_node as add_event_node_impl,  # REMOVED: Use create_node_by_action_name instead
    # add_input_action_node as add_input_action_node_impl,  # REMOVED: Use create_node_by_action_name instead
    # add_function_node as add_function_node_impl,  # REMOVED: Use create_node_by_action_name instead
    connect_nodes_impl,
    get_blueprint_graphs_impl,
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
        connections: list,
        target_graph: str = "EventGraph"
    ) -> Dict[str, Any]:
        """
        Connect nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            connections: List of connection dicts (batch mode). Each dict must have:
                - source_node_id
                - source_pin
                - target_node_id
                - target_pin
            target_graph: Optional name of the specific graph to connect nodes in (e.g., "EventGraph", "CanInteract")

        Returns:
            Response indicating success or failure. Returns an array of results.

        Examples:
            # Batch connection (only supported mode)
            connect_blueprint_nodes(ctx, blueprint_name="BP_MyActor", connections=[
                {"source_node_id": "id1", "source_pin": "Exec", "target_node_id": "id2", "target_pin": "Then"},
                {"source_node_id": "id3", "source_pin": "Out", "target_node_id": "id4", "target_pin": "In"}
            ])
        """
        try:
            return connect_nodes_impl(
                ctx, blueprint_name,
                connections=connections,
                target_graph=target_graph
            )
        except Exception as e:
            logger.error(f"Error connecting nodes: {e}")
            return {
                "success": False,
                "message": f"Failed to connect nodes: {str(e)}"
            }

    @mcp.tool()
    def get_blueprint_graphs(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Get list of all available graphs in a Blueprint.
        
        Args:
            blueprint_name: Name of the target Blueprint
            
        Returns:
            Response containing array of graph names and success status
        """
        try:
            return get_blueprint_graphs_impl(ctx, blueprint_name)
        except Exception as e:
            logger.error(f"Error getting Blueprint graphs: {e}")
            return {
                "success": False,
                "message": f"Failed to get Blueprint graphs: {str(e)}"
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