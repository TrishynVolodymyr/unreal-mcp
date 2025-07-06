"""
Widget Components for Unreal MCP.

This module provides utilities for working with UMG Widget Components in Unreal Engine.
"""

import logging
from typing import Dict, List, Any, Union
from mcp.server.fastmcp import Context
from utils.unreal_connection_utils import send_unreal_command

# Get logger
logger = logging.getLogger("UnrealMCP")

def create_widget_blueprint(
    ctx: Context,
    widget_name: str,
    parent_class: str = "UserWidget",
    path: str = "/Game/Widgets"
) -> Dict[str, Any]:
    """Implementation for creating a new UMG Widget Blueprint."""
    params = {
        "name": widget_name,
        "parent_class": parent_class,
        "path": path
    }
    
    logger.info(f"Creating widget blueprint: {widget_name} (Parent: {parent_class}, Path: {path})")
    return send_unreal_command("create_umg_widget_blueprint", params)

def bind_widget_component_event(
    ctx: Context,
    widget_name: str,
    widget_component_name: str,
    event_name: str,
    function_name: str = ""
) -> Dict[str, Any]:
    """Implementation for binding an event on a widget component to a function."""
    # If no function name provided, create one from component and event names
    if not function_name:
        function_name = f"{widget_component_name}_{event_name}"
    
    params = {
        "blueprint_name": widget_name,   # Name of the Widget Blueprint asset
        "widget_component_name": widget_component_name,  
        "event_name": event_name,
        "function_name": function_name
    }
    
    # Use local variable to avoid shadowing
    func_name = function_name
    logger.info(f"Binding event '{event_name}' on component '{widget_component_name}' in widget '{widget_name}' to function '{func_name}'")
    return send_unreal_command("bind_widget_component_event", params)

def add_widget_to_viewport(
    ctx: Context,
    widget_name: str,
    z_order: int = 0
) -> Dict[str, Any]:
    """Implementation for adding a Widget Blueprint instance to the viewport."""
    params = {
        "blueprint_name": widget_name,  # Name of the Widget Blueprint asset
        "z_order": z_order
    }
    
    logger.info(f"Adding widget '{widget_name}' to viewport (Z-Order: {z_order})")
    return send_unreal_command("add_widget_to_viewport", params)

def set_text_block_widget_component_binding(
    ctx: Context,
    widget_name: str,
    text_block_name: str,
    binding_property: str,
    binding_type: str = "Text"
) -> Dict[str, Any]:
    """Implementation for setting up a property binding for a Text Block widget."""
    params = {
        "blueprint_name": widget_name,      # Name of the Widget Blueprint asset
        "widget_name": text_block_name,     # Name of the Text Block widget in the blueprint
        "binding_name": binding_property,   # Name of the property to bind to
        "binding_type": binding_type
    }
    
    logger.info(f"Setting text binding for '{text_block_name}' in widget '{widget_name}' to property '{binding_property}' (Type: {binding_type})")
    return send_unreal_command("set_text_block_widget_component_binding", params)

def add_child_widget_component_to_parent(
    ctx: Context,
    widget_name: str,
    parent_component_name: str,
    child_component_name: str,
    create_parent_if_missing: bool = False,
    parent_component_type: str = "Border",
    parent_position: List[float] = [0.0, 0.0],
    parent_size: List[float] = [300.0, 200.0]
) -> Dict[str, Any]:
    """Implementation for adding a widget component as child to another component."""
    params = {
        "blueprint_name": widget_name,
        "parent_component_name": parent_component_name,
        "child_component_name": child_component_name,
        "create_parent_if_missing": create_parent_if_missing,
        "parent_component_type": parent_component_type,
        "parent_position": parent_position,
        "parent_size": parent_size
    }
    
    logger.info(f"Adding child '{child_component_name}' to parent '{parent_component_name}' in widget '{widget_name}'")
    return send_unreal_command("add_child_widget_component_to_parent", params)

def create_parent_and_child_widget_components(
    ctx: Context,
    widget_name: str,
    parent_component_name: str,
    child_component_name: str,
    parent_component_type: str = "Border",
    child_component_type: str = "TextBlock",
    parent_position: List[float] = [0.0, 0.0],
    parent_size: List[float] = [300.0, 200.0],
    child_attributes: Dict[str, Any] = None
) -> Dict[str, Any]:
    """Implementation for creating a new parent widget component with a new child component."""
    if child_attributes is None:
        child_attributes = {}
        
    params = {
        "blueprint_name": widget_name,
        "parent_component_name": parent_component_name,
        "child_component_name": child_component_name,
        "parent_component_type": parent_component_type,
        "child_component_type": child_component_type,
        "parent_position": parent_position,
        "parent_size": parent_size,
        "child_attributes": child_attributes or {}
    }
    
    logger.info(f"Creating parent '{parent_component_name}' ({parent_component_type}) with child '{child_component_name}' ({child_component_type}) in widget '{widget_name}'")
    return send_unreal_command("create_parent_and_child_widget_components", params)

def check_widget_component_exists(
    ctx: Context,
    widget_name: str,
    component_name: str
) -> Dict[str, Any]:
    """Implementation for checking if a component exists in a widget blueprint."""
    params = {
        "blueprint_name": widget_name,
        "component_name": component_name
    }
    
    logger.info(f"Checking if component '{component_name}' exists in widget '{widget_name}'")
    return send_unreal_command("check_widget_component_exists", params)

def set_widget_component_placement(
    ctx: Context,
    widget_name: str,
    component_name: str,
    position: List[float] = None,
    size: List[float] = None,
    alignment: List[float] = None
) -> Dict[str, Any]:
    """Implementation for changing the placement (position/size) of a widget component.
    
    Args:
        ctx: The current context
        widget_name: Name of the target Widget Blueprint
        component_name: Name of the component to modify
        position: Optional [X, Y] new position in the canvas panel
        size: Optional [Width, Height] new size for the component
        alignment: Optional [X, Y] alignment values (0.0 to 1.0)
        
    Returns:
        Dict containing success status and updated placement information
    """
    params = {
        "widget_name": widget_name,
        "component_name": component_name
    }
    
    # Only add parameters that are actually provided
    if position is not None:
        params["position"] = position
    
    if size is not None:
        params["size"] = size
        
    if alignment is not None:
        params["alignment"] = alignment
    
    logger.info(f"Setting placement for component '{component_name}' in widget '{widget_name}': Pos={position}, Size={size}, Align={alignment}")
    return send_unreal_command("set_widget_component_placement", params)

def get_widget_container_component_dimensions(
    ctx: Context,
    widget_name: str,
    container_name: str = "CanvasPanel_0"
) -> Dict[str, Any]:
    """Implementation for getting the dimensions of a container widget in a UMG Widget Blueprint.
    
    Args:
        ctx: The current context
        widget_name: Name of the target Widget Blueprint
        container_name: Name of the container widget (defaults to "RootCanvas" for the root canvas panel)
        
    Returns:
        Dict containing the container dimensions (width, height) and its position
    """
    params = {
        "widget_name": widget_name,
        "container_name": container_name
    }
    
    logger.info(f"Getting dimensions for container '{container_name}' in widget '{widget_name}'")
    return send_unreal_command("get_widget_container_component_dimensions", params)

def add_widget_component_to_widget(
    ctx: Context,
    widget_name: str,
    component_name: str,
    component_type: str,
    position: List[float] = None,
    size: List[float] = None,
    **kwargs
) -> Dict[str, Any]:
    """Implementation for the unified widget component creation function.
    
    This function can create any supported UMG widget component type.
    
    Args:
        ctx: The MCP context
        widget_name: Name of the target Widget Blueprint
        component_name: Name to give the new component
        component_type: Type of component to add (e.g., "TextBlock", "Button", etc.)
        position: Optional [X, Y] position in the canvas panel
        size: Optional [Width, Height] of the component
        **kwargs: Additional parameters specific to the component type
            For Border components:
            - background_color/brush_color: [R, G, B, A] color values (0.0-1.0)
              To achieve transparency, set the Alpha value (A) in the color array
            - opacity: Sets the overall render opacity of the entire border
              Note: This property is separate from the brush color's alpha value
            - use_brush_transparency: Boolean (True/False) to enable the "Use Brush Transparency" option
              This is required for alpha transparency to work properly with rounded corners or other complex brushes
            - padding: [Left, Top, Right, Bottom] values
        
    Returns:
        Dict containing success status and component properties
    """
    params = {
        "blueprint_name": widget_name,
        "component_name": component_name,
        "component_type": component_type,
        "kwargs": kwargs  # Pass all additional parameters
    }
    
    if position is not None:
        params["position"] = position
        
    if size is not None:
        params["size"] = size
    
    params.update(kwargs) # Add dynamic kwargs to params
    logger.info(f"Adding widget component '{component_name}' ({component_type}) to widget '{widget_name}'...")
    return send_unreal_command("add_widget_component_to_widget", params)

def set_widget_component_property(ctx: Context, widget_name: str, component_name: str, **kwargs):
    """
    Implementation for setting one or more properties on a widget component.
    Pass properties as keyword arguments (e.g., Text="Hello").

    Example (simple property):
        set_widget_component_property(ctx, "MyWidget", "MyTextBlock", Text="Hello World")

    Example (struct property - FSlateColor):
        set_widget_component_property(
            ctx,
            "MyWidget",
            "MyTextBlock",
            Text="Red Text",
            ColorAndOpacity={
                "SpecifiedColor": {
                    "R": 1.0,
                    "G": 0.0,
                    "B": 0.0,
                    "A": 1.0
                }
            }
        )

    The struct property (e.g., FSlateColor) must be passed as a dict matching Unreal's expected JSON structure.
    """
    # Debug: Log all incoming arguments
    logger.info(f"[DEBUG] set_widget_component_property called with: widget_name={widget_name}, component_name={component_name}, kwargs={kwargs}")

    # Flatten if kwargs is double-wrapped (i.e., kwargs={'kwargs': {...}})
    if (
        len(kwargs) == 1 and
        'kwargs' in kwargs and
        isinstance(kwargs['kwargs'], dict)
    ):
        logger.info("[DEBUG] Flattening double-wrapped kwargs in set_widget_component_property")
        kwargs = kwargs['kwargs']

    # Argument validation
    if not widget_name or not isinstance(widget_name, str):
        logger.error("[ERROR] 'widget_name' is required and must be a string.")
        raise ValueError("'widget_name' is required and must be a string.")
    if not component_name or not isinstance(component_name, str):
        logger.error("[ERROR] 'component_name' is required and must be a string.")
        raise ValueError("'component_name' is required and must be a string.")
    if not kwargs or not isinstance(kwargs, dict):
        logger.error("[ERROR] At least one property must be provided as a keyword argument.")
        raise ValueError("At least one property must be provided as a keyword argument.")

    params = {
        "widget_name": widget_name,
        "component_name": component_name,
        "kwargs": kwargs
    }
    logger.info(f"[DEBUG] Sending set_widget_component_property params: {params}")
    return send_unreal_command("set_widget_component_property", params)

def get_widget_component_layout_impl(ctx: Context, widget_name: str) -> dict:
    """Implementation for getting layout information for all components within a UMG Widget Blueprint."""
    command = "get_widget_component_layout"
    params = {"widget_name": widget_name}
    
    logger.info(f"Getting component layout for widget: {widget_name}")
    
    # Just prepare params and call send_unreal_command, returning its result directly.
    # Response parsing and detailed error handling will be done in the tool function.
    return send_unreal_command(command, params)