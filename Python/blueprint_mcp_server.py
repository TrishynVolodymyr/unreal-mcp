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
        
        # Read response (увеличено буфер для детальних помилок компіляції)
        response_data = await reader.read(49152)  # Read up to 48KB
        response_str = response_data.decode('utf-8').strip()
        
        # Close connection
        writer.close()
        await writer.wait_closed()
        
        # Parse JSON response
        if response_str:
            try:
                return json.loads(response_str)
            except json.JSONDecodeError as json_err:
                return {"success": False, "error": f"JSON decode error: {str(json_err)}, Response: {response_str[:200]}"}
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
async def modify_blueprint_component_properties(
    blueprint_name: str,
    component_name: str,
    **kwargs
) -> Dict[str, Any]:
    """
    Modify ANY properties on a Blueprint component.
    
    This is a universal tool for setting component properties including:
    - WidgetClass on WidgetComponent
    - StaticMesh on StaticMeshComponent  
    - Material on any mesh component
    - LightColor, Intensity on LightComponent
    - Any other component-specific properties
    
    Args:
        blueprint_name: Name of the target Blueprint
        component_name: Name of the component to modify
        **kwargs: Component properties to set (e.g., WidgetClass, StaticMesh, Material, LightColor, etc.)
    
    Returns:
        Dictionary containing success status and list of successfully set properties
        
    Examples:
        # Set widget class on WidgetComponent
        modify_blueprint_component_properties(
            blueprint_name="BP_DialogueNPC",
            component_name="InteractionWidget",
            WidgetClass="/Game/Dialogue/WBP_InteractionPrompt.WBP_InteractionPrompt_C"
        )
        
        # Set static mesh and material
        modify_blueprint_component_properties(
            blueprint_name="BP_Prop",
            component_name="MeshComponent",
            StaticMesh="/Game/Meshes/SM_Crate",
            Material="/Game/Materials/M_Wood"
        )
        
        # Set light properties
        modify_blueprint_component_properties(
            blueprint_name="BP_Lamp",
            component_name="PointLight",
            LightColor=[1.0, 0.8, 0.6],
            Intensity=5000.0
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "kwargs": json.dumps(kwargs)
    }
    
    return await send_tcp_command("modify_blueprint_component_properties", params)

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


@app.tool()
async def set_blueprint_variable_value(
    blueprint_name: str,
    variable_name: str,
    value: Any
) -> Dict[str, Any]:
    """
    Set the default value of a Blueprint variable.

    This sets the value on the Blueprint's Class Default Object (CDO), which means
    it becomes the default value for all instances of this Blueprint.

    Args:
        blueprint_name: Name of the target Blueprint
        variable_name: Name of the variable to modify
        value: New default value for the variable. Type must match the variable type:
            - For bool: True/False
            - For int/float: numeric values
            - For string: text values
            - For arrays: list of values
            - For structs: dictionary with field names as keys

    Returns:
        Dictionary containing success status

    Examples:
        # Set a float variable
        set_blueprint_variable_value(
            blueprint_name="BP_DialogueNPC",
            variable_name="InteractionRadius",
            value=500.0
        )

        # Set a string variable
        set_blueprint_variable_value(
            blueprint_name="BP_Character",
            variable_name="CharacterName",
            value="Hero"
        )

        # Set a boolean variable
        set_blueprint_variable_value(
            blueprint_name="BP_Door",
            variable_name="bIsLocked",
            value=True
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "property_name": variable_name,
        "property_value": value
    }

    return await send_tcp_command("set_blueprint_property", params)


@app.tool()
async def get_blueprint_metadata(
    blueprint_name: str,
    fields: Optional[List[str]] = None,
    graph_name: str = None,
    node_type: str = None,
    event_type: str = None,
    detail_level: str = None
) -> Dict[str, Any]:
    """
    Get comprehensive metadata about a Blueprint with selective field querying.

    This unified command replaces multiple separate commands:
    - list_blueprint_components (use fields=["components"])
    - get parent class info (use fields=["parent_class"])
    - get Blueprint variables (use fields=["variables"])
    - get Blueprint functions (use fields=["functions"])
    - find_blueprint_nodes (use fields=["graph_nodes"] with node_type/event_type filters)

    Args:
        blueprint_name: Name or path of the Blueprint
        fields: List of metadata fields to retrieve. If None or ["*"], returns all.
                Options:
                - "parent_class": Parent class name and path
                - "interfaces": Implemented interfaces and their functions
                - "variables": Blueprint variables with types and default values
                - "functions": Custom Blueprint functions
                - "components": All components with names and types (replaces list_blueprint_components)
                - "graphs": Event graphs and function graphs
                - "status": Compilation status and error state
                - "metadata": Asset metadata and tags
                - "timelines": Timeline components
                - "asset_info": Asset path, size, and modification date
                - "orphaned_nodes": Detects disconnected nodes in all graphs
                - "graph_nodes": Detailed node info with pin connections and default values
        graph_name: Optional graph name filter for "graph_nodes" field.
                   When specified, only returns nodes from that specific graph.
        node_type: Optional node type filter for "graph_nodes" field.
                  Options: "Event", "Function", "Variable", "Comment", or any class name.
        event_type: Optional event type filter for "graph_nodes" field.
                   Options: "BeginPlay", "Tick", "EndPlay", "Destroyed", "Construct",
                   or any custom event name.
        detail_level: Optional detail level for "graph_nodes" field. Options:
                     - "summary": Node IDs and titles only (minimal output)
                     - "flow": Node IDs, titles, and exec pin connections only (DEFAULT)
                     - "full": Everything including all data pin connections and default values

    Returns:
        Dictionary containing requested metadata fields

    Examples:
        # Get all metadata
        get_blueprint_metadata(blueprint_name="BP_MyActor")

        # Get only components (replaces list_blueprint_components)
        get_blueprint_metadata(
            blueprint_name="BP_MyActor",
            fields=["components"]
        )

        # Get only parent class and interfaces
        get_blueprint_metadata(
            blueprint_name="BP_MyActor",
            fields=["parent_class", "interfaces"]
        )

        # Find all Event nodes (replaces find_blueprint_nodes)
        get_blueprint_metadata(
            blueprint_name="BP_MyActor",
            fields=["graph_nodes"],
            node_type="Event"
        )

        # Find BeginPlay event node
        get_blueprint_metadata(
            blueprint_name="BP_MyActor",
            fields=["graph_nodes"],
            node_type="Event",
            event_type="BeginPlay"
        )

        # Get minimal node info (just IDs and titles)
        get_blueprint_metadata(
            blueprint_name="BP_MyActor",
            fields=["graph_nodes"],
            graph_name="EventGraph",
            detail_level="summary"
        )

        # Get execution flow only (default)
        get_blueprint_metadata(
            blueprint_name="BP_MyActor",
            fields=["graph_nodes"],
            graph_name="EventGraph",
            detail_level="flow"
        )
    """
    params = {
        "blueprint_name": blueprint_name
    }

    if fields is not None:
        params["fields"] = fields

    if graph_name is not None:
        params["graph_name"] = graph_name

    if node_type is not None:
        params["node_type"] = node_type

    if event_type is not None:
        params["event_type"] = event_type

    if detail_level is not None:
        params["detail_level"] = detail_level

    return await send_tcp_command("get_blueprint_metadata", params)


@app.tool()
async def add_interface_to_blueprint(
    blueprint_name: str,
    interface_name: str
) -> Dict[str, Any]:
    """
    Add an interface to a Blueprint.

    Args:
        blueprint_name: Name of the target Blueprint (e.g., "BP_MyActor")
        interface_name: Name or path of the interface to add (e.g., "/Game/Blueprints/BPI_MyInterface")

    Returns:
        Response indicating success or failure

    Example:
        add_interface_to_blueprint(
            blueprint_name="BP_MyActor",
            interface_name="/Game/Blueprints/BPI_MyInterface"
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "interface_name": interface_name
    }

    return await send_tcp_command("add_interface_to_blueprint", params)


@app.tool()
async def create_blueprint_interface(
    name: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """
    Create a new Blueprint Interface asset.

    Args:
        name: Name of the interface (e.g., "BPI_Interactable")
        folder_path: Optional folder path for the interface (e.g., "/Game/Interfaces")

    Returns:
        Dictionary containing information about the created interface

    Example:
        create_blueprint_interface(
            name="BPI_Interactable",
            folder_path="/Game/Interfaces"
        )
    """
    params = {
        "name": name
    }
    if folder_path:
        params["folder_path"] = folder_path

    return await send_tcp_command("create_blueprint_interface", params)


if __name__ == "__main__":
    app.run()