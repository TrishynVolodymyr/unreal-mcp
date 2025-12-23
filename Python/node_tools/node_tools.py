"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any, Optional, Union
from mcp.server.fastmcp import FastMCP, Context
from utils.nodes.node_operations import (
    # add_event_node as add_event_node_impl,  # REMOVED: Use create_node_by_action_name instead
    # add_input_action_node as add_input_action_node_impl,  # REMOVED: Use create_node_by_action_name instead
    # add_function_node as add_function_node_impl,  # REMOVED: Use create_node_by_action_name instead
    connect_nodes_impl,
    # find_nodes as find_nodes_impl,  # REMOVED: Use get_blueprint_metadata with fields=["graph_nodes"] and node_type/event_type filters instead
    get_variable_info_impl
)
from utils.nodes.graph_manipulation import (
    disconnect_node_impl,
    delete_node_impl,
    replace_node_impl
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

    # REMOVED: get_blueprint_graphs - Use get_blueprint_metadata with fields=["graphs"] instead
    # The metadata version provides more info (graph names + node counts)

    # REMOVED: find_blueprint_nodes - Use get_blueprint_metadata with fields=["graph_nodes"] and node_type/event_type filters instead
    # Example: get_blueprint_metadata(blueprint_name="BP_MyActor", fields=["graph_nodes"], node_type="Event", event_type="BeginPlay")

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

    # ========== Graph Manipulation Tools (Low-level) ==========
    
    @mcp.tool()
    def disconnect_node(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        target_graph: str = "EventGraph",
        disconnect_inputs: bool = True,
        disconnect_outputs: bool = True
    ) -> Dict[str, Any]:
        """
        Disconnect all connections from a specific node in a Blueprint graph.
        This is useful for preparing a node for replacement or removal.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_id: ID of the node to disconnect
            target_graph: Name of the graph containing the node (default: "EventGraph")
            disconnect_inputs: Whether to disconnect input pins (default: True)
            disconnect_outputs: Whether to disconnect output pins (default: True)
        
        Returns:
            Dict containing disconnection results including number of disconnections
            
        Examples:
            # Disconnect all connections from a node
            disconnect_node(ctx, blueprint_name="DialogueComponent", node_id="354D34B94684C47CC592A88E87C22772")
            
            # Disconnect only inputs
            disconnect_node(ctx, blueprint_name="BP_MyActor", node_id="some_id", disconnect_outputs=False)
        """
        try:
            return disconnect_node_impl(
                ctx, blueprint_name, node_id, target_graph, disconnect_inputs, disconnect_outputs
            )
        except Exception as e:
            logger.error(f"Error disconnecting node: {e}")
            return {
                "success": False,
                "message": f"Failed to disconnect node: {str(e)}"
            }
    
    @mcp.tool()
    def delete_node(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        target_graph: str = "EventGraph"
    ) -> Dict[str, Any]:
        """
        Delete a node from a Blueprint graph.
        Automatically disconnects all pins before deletion.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_id: ID of the node to delete
            target_graph: Name of the graph containing the node (default: "EventGraph")
        
        Returns:
            Dict containing deletion results
            
        Examples:
            # Delete a node
            delete_node(ctx, blueprint_name="DialogueComponent", node_id="354D34B94684C47CC592A88E87C22772")
        """
        try:
            return delete_node_impl(ctx, blueprint_name, node_id, target_graph)
        except Exception as e:
            logger.error(f"Error deleting node: {e}")
            return {
                "success": False,
                "message": f"Failed to delete node: {str(e)}"
            }
    
    @mcp.tool()
    def replace_node(
        ctx: Context,
        blueprint_name: str,
        old_node_id: str,
        new_node_type: str,
        target_graph: str = "EventGraph",
        new_node_config: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Replace a node in a Blueprint graph.
        Returns stored connection information that can be used to reconnect after creating the new node.
        
        This is a multi-step process:
        1. Stores all connections from the old node
        2. Disconnects and deletes the old node
        3. Returns connection data for manual reconnection after creating new node
        
        Args:
            blueprint_name: Name of the target Blueprint
            old_node_id: ID of the node to replace
            new_node_type: Type of the new node (for reference/logging)
            target_graph: Name of the graph containing the node (default: "EventGraph")
            new_node_config: Optional configuration for the new node
        
        Returns:
            Dict containing:
            - stored_connections: Array of connection info for reconnection
            - old_node_pos_x, old_node_pos_y: Position of the old node
            - success: Boolean indicating success
            
        Examples:
            # Replace Get Owner with Get Owning Actor
            result = replace_node(
                ctx,
                blueprint_name="DialogueComponent",
                old_node_id="354D34B94684C47CC592A88E87C22772",
                new_node_type="GetOwningActor"
            )
            # Then use create_node_by_action_name to create new node
            # And connect_blueprint_nodes to reconnect using stored_connections
        """
        try:
            return replace_node_impl(
                ctx, blueprint_name, old_node_id, new_node_type, target_graph, new_node_config
            )
        except Exception as e:
            logger.error(f"Error replacing node: {e}")
            return {
                "success": False,
                "message": f"Failed to replace node: {str(e)}"
            }
    
    # NOTE: For getting node pin info, use the existing tool from blueprint_action_mcp_server:
    # inspect_node_pin_connection(node_name="...", pin_name="...")
    
    @mcp.tool()
    def set_node_pin_value(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        pin_name: str,
        value: Union[str, int, float, bool],
        target_graph: str = "EventGraph"
    ) -> Dict[str, Any]:
        """
        Set the default value or selection for a pin on a Blueprint node.
        Works for dropdown selectors, literal values, class references, etc.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_id: ID of the node containing the pin
            pin_name: Name of the pin to set (e.g., "ComponentClass", "Class", "Value")
            value: Value to set. Can be:
                - Class path: "/Script/Engine.SplineComponent" or "SplineComponent"
                - Enum value: "ESplineCoordinateSpace::Local" or "Local"
                - Literal value: "1", "1.0", "true", "Hello" (or int/float/bool types)
            target_graph: Name of the graph containing the node (default: "EventGraph")
        
        Returns:
            Dict containing success status and updated pin info
            
        Examples:
            # Set component class on Get Component by Class node
            set_node_pin_value(
                ctx,
                blueprint_name="BP_MyActor",
                node_id="ABC123...",
                pin_name="ComponentClass",
                value="SplineComponent"
            )
            
            # Set coordinate space on spline function
            set_node_pin_value(
                ctx,
                blueprint_name="BP_MyActor",
                node_id="DEF456...",
                pin_name="CoordinateSpace",
                value="Local"
            )
            
            # Set literal integer value
            set_node_pin_value(
                ctx,
                blueprint_name="BP_MyActor",
                node_id="GHI789...",
                pin_name="Value",
                value=42  # Can pass int directly
            )
        """
        try:
            # Convert value to string for C++ command
            value_str = str(value) if not isinstance(value, str) else value
            
            params = {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "pin_name": pin_name,
                "value": value_str,
                "target_graph": target_graph
            }
            
            result = send_unreal_command("set_node_pin_value", params)
            return result
            
        except Exception as e:
            logger.error(f"Error setting node pin value: {e}")
            return {
                "success": False,
                "message": f"Failed to set pin value: {str(e)}"
            }

    # ========== Graph Layout Tools ==========

    @mcp.tool()
    def auto_arrange_nodes(
        ctx: Context,
        blueprint_name: str,
        graph_name: str = "EventGraph"
    ) -> Dict[str, Any]:
        """
        Auto-arrange nodes in a Blueprint graph for better readability.
        Uses connection-aware horizontal flow layout:
        - Event/entry nodes placed on the left
        - Connected nodes flow left-to-right following execution order
        - Pure nodes (getters, math) positioned near their consumers

        Args:
            blueprint_name: Name or path of the Blueprint
            graph_name: Name of the graph to arrange (default: "EventGraph")

        Returns:
            Dict containing:
            - success: Boolean indicating success
            - arranged_count: Number of nodes that were arranged
            - graph_name: Name of the graph that was arranged

        Examples:
            # Arrange EventGraph nodes
            auto_arrange_nodes(ctx, blueprint_name="BP_MyActor")

            # Arrange a specific function graph
            auto_arrange_nodes(ctx, blueprint_name="BP_MyActor", graph_name="UpdateHealth")
        """
        try:
            params = {
                "blueprint_name": blueprint_name,
                "graph_name": graph_name
            }

            result = send_unreal_command("auto_arrange_nodes", params)
            return result

        except Exception as e:
            logger.error(f"Error auto-arranging nodes: {e}")
            return {
                "success": False,
                "message": f"Failed to auto-arrange nodes: {str(e)}"
            }