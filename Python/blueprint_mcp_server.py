#!/usr/bin/env python3
"""
Blueprint MCP Server

This server provides MCP tools for Blueprint operations in Unreal Engine,
including the enhanced compilation error reporting functionality.
"""

import asyncio
import json
import socket
from typing import Any, Dict, List, Optional

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("Blueprint MCP Server")

# TCP connection settings
TCP_HOST = "127.0.0.1"
TCP_PORT = 55557

async def send_tcp_command(command_type: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to the Unreal Engine TCP server."""
    try:
        # Create the command payload
        command_data = {
            "type": command_type,
            "params": params
        }
        
        # Convert to JSON string
        json_data = json.dumps(command_data)
        
        # Connect to TCP server
        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)
        
        # Send command
        writer.write(json_data.encode('utf-8'))
        writer.write(b'\n')  # Add newline delimiter
        await writer.drain()
        
        # Read response
        response_data = await reader.read(8192)  # Read up to 8KB
        response_str = response_data.decode('utf-8').strip()
        
        # Close connection
        writer.close()
        await writer.wait_closed()
        
        # Parse JSON response
        if response_str:
            return json.loads(response_str)
        else:
            return {"success": False, "error": "Empty response from server"}
            
    except Exception as e:
        return {"success": False, "error": f"TCP communication error: {str(e)}"}

@app.tool()
async def create_blueprint(name: str, parent_class: str, folder_path: str = "") -> Dict[str, Any]:
    """
    Create a new Blueprint class.
    
    Args:
        name: Name of the new Blueprint class
        parent_class: Parent class for the Blueprint (e.g., "Actor", "Pawn")
        folder_path: Optional folder path where the blueprint should be created
    
    Returns:
        Dictionary containing information about the created Blueprint
    """
    params = {
        "name": name,
        "parent_class": parent_class
    }
    if folder_path:
        params["folder_path"] = folder_path
    
    return await send_tcp_command("create_blueprint", params)

@app.tool()
async def compile_blueprint(blueprint_name: str) -> Dict[str, Any]:
    """
    Compile a Blueprint with enhanced error reporting.
    
    This tool provides detailed compilation error information including:
    - Node-level errors (missing connections, invalid types)
    - Graph-level errors (unreachable code, missing entry points)
    - Blueprint-level errors (missing parent class, invalid configurations)
    - Component-level errors (invalid classes, missing templates)
    
    Args:
        blueprint_name: Name of the target Blueprint
    
    Returns:
        Dictionary containing compilation results with detailed error information:
        - For successful compilation: success=True, compilation_time_seconds, status
        - For failed compilation: success=False, error message, compilation_errors array
        - For compilation with warnings: success=True, warnings array
    """
    params = {
        "blueprint_name": blueprint_name
    }
    
    return await send_tcp_command("compile_blueprint", params)

@app.tool()
async def add_blueprint_variable(
    blueprint_name: str, 
    variable_name: str, 
    variable_type: str, 
    is_exposed: bool = False
) -> Dict[str, Any]:
    """
    Add a variable to a Blueprint.
    
    Args:
        blueprint_name: Name of the target Blueprint
        variable_name: Name of the variable
        variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
        is_exposed: Whether to expose the variable to the editor
    
    Returns:
        Response indicating success or failure
    """
    params = {
        "blueprint_name": blueprint_name,
        "variable_name": variable_name,
        "variable_type": variable_type,
        "is_exposed": is_exposed
    }
    
    return await send_tcp_command("add_blueprint_variable", params)

@app.tool()
async def add_component_to_blueprint(
    blueprint_name: str,
    component_type: str,
    component_name: str,
    location: Optional[List[float]] = None,
    rotation: Optional[List[float]] = None,
    scale: Optional[List[float]] = None
) -> Dict[str, Any]:
    """
    Add a component to a Blueprint.
    
    Args:
        blueprint_name: Name of the target Blueprint
        component_type: Type of component to add
        component_name: Name for the new component
        location: [X, Y, Z] coordinates for component's position
        rotation: [Pitch, Yaw, Roll] values for component's rotation
        scale: [X, Y, Z] values for component's scale
    
    Returns:
        Information about the added component
    """
    params = {
        "blueprint_name": blueprint_name,
        "component_type": component_type,
        "component_name": component_name
    }
    
    if location:
        params["location"] = location
    if rotation:
        params["rotation"] = rotation
    if scale:
        params["scale"] = scale
    
    return await send_tcp_command("add_component_to_blueprint", params)

@app.tool()
async def set_component_property(
    blueprint_name: str,
    component_name: str,
    **kwargs
) -> Dict[str, Any]:
    """
    Set properties on a Blueprint component.
    
    Args:
        blueprint_name: Name of the target Blueprint
        component_name: Name of the component to modify
        **kwargs: Properties to set (passed as JSON string)
    
    Returns:
        Dictionary containing success status and property results
    """
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "kwargs": json.dumps(kwargs)
    }
    
    return await send_tcp_command("set_component_property", params)

@app.tool()
async def list_blueprint_components(blueprint_name: str) -> Dict[str, Any]:
    """
    List all components in a Blueprint class.
    
    Args:
        blueprint_name: Name of the target Blueprint
    
    Returns:
        Dictionary with a list of component names and types
    """
    params = {
        "blueprint_name": blueprint_name
    }
    
    return await send_tcp_command("list_blueprint_components", params)

@app.tool()
async def create_custom_blueprint_function(
    blueprint_name: str,
    function_name: str,
    inputs: Optional[List[Dict[str, str]]] = None,
    outputs: Optional[List[Dict[str, str]]] = None,
    is_pure: bool = False,
    is_const: bool = False,
    access_specifier: str = "Public",
    category: str = "Default"
) -> Dict[str, Any]:
    """
    Create a custom user-defined function in a Blueprint.
    This will create a new function that appears in the Functions section of the Blueprint editor.
    
    Args:
        blueprint_name: Name of the target Blueprint
        function_name: Name of the custom function to create
        inputs: List of input parameters, each with 'name' and 'type' keys
        outputs: List of output parameters, each with 'name' and 'type' keys  
        is_pure: Whether the function is pure (no execution pins)
        is_const: Whether the function is const
        access_specifier: Access level ("Public", "Protected", "Private")
        category: Category for organization in the functions list
        
    Returns:
        Dictionary containing success status and function information
        
    Examples:
        # Create a simple function with no parameters
        create_custom_blueprint_function(
            blueprint_name="BP_LoopTest",
            function_name="TestLoopFunction"
        )
        
        # Create a function with input and output parameters
        create_custom_blueprint_function(
            blueprint_name="BP_LoopTest", 
            function_name="ProcessArray",
            inputs=[{"name": "InputArray", "type": "String[]"}],
            outputs=[{"name": "ProcessedCount", "type": "Integer"}]
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name,
        "is_pure": is_pure,
        "is_const": is_const,
        "access_specifier": access_specifier,
        "category": category
    }
    
    if inputs:
        params["inputs"] = inputs
    if outputs:
        params["outputs"] = outputs
    
    return await send_tcp_command("create_custom_blueprint_function", params)

if __name__ == "__main__":
    app.run()