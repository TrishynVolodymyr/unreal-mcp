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


def set_text_block_widget_component_binding(
    ctx: Context,
    widget_name: str,
    text_block_name: str,
    binding_property: str,
    binding_type: str = "Text",
    variable_type: str = "Text"
) -> Dict[str, Any]:
    """Implementation for setting up a property binding for a Text Block widget."""
    params = {
        "blueprint_name": widget_name,      # Name of the Widget Blueprint asset
        "widget_name": text_block_name,     # Name of the Text Block widget in the blueprint
        "binding_name": binding_property,   # Name of the property to bind to
        "binding_type": binding_type,
        "variable_type": variable_type      # Type of the variable to create (Text, String, Int, Float, Bool)
    }
    
    logger.info(f"Setting text binding for '{text_block_name}' in widget '{widget_name}' to property '{binding_property}' (Type: {binding_type}, Variable Type: {variable_type})")
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

def create_widget_input_handler(
    ctx: Context,
    widget_name: str,
    input_type: str,
    input_event: str,
    trigger: str = "Pressed",
    handler_name: str = "",
    component_name: str = ""
) -> Dict[str, Any]:
    """Implementation for creating an input event handler in a Widget Blueprint.

    This creates handlers for input events not exposed as standard delegates,
    such as right mouse button clicks, keyboard events, touch events, etc.

    Args:
        ctx: The current context
        widget_name: Name of the target Widget Blueprint
        input_type: Type of input to handle:
            - "MouseButton" - Mouse button events
            - "Key" - Keyboard key events
            - "Touch" - Touch/gesture events
            - "Focus" - Widget focus events
            - "Drag" - Drag and drop events
        input_event: Specific input event to handle:
            - For MouseButton: "LeftMouseButton", "RightMouseButton", "MiddleMouseButton",
                               "ThumbMouseButton", "ThumbMouseButton2"
            - For Key: Any key name (e.g., "Enter", "Escape", "SpaceBar", "A", "F1", etc.)
            - For Touch: "Touch", "Pinch", "Swipe"
            - For Focus: "FocusReceived", "FocusLost"
            - For Drag: "DragDetected", "DragEnter", "DragLeave", "DragOver", "Drop"
        trigger: When to trigger the handler:
            - "Pressed" - On button/key press (default)
            - "Released" - On button/key release
            - "DoubleClick" - On double click (mouse only)
        handler_name: Name of the custom event function to create.
                     If empty, auto-generates based on input type and event.
        component_name: Optional widget component name. If specified, the handler
                       is associated with that component. If empty, handles at
                       the widget blueprint level.

    Returns:
        Dict containing:
            - success: Boolean indicating if handler was created
            - handler_name: The actual name of the created handler function
            - input_type: The input type that was configured
            - input_event: The specific event that triggers the handler
            - message: Description of what was created

    Examples:
        # Create a right-click handler for context menu
        create_widget_input_handler(
            ctx,
            widget_name="InventoryWidget",
            input_type="MouseButton",
            input_event="RightMouseButton",
            trigger="Pressed",
            handler_name="OnItemRightClicked"
        )

        # Create an Escape key handler
        create_widget_input_handler(
            ctx,
            widget_name="PauseMenu",
            input_type="Key",
            input_event="Escape",
            trigger="Pressed",
            handler_name="OnEscapePressed"
        )

        # Create a middle mouse button handler on a specific component
        create_widget_input_handler(
            ctx,
            widget_name="MapWidget",
            input_type="MouseButton",
            input_event="MiddleMouseButton",
            trigger="Pressed",
            handler_name="OnMapPan",
            component_name="MapImage"
        )
    """
    params = {
        "widget_name": widget_name,
        "input_type": input_type,
        "input_event": input_event,
        "trigger": trigger
    }

    if handler_name:
        params["handler_name"] = handler_name

    if component_name:
        params["component_name"] = component_name

    logger.info(f"Creating input handler for {input_type} {input_event} ({trigger}) in widget '{widget_name}'")
    return send_unreal_command("create_widget_input_handler", params)


def remove_widget_function_graph(
    ctx: Context,
    widget_name: str,
    function_name: str
) -> Dict[str, Any]:
    """Remove a function graph from a Widget Blueprint.

    This function removes function graphs (override functions, custom events, etc.)
    from Widget Blueprints. Useful for:
    - Cleaning up broken/corrupt function graphs that prevent compilation
    - Removing unwanted override functions
    - Resetting widget event handlers

    Args:
        ctx: The current context
        widget_name: Name of the target Widget Blueprint
        function_name: Name of the function graph to remove (e.g., "OnMouseButtonDown")

    Returns:
        Dict containing:
            - success: Boolean indicating if the function was removed
            - widget_name: The widget blueprint name
            - function_name: The function that was removed
            - message: Description of what happened

    Examples:
        # Remove a broken OnMouseButtonDown override
        remove_widget_function_graph(
            ctx,
            widget_name="WBP_InventorySlot",
            function_name="OnMouseButtonDown"
        )

        # Remove a custom event handler
        remove_widget_function_graph(
            ctx,
            widget_name="WBP_MainMenu",
            function_name="OnRightClickHandler"
        )
    """
    params = {
        "widget_name": widget_name,
        "function_name": function_name
    }

    logger.info(f"Removing function graph '{function_name}' from widget '{widget_name}'")
    return send_unreal_command("remove_widget_function_graph", params)


def get_widget_blueprint_metadata_impl(
    ctx: Context,
    widget_name: str,
    fields: List[str] = None,
    container_name: str = "CanvasPanel_0"
) -> Dict[str, Any]:
    """Implementation for getting comprehensive metadata about a Widget Blueprint.

    This is the consolidated metadata tool that replaces:
    - check_widget_component_exists (use fields=["components"])
    - get_widget_component_layout (use fields=["layout"])
    - get_widget_container_component_dimensions (use fields=["dimensions"])

    Args:
        ctx: The current context
        widget_name: Name of the target Widget Blueprint
        fields: List of fields to include. Options:
            - "components" - List all widget components
            - "layout" - Full hierarchical layout with positions/sizes
            - "dimensions" - Container dimensions
            - "hierarchy" - Parent/child widget tree
            - "bindings" - Property bindings
            - "events" - Bound events and delegates
            - "variables" - Blueprint variables
            - "functions" - Blueprint functions
            - "*" - Return all fields (default)
        container_name: Container name for dimensions field (default: "CanvasPanel_0")

    Returns:
        Dict containing the requested metadata fields
    """
    params = {
        "widget_name": widget_name
    }

    if fields is not None:
        params["fields"] = fields

    if container_name:
        params["container_name"] = container_name

    logger.info(f"Getting widget blueprint metadata for: {widget_name}, fields: {fields}")
    return send_unreal_command("get_widget_blueprint_metadata", params)