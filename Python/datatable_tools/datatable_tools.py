from __future__ import annotations
from typing import Dict, List, Any, Optional
import sys
import os

# Add the parent directory to sys.path to enable absolute imports
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.datatable.datatable_utils import (
    create_datatable_impl,
    get_datatable_rows_impl,
    get_datatable_row_names_impl,
    add_rows_to_datatable_impl,
    update_rows_in_datatable_impl,
    delete_datatable_rows_impl
)

def register_datatable_tools(mcp: 'FastMCP'):
    """Register DataTable-related tools with the MCP server."""
    
    @mcp.tool()
    def create_datatable(
        datatable_name: str,
        row_struct_name: str,
        path: str = "/Game/Data",
        description: str = ""
    ) -> Dict[str, Any]:
        """Create a new DataTable asset.
        
        Args:
            datatable_name: Name of the DataTable to create
            row_struct_name: Name or path of the struct to use for rows. Can be:
                - Full path (recommended): "/Game/Path/To/MyStruct"
                - Simple name: "MyStruct" (will search in multiple locations)
                The system will try to find the struct in the following locations:
                1. Direct path if starting with "/Game/" or "/Script/"
                2. Engine and core paths
                3. Game content paths:
                   - /Game/Blueprints/
                   - /Game/Data/
                   - /Game/ (root)
            path: Path where to create the DataTable (defaults to "/Game/Data")
            description: Optional description for the DataTable
            
        Returns:
            Dict containing success status and created asset info
            
        Examples:
            # Create with full struct path (recommended)
            create_datatable(
                datatable_name="ItemTable",
                row_struct_name="/Game/Data/ItemStruct",
                path="/Game/Data"
            )
            
            # Create with simple struct name (will search in multiple locations)
            create_datatable(
                datatable_name="ItemTable",
                row_struct_name="ItemStruct"
            )
            
            # Create with description
            create_datatable(
                datatable_name="ItemTable",
                row_struct_name="/Game/Data/ItemStruct",
                description="Table containing all game items"
            )
        """
        return create_datatable_impl(datatable_name, row_struct_name, path, description)
    
    @mcp.tool()
    def get_datatable_rows(
        datatable_path: str,
        row_names: Optional[List[str]] = None
    ) -> Dict[str, Any]:
        """Get rows from a DataTable.
        
        Args:
            datatable_path: Path to the target DataTable
            row_names: Optional list of specific row names to retrieve
            
        Returns:
            Dict containing the requested rows
        """
        return get_datatable_rows_impl(datatable_path, row_names)
    

    
    @mcp.tool()
    def get_datatable_row_names(
        datatable_path: str
    ) -> Dict[str, Any]:
        """Get all row names and struct field names from a DataTable.
        
        Args:
            datatable_path: Path to the target DataTable
        Returns:
            Dict containing:
                - 'row_names': list of row names (str)
                - 'field_names': list of struct field names (str)
        """
        return get_datatable_row_names_impl(datatable_path)
    
    @mcp.tool()
    def add_rows_to_datatable(
        datatable_path: str,
        rows: list[dict]
    ) -> Dict[str, Any]:
        """Add multiple rows to an existing DataTable.
        Args:
            datatable_path: Path to the target DataTable. Can be:
                - Full path (recommended): "/Game/Data/MyTable"
                - Simple name: "MyTable" (will search in multiple locations)
            rows: List of dicts, each with:
                - 'row_name': Unique name for the row
                - 'row_data': Dict of property values using the internal GUID-based property names
                             You must first call get_datatable_row_names() to get the correct
                             property names, as they include auto-generated GUIDs.
        Returns:
            Dict containing success status and list of added row names

        Examples:
            # First get the correct property names
            property_info = get_datatable_row_names("MyItemTable")
            field_names = property_info["result"]["field_names"]  # Contains GUID-based names
            
            # Then add rows using those property names
            add_rows_to_datatable(
                datatable_path="/Game/Data/MyItemTable",
                rows=[{
                    "row_name": "Item1",
                    "row_data": {
                        field_names[0]: "Sword",      # Name field
                        field_names[1]: 99.99,        # Price field
                        field_names[2]: 5,            # Quantity field
                        field_names[3]: True,         # IsAvailable field
                        field_names[4]: ["weapon"]    # Tags field
                    }
                }]
            )
        """
        return add_rows_to_datatable_impl(datatable_path, rows)
    
    @mcp.tool()
    def update_rows_in_datatable(
        datatable_path: str,
        rows: list[dict]
    ) -> Dict[str, Any]:
        """Update multiple rows in an existing DataTable.
        Args:
            datatable_path: Path to the target DataTable. Can be:
                - Full path (recommended): "/Game/Data/MyTable"
                - Simple name: "MyTable" (will search in multiple locations)
            rows: List of dicts, each with:
                - 'row_name': Name of the row to update
                - 'row_data': Dict of property values using the internal GUID-based property names
                             You must first call get_datatable_row_names() to get the correct
                             property names, as they include auto-generated GUIDs.
        Returns:
            Dict containing success status and list of updated/failed row names

        Examples:
            # First get the correct property names
            property_info = get_datatable_row_names("MyItemTable")
            field_names = property_info["result"]["field_names"]  # Contains GUID-based names
            
            # Then update existing rows using those property names
            update_rows_in_datatable(
                datatable_path="/Game/Data/MyItemTable",
                rows=[{
                    "row_name": "Item1",
                    "row_data": {
                        field_names[0]: "Enhanced Sword",  # Name field
                        field_names[1]: 149.99,           # Price field
                        field_names[2]: 3,                # Quantity field
                        field_names[3]: True,             # IsAvailable field
                        field_names[4]: ["weapon", "rare"] # Tags field
                    }
                }]
            )
        """
        return update_rows_in_datatable_impl(datatable_path, rows)
    
    @mcp.tool()
    def delete_datatable_rows(
        datatable_path: str,
        row_names: List[str]
    ) -> Dict[str, Any]:
        """Delete multiple rows from a DataTable.
        Args:
            datatable_path: Path to the target DataTable
            row_names: List of row names to delete
        Returns:
            Dict containing success status and updated DataTable info
        """
        return delete_datatable_rows_impl(datatable_path, row_names) 