"""
Project Tools for Unreal MCP.

This module provides tools for managing project-wide settings and configuration.
"""

import logging
from typing import Dict, Any, List
from mcp.server.fastmcp import FastMCP, Context
from utils.project.struct_operations import create_struct as create_struct_impl
from utils.project.struct_operations import update_struct as update_struct_impl
from utils.project.struct_operations import get_project_metadata as get_project_metadata_impl

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
    def create_enum(
        ctx: Context,
        enum_name: str,
        values: List[Dict[str, str]],
        path: str = "/Game/Blueprints",
        description: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new Unreal user-defined enum.

        Args:
            enum_name: Name of the enum to create (e.g., "E_ItemType")
            values: List of enum values. Can be either:
                    - Simple strings: ["Weapon", "Armor", "Consumable"]
                    - Objects with name and description: [{"name": "Weapon", "description": "Melee or ranged weapons"}]
            path: Path where to create the enum asset
            description: Optional description for the enum (shown in Enum Description field)

        Returns:
            Dictionary with the creation status and enum path

        Examples:
            # Simple enum (values as strings)
            create_enum(
                enum_name="E_ItemType",
                values=["Weapon", "Armor", "Consumable", "Material", "QuestItem"],
                path="/Game/Inventory/Data",
                description="Categories for inventory items"
            )

            # Enum with per-value descriptions
            create_enum(
                enum_name="E_EquipmentSlot",
                values=[
                    {"name": "None", "description": "No slot assigned"},
                    {"name": "Head", "description": "Helmet slot"},
                    {"name": "Chest", "description": "Body armor slot"},
                    {"name": "Hands", "description": "Gloves slot"},
                    {"name": "Feet", "description": "Boots slot"}
                ],
                path="/Game/Inventory/Data",
                description="Equipment slots for equippable items"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "enum_name": enum_name,
                "values": values,
                "path": path,
                "description": description
            }

            logger.info(f"Creating enum '{enum_name}' with {len(values)} values at '{path}'")
            response = unreal.send_command("create_enum", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Enum creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating enum: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_project_metadata(
        ctx: Context,
        fields: List[str] = None,
        path: str = "/Game",
        folder_path: str = None,
        struct_name: str = None
    ) -> Dict[str, Any]:
        """
        Get project metadata with selective field querying.

        This tool consolidates multiple project query tools into one flexible interface.

        Args:
            fields: List of metadata fields to retrieve. Options:
                - "input_actions": Enhanced Input Action assets in path
                - "input_contexts": Input Mapping Context assets in path
                - "structs": Struct variables (requires struct_name parameter)
                - "folder_contents": Folder contents (requires folder_path parameter)
                - "*": All available fields (default if None)
            path: Base path for searching input actions/contexts (default: /Game).
                  For structs, this is optional - if not provided, smart discovery will search
                  common paths and the asset registry.
            folder_path: Required for "folder_contents" field - path to list
            struct_name: Required for "structs" field - name or path of struct to inspect.
                        Supports: simple name ("S_InventoryItem"), partial path, or full path.
                        Smart discovery searches: /Game, /Game/Blueprints, /Game/Data,
                        /Game/Structs, /Game/Inventory/Data, and asset registry.

        Returns:
            Dictionary with requested project metadata

        Examples:
            # Get all input-related metadata
            get_project_metadata(
                ctx,
                fields=["input_actions", "input_contexts"],
                path="/Game/Input"
            )

            # Get struct variables (with smart discovery - just provide name)
            get_project_metadata(
                ctx,
                fields=["structs"],
                struct_name="S_InventoryItem"  # Will search common paths automatically
            )

            # Get struct variables (with explicit path)
            get_project_metadata(
                ctx,
                fields=["structs"],
                struct_name="PlayerStats",
                path="/Game/DataStructures"
            )

            # Get folder contents
            get_project_metadata(
                ctx,
                fields=["folder_contents"],
                folder_path="/Game/Blueprints"
            )

            # Get everything (input actions and contexts in /Game)
            get_project_metadata(ctx)
        """
        return get_project_metadata_impl(ctx, fields, path, folder_path, struct_name)

    @mcp.tool()
    def get_struct_pin_names(
        ctx: Context,
        struct_name: str
    ) -> Dict[str, Any]:
        """
        Get pin names (field names) for a user-defined struct.

        User-defined structs in Unreal Engine use GUID-based internal names for their
        fields (e.g., "ItemName_F2A4BC92"). This tool discovers those internal names
        so you can use them when creating Blueprint nodes that reference struct fields.

        Args:
            struct_name: Name or path of the struct to inspect. Supports:
                        - Simple name: "S_InventorySlot"
                        - Full path: "/Game/Inventory/Data/S_InventorySlot"

        Returns:
            Dictionary containing:
            - success: Whether the struct was found
            - struct_name: The struct name that was queried
            - struct_path: Full asset path of the struct
            - field_count: Number of fields in the struct
            - fields: Array of field information, each containing:
                - pin_name: The GUID-based internal name (use this for Blueprint nodes)
                - display_name: The friendly display name
                - type: The field's type
                - is_guid_name: Whether the pin_name contains a GUID suffix

        Examples:
            # Get field names for an inventory slot struct
            get_struct_pin_names(struct_name="S_InventorySlot")
            # Returns: {
            #     "fields": [
            #         {"pin_name": "ItemID_ABC123...", "display_name": "ItemID", "type": "Name"},
            #         {"pin_name": "StackCount_DEF456...", "display_name": "StackCount", "type": "int32"}
            #     ]
            # }

            # Get field names using full path
            get_struct_pin_names(struct_name="/Game/Inventory/Data/S_ItemDefinition")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "struct_name": struct_name
            }

            logger.info(f"Getting pin names for struct '{struct_name}'")
            response = unreal.send_command("get_struct_pin_names", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get struct pin names response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting struct pin names: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def duplicate_asset(
        ctx: Context,
        source_path: str,
        new_name: str,
        destination_path: str = None
    ) -> Dict[str, Any]:
        """
        Duplicate an existing asset (Blueprint, Widget, DataTable, Material, etc.) to a new location.

        This is useful for creating copies of existing assets as starting points for new functionality,
        avoiding the need to recreate complex assets from scratch.

        Args:
            source_path: Full path to the source asset (e.g., "/Game/Inventory/UI/WBP_InventorySlot")
            new_name: Name for the new asset (e.g., "WBP_LootSlot")
            destination_path: Optional destination folder path. If not provided, uses the same
                            folder as the source asset.

        Returns:
            Dictionary containing:
            - success: Whether the duplication succeeded
            - source_path: The original asset path
            - destination_path: The destination folder
            - new_name: The new asset name
            - new_asset_path: Full path to the newly created asset
            - message: Status message

        Examples:
            # Duplicate an inventory slot to create a loot slot in the same folder
            duplicate_asset(
                source_path="/Game/Inventory/UI/WBP_InventorySlot",
                new_name="WBP_LootSlot"
            )

            # Duplicate to a different folder
            duplicate_asset(
                source_path="/Game/Inventory/UI/WBP_InventorySlot",
                new_name="WBP_LootSlot",
                destination_path="/Game/Loot/UI"
            )

            # Duplicate a Blueprint actor
            duplicate_asset(
                source_path="/Game/Blueprints/BP_BaseEnemy",
                new_name="BP_BossEnemy",
                destination_path="/Game/Blueprints/Enemies"
            )

            # Duplicate a DataTable
            duplicate_asset(
                source_path="/Game/Data/DT_Items",
                new_name="DT_LootItems",
                destination_path="/Game/Loot/Data"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "source_path": source_path,
                "new_name": new_name
            }

            if destination_path:
                params["destination_path"] = destination_path

            logger.info(f"Duplicating asset '{source_path}' to '{new_name}'")
            response = unreal.send_command("duplicate_asset", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Duplicate asset response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error duplicating asset: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Project tools registered successfully")