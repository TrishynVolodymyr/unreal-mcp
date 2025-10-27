"""
Utilities for working with Unreal Engine DataTables.

This module provides helper functions for common operations with DataTables in Unreal Engine.
"""

import logging
from typing import Dict, List, Any, Optional

from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def create_datatable_impl(
    datatable_name: str,
    row_struct_name: str,
    path: str = "/Game/Data",
    description: str = ""
) -> Dict[str, Any]:
    """Create a new DataTable asset in Unreal Engine.
    
    Args:
        datatable_name: Name of the DataTable to create
        row_struct_name: Name of the struct to use for rows
        path: Path where to create the DataTable
        description: Optional description for the DataTable
        
    Returns:
        Dict containing success status and created asset info
    """
    params = {
        "datatable_name": datatable_name,
        "row_struct_name": row_struct_name,
        "path": path,
        "description": description
    }
    
    return send_unreal_command("create_datatable", params)

def get_datatable_rows_impl(
    datatable_path: str,
    row_names: Optional[List[str]] = None
) -> Dict[str, Any]:
    """Get rows from a DataTable in Unreal Engine.
    
    Args:
        datatable_path: Path to the target DataTable
        row_names: Optional list of specific row names to retrieve
        
    Returns:
        Dict containing the requested rows
    """
    params = {
        "datatable_path": datatable_path,
        "row_names": row_names
    }
    
    return send_unreal_command("get_datatable_rows", params)



def get_datatable_row_names_impl(
    datatable_path: str
) -> Dict[str, Any]:
    """Get all row names and struct field names from a DataTable in Unreal Engine.
    
    Args:
        datatable_path: Path to the target DataTable
    Returns:
        Dict containing:
            - 'row_names': list of row names (str)
            - 'field_names': list of struct field names (str)
    """
    params = {
        "datatable_path": datatable_path
    }
    return send_unreal_command("get_datatable_row_names", params)

def add_rows_to_datatable_impl(
    datatable_path: str,
    rows: list[dict]
) -> Dict[str, Any]:
    """Add multiple rows to an existing DataTable in Unreal Engine.
    Args:
        datatable_path: Path to the target DataTable
        rows: List of dicts, each with 'row_name' and 'row_data'
    Returns:
        Dict containing success status and list of added row names
    """
    # Auto-fill missing fields for better user experience
    processed_rows = []
    for row in rows:
        processed_row = row.copy()
        if 'row_data' in processed_row:
            # Ensure row_data has basic structure
            row_data = processed_row['row_data']
            
            # Auto-add empty arrays for common array fields if not present
            if 'responses' not in row_data and 'Responses' not in row_data:
                # Try both lowercase and capitalized versions
                row_data['responses'] = []
                logger.info(f"Auto-filled missing 'responses' field with empty array for row '{row.get('row_name', 'unknown')}'")
        
        processed_rows.append(processed_row)
    
    params = {
        "datatable_path": datatable_path,
        "rows": processed_rows
    }
    return send_unreal_command("add_rows_to_datatable", params)

def update_rows_in_datatable_impl(
    datatable_path: str,
    rows: list[dict]
) -> Dict[str, Any]:
    """Update multiple rows in an existing DataTable in Unreal Engine.
    Args:
        datatable_path: Path to the target DataTable
        rows: List of dicts, each with 'row_name' and 'row_data'
    Returns:
        Dict containing success status and list of updated/failed row names
    """
    # Auto-fill missing fields for better user experience
    processed_rows = []
    for row in rows:
        processed_row = row.copy()
        if 'row_data' in processed_row:
            # Ensure row_data has basic structure
            row_data = processed_row['row_data']
            
            # Auto-add empty arrays for common array fields if not present
            if 'responses' not in row_data and 'Responses' not in row_data:
                # Try both lowercase and capitalized versions
                row_data['responses'] = []
                logger.info(f"Auto-filled missing 'responses' field with empty array for row '{row.get('row_name', 'unknown')}'")
        
        processed_rows.append(processed_row)
    
    params = {
        "datatable_path": datatable_path,
        "rows": processed_rows
    }
    return send_unreal_command("update_rows_in_datatable", params)

def delete_datatable_rows_impl(
    datatable_path: str,
    row_names: List[str]
) -> Dict[str, Any]:
    """Delete multiple rows from a DataTable in Unreal Engine.
    Args:
        datatable_path: Path to the target DataTable
        row_names: List of row names to delete
    Returns:
        Dict containing success status and updated DataTable info
    """
    params = {
        "datatable_path": datatable_path,
        "row_names": row_names
    }
    return send_unreal_command("delete_datatable_rows", params) 