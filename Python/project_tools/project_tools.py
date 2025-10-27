"""
Project Tools for Unreal MCP.

This module provides tools for managing project-wide settings and configuration.
"""

import logging
from typing import Dict, Any, List
from mcp.server.fastmcp import FastMCP, Context
from utils.project.struct_operations import create_struct as create_struct_impl
from utils.project.struct_operations import update_struct as update_struct_impl
from utils.project.struct_operations import show_struct_variables as show_struct_variables_impl
from utils.project.struct_operations import list_folder_contents as list_folder_contents_impl

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_project_tools(mcp: FastMCP):
    """Register project tools with the MCP server."""
    
    @mcp.tool()
    def create_input_mapping(
        ctx: Context,
        action_name: str,
        key: str,
        input_type: str = "Action"
    ) -> Dict[str, Any]:
        """
        Create an input mapping for the project.
        
        Args:
            action_name: Name of the input action
            key: Key to bind (SpaceBar, LeftMouseButton, etc.)
            input_type: Type of input mapping (Action or Axis)
            
        Returns:
            Response indicating success or failure
        
        Example:
            create_input_mapping(action_name="Jump", key="SpaceBar")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "action_name": action_name,
                "key": key,
                "input_type": input_type
            }
            
            logger.info(f"Creating input mapping '{action_name}' with key '{key}'")
            response = unreal.send_command("create_input_mapping", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input mapping creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating input mapping: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_enhanced_input_action(
        ctx: Context,
        action_name: str,
        path: str = "/Game/Input/Actions",
        description: str = "",
        value_type: str = "Digital"
    ) -> Dict[str, Any]:
        """
        Create an Enhanced Input Action asset.
        
        Args:
            action_name: Name of the input action (will add IA_ prefix if not present)
            path: Path where to create the action asset
            description: Optional description for the action
            value_type: Type of input action ("Digital", "Analog", "Axis2D", "Axis3D")
            
        Returns:
            Response indicating success or failure with asset details
        
        Example:
            create_enhanced_input_action(action_name="Jump", value_type="Digital")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "action_name": action_name,
                "path": path,
                "description": description,
                "value_type": value_type
            }
            
            logger.info(f"Creating Enhanced Input Action '{action_name}' with value type '{value_type}'")
            response = unreal.send_command("create_enhanced_input_action", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Enhanced Input Action creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating Enhanced Input Action: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_input_mapping_context(
        ctx: Context,
        context_name: str,
        path: str = "/Game/Input",
        description: str = ""
    ) -> Dict[str, Any]:
        """
        Create an Input Mapping Context asset.
        
        Args:
            context_name: Name of the mapping context (will add IMC_ prefix if not present)
            path: Path where to create the context asset
            description: Optional description for the context
            
        Returns:
            Response indicating success or failure with asset details
        
        Example:
            create_input_mapping_context(context_name="Default")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "context_name": context_name,
                "path": path,
                "description": description
            }
            
            logger.info(f"Creating Input Mapping Context '{context_name}'")
            response = unreal.send_command("create_input_mapping_context", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input Mapping Context creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating Input Mapping Context: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_mapping_to_context(
        ctx: Context,
        context_path: str,
        action_path: str,
        key: str,
        shift: bool = False,
        ctrl: bool = False,
        alt: bool = False,
        cmd: bool = False
    ) -> Dict[str, Any]:
        """
        Add a key mapping to an Input Mapping Context.
        
        Args:
            context_path: Full path to the Input Mapping Context asset
            action_path: Full path to the Input Action asset
            key: Key to bind (SpaceBar, LeftMouseButton, etc.)
            shift: Whether Shift modifier is required
            ctrl: Whether Ctrl modifier is required
            alt: Whether Alt modifier is required
            cmd: Whether Cmd modifier is required
            
        Returns:
            Response indicating success or failure
        
        Example:
            add_mapping_to_context(
                context_path="/Game/Input/IMC_Default",
                action_path="/Game/Input/Actions/IA_Jump",
                key="SpaceBar"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "context_path": context_path,
                "action_path": action_path,
                "key": key,
                "shift": shift,
                "ctrl": ctrl,
                "alt": alt,
                "cmd": cmd
            }
            
            logger.info(f"Adding mapping for '{key}' to context '{context_path}' -> action '{action_path}'")
            response = unreal.send_command("add_mapping_to_context", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add mapping to context response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding mapping to context: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def list_input_actions(
        ctx: Context,
        path: str = "/Game"
    ) -> Dict[str, Any]:
        """
        List all Enhanced Input Action assets in the project.
        
        Args:
            path: Path to search for Input Actions (searches recursively)
            
        Returns:
            Response with list of Input Action assets and their details
        
        Example:
            list_input_actions(path="/Game/Input")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "path": path
            }
            
            logger.info(f"Listing Input Actions in path '{path}'")
            response = unreal.send_command("list_input_actions", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"List Input Actions response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error listing Input Actions: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def list_input_mapping_contexts(
        ctx: Context,
        path: str = "/Game"
    ) -> Dict[str, Any]:
        """
        List all Input Mapping Context assets in the project.
        
        Args:
            path: Path to search for Input Mapping Contexts (searches recursively)
            
        Returns:
            Response with list of Input Mapping Context assets and their details
        
        Example:
            list_input_mapping_contexts(path="/Game/Input")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "path": path
            }
            
            logger.info(f"Listing Input Mapping Contexts in path '{path}'")
            response = unreal.send_command("list_input_mapping_contexts", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"List Input Mapping Contexts response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error listing Input Mapping Contexts: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_folder(
        ctx: Context,
        folder_path: str
    ) -> Dict[str, Any]:
        """
        Create a new folder in the Unreal project.

        This tool can create both content folders (which will appear in the content browser)
        and regular project folders.

        Args:
            folder_path: Path to create, relative to project root.
                       Use "Content/..." prefix for content browser folders.

        Returns:
            Dictionary with the creation status and folder path
            
        Examples:
            # Create a content browser folder
            create_folder(folder_path="Content/MyGameContent")
            
            # Create a regular project folder
            create_folder(folder_path="Intermediate/MyTools")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "folder_path": folder_path
            }
            
            logger.info(f"Creating folder: {folder_path}")
            response = unreal.send_command("create_folder", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Folder creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating folder: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def create_struct(
        ctx: Context,
        struct_name: str,
        properties: List[Dict[str, str]],
        path: str = "/Game/Blueprints",
        description: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Unreal struct.
        
        Args:
            struct_name: Name of the struct to create
            properties: List of property dictionaries, each containing:
                        - name: Property name
                        - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
                        - description: (Optional) Property description
            path: Path where to create the struct
            description: Optional description for the struct
            
        Returns:
            Dictionary with the creation status and struct path
            
        Examples:
            # Create a simple Item struct
            create_struct(
                struct_name="Item",
                properties=[
                    {"name": "Name", "type": "String"},
                    {"name": "Value", "type": "Integer"},
                    {"name": "IsRare", "type": "Boolean"}
                ],
                path="/Game/DataStructures"
            )
        """
        return create_struct_impl(ctx, struct_name, properties, path, description)

    @mcp.tool()
    def update_struct(
        ctx: Context,
        struct_name: str,
        properties: List[Dict[str, str]],
        path: str = "/Game/Blueprints",
        description: str = ""
    ) -> Dict[str, Any]:
        """
        Update an existing Unreal struct.
        Args:
            struct_name: Name of the struct to update
            properties: List of property dictionaries, each containing:
                        - name: Property name
                        - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
                        - description: (Optional) Property description
            path: Path where the struct exists
            description: Optional description for the struct
        Returns:
            Dictionary with the update status and struct path
        """
        return update_struct_impl(ctx, struct_name, properties, path, description)

    @mcp.tool()
    def show_struct_variables(
        ctx: Context,
        struct_name: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Show variables and types of a struct in Unreal Engine.
        Args:
            struct_name: Name of the struct to inspect
            path: Path where the struct exists (default: /Game/Blueprints)
        Returns:
            Dictionary with struct variable info
        """
        return show_struct_variables_impl(ctx, struct_name, path)

    @mcp.tool()
    def list_folder_contents(
        ctx: Context,
        folder_path: str
    ) -> Dict[str, Any]:
        """
        List the contents of a folder in the Unreal project (content or regular folder).
        Args:
            folder_path: Path to the folder (e.g., "/Game/Blueprints" or "Content/MyFolder" or "Intermediate/MyTools")
        Returns:
            Dictionary with arrays of subfolders and files/assets
        Example:
            list_folder_contents(folder_path="/Game/Blueprints")
        """
        return list_folder_contents_impl(ctx, folder_path)

    logger.info("Project tools registered successfully")