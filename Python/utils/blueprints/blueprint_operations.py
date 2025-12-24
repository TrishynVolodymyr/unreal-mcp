"""
Blueprint Operations for Unreal MCP.

This module provides utility functions for working with Blueprints in Unreal Engine.
"""

import logging
from typing import Dict, List, Any, Tuple
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def list_all_blueprints(ctx: Context) -> List[Dict[str, Any]]:
    """Implementation for getting a list of all blueprints in the project."""
    return send_unreal_command("list_all_blueprints", {})

def search_blueprints(ctx: Context, query: str) -> List[Dict[str, Any]]:
    """Implementation for searching blueprints matching a query string."""
    params = {"query": query}
    return send_unreal_command("search_blueprints", params)

def get_blueprint_details(ctx: Context, blueprint_path: str) -> Dict[str, Any]:
    """Implementation for getting detailed information about a specific blueprint."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("get_blueprint_details", params)

def create_blueprint_from_actor(
    ctx: Context,
    actor_name: str,
    blueprint_name: str,
    folder_path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """Implementation for creating a new blueprint from an existing actor."""
    params = {
        "actor_name": actor_name,
        "blueprint_name": blueprint_name,
        "folder_path": folder_path
    }
    return send_unreal_command("create_blueprint_from_actor", params)

def create_blank_blueprint(
    ctx: Context,
    blueprint_name: str,
    parent_class: str = "Actor",
    folder_path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """Implementation for creating a new blank blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "parent_class": parent_class,
        "folder_path": folder_path
    }
    return send_unreal_command("create_blank_blueprint", params)

def compile_blueprint(ctx: Context, blueprint_name: str) -> Dict[str, Any]:
    """Implementation for compiling a blueprint."""
    # The C++ side will handle finding the path from the name, 
    # assuming it expects the 'blueprint_name' key.
    params = {"blueprint_name": blueprint_name} 
    return send_unreal_command("compile_blueprint", params)

def save_blueprint(ctx: Context, blueprint_path: str) -> Dict[str, Any]:
    """Implementation for saving a blueprint to disk."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("save_blueprint", params)

def get_blueprint_variables(ctx: Context, blueprint_path: str) -> List[Dict[str, Any]]:
    """Implementation for getting a list of variables defined in a blueprint."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("get_blueprint_variables", params)

def add_blueprint_variable(
    ctx: Context,
    blueprint_name: str,
    var_name: str,
    var_type: str,
    default_value: Any = None,
    is_instance_editable: bool = True,
    is_blueprint_read_only: bool = False,
    category: str = "Default"
) -> Dict[str, Any]:
    """Implementation for adding a new variable to a blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "variable_name": var_name,
        "variable_type": var_type,
        "is_exposed": is_instance_editable,
        "is_blueprint_read_only": is_blueprint_read_only,
        "category": category
    }
    
    # Only include default value if provided
    if default_value is not None:
        params["default_value"] = default_value
    
    return send_unreal_command("add_blueprint_variable", params)

def get_blueprint_functions(ctx: Context, blueprint_path: str) -> List[Dict[str, Any]]:
    """Implementation for getting a list of functions defined in a blueprint."""
    params = {"blueprint_path": blueprint_path}
    return send_unreal_command("get_blueprint_functions", params)

def add_blueprint_function(
    ctx: Context,
    blueprint_path: str,
    function_name: str,
    inputs: List[Dict[str, Any]] = None,
    outputs: List[Dict[str, Any]] = None,
    pure: bool = False,
    static: bool = False,
    category: str = "Default"
) -> Dict[str, Any]:
    """Implementation for adding a new function to a blueprint."""
    if inputs is None:
        inputs = []
    if outputs is None:
        outputs = []
    
    params = {
        "blueprint_path": blueprint_path,
        "function_name": function_name,
        "inputs": inputs,
        "outputs": outputs,
        "pure": pure,
        "static": static,
        "category": category
    }
    
    return send_unreal_command("add_blueprint_function", params)

def connect_graph_nodes(
    ctx: Context,
    blueprint_path: str,
    function_name: str,
    source_node_name: str,
    source_pin_name: str,
    target_node_name: str,
    target_pin_name: str
) -> Dict[str, Any]:
    """Implementation for connecting two nodes in a blueprint graph."""
    params = {
        "blueprint_path": blueprint_path,
        "function_name": function_name,
        "source_node_name": source_node_name,
        "source_pin_name": source_pin_name,
        "target_node_name": target_node_name,
        "target_pin_name": target_pin_name
    }
    
    return send_unreal_command("connect_graph_nodes", params)

def create_blueprint(
    ctx: Context,
    name: str,
    parent_class: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """Implementation for creating a new Blueprint class."""
    params = {
        "name": name,
        "parent_class": parent_class
    }
    
    if folder_path:
        params["folder_path"] = folder_path
        
    return send_unreal_command("create_blueprint", params)

def add_component_to_blueprint(
    ctx: Context,
    blueprint_name: str,
    component_type: str,
    component_name: str,
    location: List[float] = None,
    rotation: List[float] = None,
    scale: List[float] = None,
    component_properties: Dict[str, Any] = None
) -> Dict[str, Any]:
    """Implementation for adding a component to a Blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "component_type": component_type,
        "component_name": component_name
    }
    
    if location is not None:
        params["location"] = location
        
    if rotation is not None:
        params["rotation"] = rotation
        
    if scale is not None:
        params["scale"] = scale
        
    if component_properties is not None:
        params["component_properties"] = component_properties
        
    return send_unreal_command("add_component_to_blueprint", params)

def set_static_mesh_properties(
    ctx: Context,
    blueprint_name: str,
    component_name: str,
    static_mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Implementation for setting static mesh properties on a StaticMeshComponent."""
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "static_mesh": static_mesh
    }
    
    return send_unreal_command("set_static_mesh_properties", params)

def set_component_property(
    ctx: Context,
    blueprint_name: str,
    component_name: str,
    **kwargs
) -> Dict[str, Any]:
    """
    Set one or more properties on a component in a Blueprint.

    Args:
        ctx: MCP context
        blueprint_name: Name of the target Blueprint
        component_name: Name of the component
        kwargs: Properties to set. You can pass properties as direct keyword arguments (recommended),
            or as a single dict using kwargs={...}. If double-wrapped (kwargs={'kwargs': {...}}),
            the function will automatically flatten it for convenience. This matches the widget property setter pattern.

    Returns:
        Response indicating success or failure for each property.

    Example:
        # Preferred usage (direct keyword arguments):
        set_component_property(ctx, "MyActor", "Mesh", StaticMesh="/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube", Mobility="Movable")

        # Also supported (dict):
        set_component_property(ctx, "MyActor", "Mesh", kwargs={"StaticMesh": "/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube", "Mobility": "Movable"})

    """
    # Debug: Log all incoming arguments
    logger.info(f"[DEBUG] set_component_property called with: blueprint_name={blueprint_name}, component_name={component_name}, kwargs={kwargs}")

    # Flatten if kwargs is double-wrapped (i.e., kwargs={'kwargs': {...}})
    if (
        len(kwargs) == 1 and
        'kwargs' in kwargs and
        isinstance(kwargs['kwargs'], dict)
    ):
        logger.info("[DEBUG] Flattening double-wrapped kwargs in set_component_property")
        kwargs = kwargs['kwargs']

    # Argument validation
    if not blueprint_name or not isinstance(blueprint_name, str):
        logger.error("[ERROR] 'blueprint_name' is required and must be a string.")
        raise ValueError("'blueprint_name' is required and must be a string.")
    if not component_name or not isinstance(component_name, str):
        logger.error("[ERROR] 'component_name' is required and must be a string.")
        raise ValueError("'component_name' is required and must be a string.")
    if not kwargs or not isinstance(kwargs, dict):
        logger.error("[ERROR] At least one property must be provided as a keyword argument.")
        raise ValueError("At least one property must be provided as a keyword argument.")

    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "kwargs": kwargs
    }
    logger.info(f"[DEBUG] Sending set_component_property params: {params}")
    return send_unreal_command("set_component_property", params)

def set_physics_properties(
    ctx: Context,
    blueprint_name: str,
    component_name: str,
    simulate_physics: bool = True,
    gravity_enabled: bool = True,
    mass: float = 1.0,
    linear_damping: float = 0.01,
    angular_damping: float = 0.0
) -> Dict[str, Any]:
    """Implementation for setting physics properties on a component."""
    params = {
        "blueprint_name": blueprint_name,
        "component_name": component_name,
        "simulate_physics": simulate_physics,
        "gravity_enabled": gravity_enabled,
        "mass": mass,
        "linear_damping": linear_damping,
        "angular_damping": angular_damping
    }
    
    return send_unreal_command("set_physics_properties", params)

def set_blueprint_property(
    ctx: Context,
    blueprint_name: str,
    property_name: str,
    property_value: Any
) -> Dict[str, Any]:
    """Implementation for setting a property on a Blueprint class default object."""
    params = {
        "blueprint_name": blueprint_name,
        "property_name": property_name,
        "property_value": property_value
    }
    
    return send_unreal_command("set_blueprint_property", params)

def set_pawn_properties(
    ctx: Context,
    blueprint_name: str,
    auto_possess_player: str = "",
    use_controller_rotation_yaw: bool = None,
    use_controller_rotation_pitch: bool = None,
    use_controller_rotation_roll: bool = None,
    can_be_damaged: bool = None
) -> Dict[str, Any]:
    """Implementation for setting common Pawn properties on a Blueprint."""
    params = {
        "blueprint_name": blueprint_name
    }
    
    if auto_possess_player:
        params["auto_possess_player"] = auto_possess_player
        
    if use_controller_rotation_yaw is not None:
        params["use_controller_rotation_yaw"] = use_controller_rotation_yaw
        
    if use_controller_rotation_pitch is not None:
        params["use_controller_rotation_pitch"] = use_controller_rotation_pitch
        
    if use_controller_rotation_roll is not None:
        params["use_controller_rotation_roll"] = use_controller_rotation_roll
        
    if can_be_damaged is not None:
        params["can_be_damaged"] = can_be_damaged
        
    return send_unreal_command("set_pawn_properties", params)

def add_interface_to_blueprint(
    ctx: Context,
    blueprint_name: str,
    interface_name: str
) -> Dict[str, Any]:
    """Implementation for adding an interface to a blueprint."""
    params = {
        "blueprint_name": blueprint_name,
        "interface_name": interface_name
    }
    return send_unreal_command("add_interface_to_blueprint", params)

def create_blueprint_interface(
    ctx: Context,
    name: str,
    folder_path: str = ""
) -> Dict[str, Any]:
    """Implementation for creating a Blueprint Interface asset."""
    params = {
        "name": name,
        "folder_path": folder_path
    }
    return send_unreal_command("create_blueprint_interface", params)

def create_custom_blueprint_function(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    inputs: List[Dict[str, str]] = None,
    outputs: List[Dict[str, str]] = None,
    is_pure: bool = False,
    is_const: bool = False,
    access_specifier: str = "Public",
    category: str = "Default"
) -> Dict[str, Any]:
    """Implementation for creating a custom user-defined function in a blueprint.
    
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
    """
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name,
        "is_pure": is_pure,
        "is_const": is_const,
        "access_specifier": access_specifier,
        "category": category
    }
    
    if inputs is not None:
        params["inputs"] = inputs
        
    if outputs is not None:
        params["outputs"] = outputs
        
    return send_unreal_command("create_custom_blueprint_function", params)

def call_blueprint_function(
    ctx: Context,
    target_name: str,
    function_name: str,
    string_params: List[str] = None
) -> Dict[str, Any]:
    """Implementation for calling a BlueprintCallable function by name.

    Args:
        target_name: Name of the target object to call the function on
        function_name: Name of the BlueprintCallable function to execute
        string_params: List of string parameters to pass to the function

    Returns:
        Dictionary containing the function call result and success status
    """
    params = {
        "target_name": target_name,
        "function_name": function_name,
        "string_params": string_params or []
    }

    return send_unreal_command("call_blueprint_function", params)

def get_blueprint_metadata(
    ctx: Context,
    blueprint_name: str,
    fields: List[str],
    graph_name: str = None,
    node_type: str = None,
    event_type: str = None
) -> Dict[str, Any]:
    """Implementation for getting comprehensive metadata about a Blueprint.

    IMPORTANT: At least one field must be specified. When using "graph_nodes",
    graph_name is REQUIRED to limit response size.

    Args:
        blueprint_name: Name or path of the Blueprint
        fields: REQUIRED list of metadata fields. At least one must be specified.
                Options: "parent_class", "interfaces", "variables", "functions",
                        "components", "graphs", "status", "metadata",
                        "timelines", "asset_info", "orphaned_nodes", "graph_nodes"
        graph_name: REQUIRED when using "graph_nodes" field. Specifies which graph.
        node_type: Optional filter for "graph_nodes": "Event", "Function", "Variable", "Comment".
        event_type: Optional filter for "graph_nodes" when node_type="Event".

    Returns:
        Dictionary containing requested metadata fields
    """
    # Validate fields parameter
    if not fields or len(fields) == 0:
        return {
            "success": False,
            "error": "Missing required 'fields' parameter. Specify at least one field."
        }

    # Validate graph_name is provided when graph_nodes is requested
    if "graph_nodes" in fields and not graph_name:
        return {
            "success": False,
            "error": "When requesting 'graph_nodes' field, 'graph_name' parameter is required. Use fields=['graphs'] first to discover available graph names."
        }

    params = {
        "blueprint_name": blueprint_name,
        "fields": fields
    }

    if graph_name is not None:
        params["graph_name"] = graph_name

    if node_type is not None:
        params["node_type"] = node_type

    if event_type is not None:
        params["event_type"] = event_type

    return send_unreal_command("get_blueprint_metadata", params)


def modify_blueprint_function_properties(
    ctx: Context,
    blueprint_name: str,
    function_name: str,
    is_pure: bool = None,
    is_const: bool = None,
    access_specifier: str = None,
    category: str = None
) -> Dict[str, Any]:
    """Implementation for modifying properties of an existing Blueprint function.

    Args:
        blueprint_name: Name or path of the Blueprint
        function_name: Name of the function to modify
        is_pure: If set, changes whether the function is pure (no side effects)
        is_const: If set, changes whether the function is const
        access_specifier: If set, changes access level ("Public", "Protected", "Private")
        category: If set, changes the function's category for organization

    Returns:
        Dictionary containing success status and modified properties
    """
    params = {
        "blueprint_name": blueprint_name,
        "function_name": function_name
    }

    if is_pure is not None:
        params["is_pure"] = is_pure

    if is_const is not None:
        params["is_const"] = is_const

    if access_specifier is not None:
        params["access_specifier"] = access_specifier

    if category is not None:
        params["category"] = category

    return send_unreal_command("modify_blueprint_function_properties", params)


def capture_blueprint_graph_screenshot(
    ctx: Context,
    blueprint_name: str,
    graph_name: str = "EventGraph",
    width: int = 1280,
    height: int = 720,
    format: str = "png"
) -> Dict[str, Any]:
    """Implementation for capturing a screenshot of a Blueprint graph.

    This renders the Blueprint graph to an image and returns it as base64-encoded data.
    The image can be used by AI to visually understand the node layout without needing
    to query detailed node metadata, significantly reducing context/token usage.

    Args:
        blueprint_name: Name or path of the Blueprint to capture
        graph_name: Name of the graph to capture (default: "EventGraph").
                   Use get_blueprint_metadata with fields=["graphs"] to discover available graphs.
        width: Screenshot width in pixels (default: 1280, range: 1-8192)
        height: Screenshot height in pixels (default: 720, range: 1-8192)
        format: Image format - "png" (default) or "jpg"

    Returns:
        Dictionary containing:
        - success: Whether the screenshot was captured
        - image_base64: Base64-encoded image data (viewable by AI)
        - width: Actual image width
        - height: Actual image height
        - format: Image format used
        - image_size_bytes: Size of the compressed image
        - blueprint_name: Name of the Blueprint
        - graph_name: Name of the captured graph
        - node_count: Number of nodes in the graph

    Examples:
        # Capture EventGraph
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="BP_MyActor"
        )

        # Capture a specific function graph
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="BP_MyActor",
            graph_name="ToggleInventory"
        )

        # Capture at higher resolution
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="WBP_InventorySlot",
            graph_name="HandleMouseDown",
            width=1920,
            height=1080
        )

        # Capture as JPEG (smaller file size)
        result = capture_blueprint_graph_screenshot(
            ctx,
            blueprint_name="BP_DialogueComponent",
            graph_name="EventGraph",
            format="jpg"
        )
    """
    params = {
        "blueprint_name": blueprint_name,
        "graph_name": graph_name,
        "width": width,
        "height": height,
        "format": format
    }

    return send_unreal_command("capture_blueprint_graph_screenshot", params)