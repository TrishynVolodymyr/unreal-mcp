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
    datatable_name: str,
    row_names: Optional[List[str]] = None
) -> Dict[str, Any]:
    """Get rows from a DataTable in Unreal Engine.
    
    Args:
        datatable_name: Name of the target DataTable
        row_names: Optional list of specific row names to retrieve
        
    Returns:
        Dict containing the requested rows
    """
    params = {
        "datatable_name": datatable_name,
        "row_names": row_names
    }
    
    return send_unreal_command("get_datatable_rows", params)

def delete_datatable_row_impl(
    datatable_name: str,
    row_name: str
) -> Dict[str, Any]:
    """Delete a row from a DataTable in Unreal Engine.
    
    Args:
        datatable_name: Name of the target DataTable
        row_name: Name of the row to delete
        
    Returns:
        Dict containing success status and updated DataTable info
    """
    params = {
        "datatable_name": datatable_name,
        "row_name": row_name
    }
    
    return send_unreal_command("delete_datatable_row", params)

def get_datatable_row_names_impl(
    datatable_name: str
) -> Dict[str, Any]:
    """Get all row names and struct field names from a DataTable in Unreal Engine.
    
    Args:
        datatable_name: Name of the target DataTable
    Returns:
        Dict containing:
            - 'row_names': list of row names (str)
            - 'field_names': list of struct field names (str)
    """
    params = {
        "datatable_name": datatable_name
    }
    return send_unreal_command("get_datatable_row_names", params)

def add_rows_to_datatable_impl(
    datatable_name: str,
    rows: list[dict]
) -> Dict[str, Any]:
    """Add multiple rows to an existing DataTable in Unreal Engine.
    Args:
        datatable_name: Name of the target DataTable
        rows: List of dicts, each with 'row_name' and 'row_data'
    Returns:
        Dict containing success status and list of added row names
    """
    params = {
        "datatable_name": datatable_name,
        "rows": rows
    }
    return send_unreal_command("add_rows_to_datatable", params)

def update_rows_in_datatable_impl(
    datatable_name: str,
    rows: list[dict]
) -> Dict[str, Any]:
    """Update multiple rows in an existing DataTable in Unreal Engine.
    Args:
        datatable_name: Name of the target DataTable
        rows: List of dicts, each with 'row_name' and 'row_data'
    Returns:
        Dict containing success status and list of updated/failed row names
    """
    params = {
        "datatable_name": datatable_name,
        "rows": rows
    }
    return send_unreal_command("update_rows_in_datatable", params)

def delete_datatable_rows_impl(
    datatable_name: str,
    row_names: List[str]
) -> Dict[str, Any]:
    """Delete multiple rows from a DataTable in Unreal Engine.
    Args:
        datatable_name: Name of the target DataTable
        row_names: List of row names to delete
    Returns:
        Dict containing success status and updated DataTable info
    """
    params = {
        "datatable_name": datatable_name,
        "row_names": row_names
    }
    return send_unreal_command("delete_datatable_rows", params) 