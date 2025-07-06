"""
Utilities for working with Unreal Engine connections.

This module provides helper functions for common operations with Unreal Engine connections.
"""

import logging
from typing import Dict, List, Any

# Get logger
logger = logging.getLogger("UnrealMCP")

def get_unreal_engine_connection():
    """Get a connection to Unreal Engine."""
    from unreal_mcp_server import get_unreal_connection
    
    unreal = get_unreal_connection()
    if not unreal:
        logger.error("Failed to connect to Unreal Engine")
    
    return unreal

def send_unreal_command(command_name: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to Unreal Engine with proper error handling."""
    try:
        unreal = get_unreal_engine_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        logger.info(f"Sending command '{command_name}' with params: {params}")
        response = unreal.send_command(command_name, params)
        
        if not response:
            logger.error(f"No response from Unreal Engine for command '{command_name}'")
            return {"success": False, "message": "No response from Unreal Engine"}
        
        logger.info(f"Command '{command_name}' response: {response}")
        return response
        
    except Exception as e:
        error_msg = f"Error executing Unreal command '{command_name}': {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}