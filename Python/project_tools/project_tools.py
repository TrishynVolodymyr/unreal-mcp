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
    def update_enum(
        ctx: Context,
        enum_name: str,
        values: List[Dict[str, str]],
        path: str = "/Game/Blueprints",
        description: str = ""
    ) -> Dict[str, Any]:
        """
        Update an existing Unreal user-defined enum.

        Replaces all existing enum values with the new set of values.

        Args:
            enum_name: Name of the enum to update (e.g., "E_ItemType")
            values: List of enum values. Can be either:
                    - Simple strings: ["Weapon", "Armor", "Consumable"]
                    - Objects with name and description: [{"name": "Weapon", "description": "Melee or ranged weapons"}]
            path: Path where the enum exists
            description: Optional new description for the enum

        Returns:
            Dictionary with the update status

        Examples:
            # Update enum with simple string values
            update_enum(
                enum_name="E_QuestStatus",
                values=["Available", "Active", "Completed", "Failed"],
                path="/Game/Quests/Data"
            )

            # Update enum with per-value descriptions
            update_enum(
                enum_name="E_ItemRarity",
                values=[
                    {"name": "Common", "description": "Basic items"},
                    {"name": "Uncommon", "description": "Better than average"},
                    {"name": "Rare", "description": "Hard to find items"},
                    {"name": "Epic", "description": "Very powerful items"},
                    {"name": "Legendary", "description": "The rarest items"}
                ],
                path="/Game/Inventory/Data",
                description="Item rarity levels"
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

            logger.info(f"Updating enum '{enum_name}' with {len(values)} values at '{path}'")
            response = unreal.send_command("update_enum", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Enum update response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error updating enum: {e}"
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

    @mcp.tool()
    def create_font_face(
        ctx: Context,
        font_name: str,
        source_texture: str,
        path: str = "/Game/Fonts",
        use_sdf: bool = True,
        distance_field_spread: int = 4,
        font_metrics: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Create a new FontFace asset from an SDF texture.

        FontFace assets are required to use custom fonts in UMG widgets. This tool creates
        a FontFace that references an SDF (Signed Distance Field) texture for high-quality
        text rendering at any scale.

        Args:
            font_name: Name of the FontFace asset to create (e.g., "FF_DarkFantasy_Regular")
            source_texture: Path to the source SDF texture (e.g., "/Game/Fonts/DarkFantasy_Regular_sdf")
            path: Path where to create the FontFace asset (default: "/Game/Fonts")
            use_sdf: Whether this font uses SDF rendering (default: True)
            distance_field_spread: SDF spread value in pixels (default: 4)
            font_metrics: Optional font metrics dict with keys:
                - ascender: Font ascender value
                - descender: Font descender value
                - line_height: Line height value

        Returns:
            Dictionary containing:
            - success: Whether the FontFace was created
            - font_path: Full path to the created FontFace asset
            - message: Status message

        Examples:
            # Create a basic SDF FontFace
            create_font_face(
                font_name="FF_DarkFantasy_Regular",
                source_texture="/Game/Fonts/DarkFantasy_Regular_sdf"
            )

            # Create FontFace with custom metrics
            create_font_face(
                font_name="FF_DarkFantasy_Bold",
                source_texture="/Game/Fonts/DarkFantasy_Bold_sdf",
                path="/Game/UI/Fonts",
                distance_field_spread=6,
                font_metrics={
                    "ascender": 0.8,
                    "descender": -0.2,
                    "line_height": 1.2
                }
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_name": font_name,
                "source_texture": source_texture,
                "path": path,
                "use_sdf": use_sdf,
                "distance_field_spread": distance_field_spread
            }

            if font_metrics:
                params["font_metrics"] = font_metrics

            logger.info(f"Creating FontFace '{font_name}' from texture '{source_texture}'")
            response = unreal.send_command("create_font_face", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create FontFace response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating FontFace: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_font_face_properties(
        ctx: Context,
        font_path: str,
        properties: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Set properties on an existing FontFace asset.

        Args:
            font_path: Path to the FontFace asset (e.g., "/Game/Fonts/FF_DarkFantasy_Regular")
            properties: Dictionary of properties to set. Supported properties:
                - Hinting: Font hinting mode ("Default", "Auto", "AutoLight", "Monochrome", "None")
                - LoadingPolicy: Font loading policy ("LazyLoad", "Stream", "Inline")
                - Ascender: Font ascender value (float)
                - Descender: Font descender value (float)
                - SubFaceIndex: Sub-face index for multi-face fonts (int)

        Returns:
            Dictionary containing:
            - success: Whether properties were set
            - font_path: Path to the modified FontFace
            - success_properties: List of properties that were set successfully
            - failed_properties: List of properties that failed to set
            - message: Status message

        Examples:
            # Set hinting mode
            set_font_face_properties(
                font_path="/Game/Fonts/FF_DarkFantasy_Regular",
                properties={"Hinting": "None"}
            )

            # Set multiple properties
            set_font_face_properties(
                font_path="/Game/Fonts/FF_DarkFantasy_Regular",
                properties={
                    "Hinting": "AutoLight",
                    "LoadingPolicy": "Inline",
                    "Ascender": 0.85
                }
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_path": font_path,
                "properties": properties
            }

            logger.info(f"Setting properties on FontFace '{font_path}'")
            response = unreal.send_command("set_font_face_properties", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set FontFace properties response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting FontFace properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_font_face_metadata(
        ctx: Context,
        font_path: str
    ) -> Dict[str, Any]:
        """
        Get metadata about an existing FontFace asset.

        Args:
            font_path: Path to the FontFace asset (e.g., "/Game/Fonts/FF_DarkFantasy_Regular")

        Returns:
            Dictionary containing:
            - success: Whether the FontFace was found
            - font_path: Path to the FontFace asset
            - font_name: Name of the FontFace
            - source_filename: Original source file (if available)
            - hinting: Current hinting mode
            - loading_policy: Current loading policy
            - ascender: Font ascender value
            - descender: Font descender value
            - sub_face_index: Sub-face index
            - message: Status message

        Examples:
            # Get FontFace metadata
            get_font_face_metadata(font_path="/Game/Fonts/FF_DarkFantasy_Regular")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_path": font_path
            }

            logger.info(f"Getting metadata for FontFace '{font_path}'")
            response = unreal.send_command("get_font_face_metadata", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get FontFace metadata response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting FontFace metadata: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_offline_font(
        ctx: Context,
        font_name: str,
        texture_path: str,
        metrics_file_path: str,
        path: str = "/Game/Fonts"
    ) -> Dict[str, Any]:
        """
        Create an offline (bitmap/SDF atlas) font from a texture and metrics JSON file.

        This tool creates a UFont asset with offline caching that uses a pre-rendered
        texture atlas (like SDF fonts) instead of TTF font data. The metrics JSON file
        (on disk, NOT an Unreal asset) provides character positions and font metrics.

        Args:
            font_name: Name of the font asset to create (e.g., "Font_DarkFantasy_Regular")
            texture_path: Path to the SDF texture atlas (e.g., "/Game/Fonts/DarkFantasy_Regular_sdf")
            metrics_file_path: Absolute file path to the metrics JSON file on disk (NOT an Unreal asset path).
                The JSON file should have the following structure:
                - atlasWidth: Width of the texture atlas in pixels
                - atlasHeight: Height of the texture atlas in pixels
                - lineHeight: Line height in pixels
                - baseline: Baseline position from top in pixels
                - characters: Object mapping character codes to glyph data:
                    - u, v: Normalized UV coordinates (0-1)
                    - width, height: Glyph dimensions in pixels
                    - xOffset, yOffset: Positioning offsets
                    - xAdvance: Horizontal advance for cursor
            path: Path where to create the font asset (default: "/Game/Fonts")

        Returns:
            Dictionary containing:
            - success: Whether the font was created
            - font_name: Name of the created font
            - font_path: Full path to the created font asset
            - message: Status message

        Examples:
            # Create font from SDF texture and metrics file
            create_offline_font(
                font_name="Font_DarkFantasy_Regular",
                texture_path="/Game/Fonts/DarkFantasy_Regular_sdf",
                metrics_file_path="E:/code/unreal-mcp/Docs/fonts/DarkFantasy_Regular_metrics.json"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_name": font_name,
                "texture_path": texture_path,
                "metrics_file_path": metrics_file_path,
                "path": path
            }

            logger.info(f"Creating offline font '{font_name}' from texture '{texture_path}' with metrics from '{metrics_file_path}'")
            response = unreal.send_command("create_offline_font", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create offline font response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating offline font: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_font_metadata(
        ctx: Context,
        font_path: str
    ) -> Dict[str, Any]:
        """
        Get metadata about an existing UFont asset.

        Args:
            font_path: Path to the font asset (e.g., "/Game/Fonts/Font_DarkFantasy_Regular")

        Returns:
            Dictionary containing:
            - success: Whether the font was found
            - font_path: Path to the font asset
            - font_name: Name of the font
            - cache_type: "Offline" or "Runtime"
            - em_scale: Em scale factor
            - ascent: Font ascent
            - descent: Font descent
            - leading: Line leading
            - kerning: Default kerning
            - scaling_factor: Scaling factor
            - legacy_font_size: Legacy font size
            - character_count: Number of characters
            - texture_count: Number of textures
            - is_remapped: Whether character remapping is used

        Examples:
            # Get font metadata
            get_font_metadata(font_path="/Game/Fonts/Font_DarkFantasy_Regular")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_path": font_path
            }

            logger.info(f"Getting metadata for font '{font_path}'")
            response = unreal.send_command("get_font_metadata", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get font metadata response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting font metadata: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_font(
        ctx: Context,
        font_name: str,
        source_type: str,
        ttf_file_path: str = None,
        sdf_texture: str = None,
        atlas_texture: str = None,
        metrics_file: str = None,
        path: str = "/Game/Fonts",
        use_sdf: bool = True,
        distance_field_spread: int = 32,
        font_metrics: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Unified font creation command supporting multiple source types.

        This is the recommended tool for creating font assets in Unreal Engine.
        It consolidates TTF import, SDF texture, and offline bitmap font workflows.

        Args:
            font_name: Name of the font asset to create (e.g., "Font_CinzelTorn")
            source_type: Type of font source. One of:
                - "ttf": Import an external TTF file as a FontFace asset
                - "sdf_texture": Create a FontFace from an SDF texture
                - "offline": Create a UFont from a texture atlas and metrics JSON
            ttf_file_path: (Required for "ttf") Absolute path to the TTF file on disk
                Example: "E:/code/unreal-mcp/Python/font_generation/Cinzel-Torn.ttf"
            sdf_texture: (Optional for "sdf_texture") Path to the SDF texture in UE
                Example: "/Game/Fonts/DarkFantasy_sdf"
            atlas_texture: (Required for "offline") Path to the texture atlas in UE
                Example: "/Game/Fonts/DarkFantasy_atlas"
            metrics_file: (Required for "offline") Absolute path to metrics JSON on disk
                Example: "E:/fonts/DarkFantasy_metrics.json"
            path: Path where to create the font asset (default: "/Game/Fonts")
            use_sdf: (For "sdf_texture") Whether to use SDF rendering (default: True)
            distance_field_spread: (For "sdf_texture") SDF spread value (default: 32)
            font_metrics: Optional font metrics dict with keys:
                - ascender: Font ascender value
                - descender: Font descender value
                - line_height: Line height value

        Returns:
            Dictionary containing:
            - success: Whether the font was created
            - font_name: Name of the created font
            - source_type: The source type used
            - asset_path: Full path to the created font asset
            - message: Status message

        Examples:
            # Import a TTF file (most common use case)
            create_font(
                font_name="Font_CinzelTorn",
                source_type="ttf",
                ttf_file_path="E:/code/unreal-mcp/Python/font_generation/Cinzel-Torn.ttf"
            )

            # Create from SDF texture
            create_font(
                font_name="FF_DarkFantasy",
                source_type="sdf_texture",
                sdf_texture="/Game/Fonts/DarkFantasy_sdf",
                distance_field_spread=6
            )

            # Create offline/bitmap font from atlas
            create_font(
                font_name="Font_Bitmap",
                source_type="offline",
                atlas_texture="/Game/Fonts/Bitmap_atlas",
                metrics_file="E:/fonts/bitmap_metrics.json"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # Validate source_type
            valid_source_types = ["ttf", "sdf_texture", "offline"]
            if source_type not in valid_source_types:
                return {
                    "success": False,
                    "message": f"Invalid source_type '{source_type}'. Must be one of: {valid_source_types}"
                }

            # Build params based on source_type
            params = {
                "font_name": font_name,
                "source_type": source_type,
                "path": path
            }

            if source_type == "ttf":
                if not ttf_file_path:
                    return {"success": False, "message": "ttf_file_path is required for source_type='ttf'"}
                params["ttf_file_path"] = ttf_file_path
                if font_metrics:
                    params["font_metrics"] = font_metrics

            elif source_type == "sdf_texture":
                if sdf_texture:
                    params["sdf_texture"] = sdf_texture
                params["use_sdf"] = use_sdf
                params["distance_field_spread"] = distance_field_spread
                if font_metrics:
                    params["font_metrics"] = font_metrics

            elif source_type == "offline":
                if not atlas_texture:
                    return {"success": False, "message": "atlas_texture is required for source_type='offline'"}
                if not metrics_file:
                    return {"success": False, "message": "metrics_file is required for source_type='offline'"}
                params["atlas_texture"] = atlas_texture
                params["metrics_file"] = metrics_file

            logger.info(f"Creating font '{font_name}' with source_type='{source_type}'")
            response = unreal.send_command("create_font", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create font response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating font: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Project tools registered successfully")