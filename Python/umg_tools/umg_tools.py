"""
UMG Tools for Unreal MCP.

This module provides tools for creating and manipulating UMG Widget Blueprints in Unreal Engine.
"""

import logging
from typing import Any, Dict, List, Union
from mcp.server.fastmcp import FastMCP, Context
from utils.widgets.widget_components import (
    create_widget_blueprint as create_widget_blueprint_impl,
    bind_widget_component_event as bind_widget_component_event_impl,
    add_child_widget_component_to_parent as add_child_widget_component_to_parent_impl,
    set_widget_component_placement as set_widget_component_placement_impl,
    add_widget_component_to_widget as add_widget_component_to_widget_impl,
    set_widget_component_property as set_widget_component_property_impl,
    set_text_block_widget_component_binding as set_text_block_widget_component_binding_impl,
    get_widget_blueprint_metadata_impl,
    create_widget_input_handler as create_widget_input_handler_impl,
    remove_widget_function_graph as remove_widget_function_graph_impl,
    reorder_widget_children as reorder_widget_children_impl
)
from utils.widgets.widget_screenshot import (
    capture_widget_screenshot_impl
)

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_umg_tools(mcp: FastMCP):
    """Register UMG tools with the MCP server."""

    @mcp.tool()
    def create_umg_widget_blueprint(
        ctx: Context,
        widget_name: str,
        parent_class: str = "UserWidget",
        path: str = "/Game/Widgets"
    ) -> Dict[str, object]:
        """
        Create a new UMG Widget Blueprint.

        Args:
            widget_name: Name of the widget blueprint to create
            parent_class: Parent class for the widget (default: UserWidget)
            path: Content browser path where the widget should be created

        Example:
            create_umg_widget_blueprint(widget_name="MyWidget", path="/Game/UI/Widgets")
        """
        # Call aliased implementation
        return create_widget_blueprint_impl(ctx, widget_name, parent_class, path)

    @mcp.tool()
    def bind_widget_component_event(
        ctx: Context,
        widget_name: str,
        widget_component_name: str,
        event_name: str,
        function_name: str = ""
    ) -> Dict[str, object]:
        """
        Bind an event on a widget component to a function.

        Args:
            widget_name: Name of the target Widget Blueprint
            widget_component_name: Name of the widget component (button, etc.)
            event_name: Name of the event to bind (OnClicked, etc.)
            function_name: Name of the function to create/bind to (defaults to f"{widget_component_name}_{event_name}")

        Example:
            bind_widget_component_event(
                widget_name="LoginScreen",
                widget_component_name="LoginButton",
                event_name="OnClicked"
            )
        """
        # Call aliased implementation
        return bind_widget_component_event_impl(ctx, widget_name, widget_component_name, event_name, function_name)

    @mcp.tool()
    def set_text_block_widget_component_binding(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        binding_property: str,
        binding_type: str = "Text",
        variable_type: str = "Text"
    ) -> Dict[str, object]:
        """
        Set up a property binding for a Text Block widget.

        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name of the Text Block to bind
            binding_property: Name of the property to bind to
            binding_type: Type of binding (Text, Visibility, etc.)
            variable_type: Type of variable to create if it doesn't exist (Text, String, Int, Float, Bool)

        Example:
            set_text_block_widget_component_binding(
                widget_name="PlayerHUD",
                text_block_name="ScoreText",
                binding_property="CurrentScore"
            )
        """
        # Call aliased implementation
        return set_text_block_widget_component_binding_impl(ctx, widget_name, text_block_name, binding_property, binding_type, variable_type)

    @mcp.tool()
    def add_child_widget_component_to_parent(
        ctx: Context,
        widget_name: str,
        parent_component_name: str,
        child_component_name: str,
        create_parent_if_missing: bool = False,
        parent_component_type: str = "Border",
        parent_position: List[float] = [0.0, 0.0],
        parent_size: List[float] = [300.0, 200.0]
    ) -> Dict[str, object]:
        """
        Add a widget component as a child to another component.

        This function can:
        1. Add an existing component inside an existing component
        2. If the parent doesn't exist, optionally create it and add the child inside

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_component_name: Name of the parent component
            child_component_name: Name of the child component to add to the parent
            create_parent_if_missing: Whether to create the parent component if it doesn't exist
            parent_component_type: Type of parent component to create if needed (e.g., "Border", "VerticalBox")
            parent_position: [X, Y] position of the parent component if created
            parent_size: [Width, Height] of the parent component if created

        Example:
            add_child_widget_component_to_parent(
                widget_name="MyWidget",
                parent_component_name="ContentBorder",
                child_component_name="HeaderText"
            )
        """
        # Call aliased implementation
        return add_child_widget_component_to_parent_impl(
            ctx,
            widget_name,
            parent_component_name,
            child_component_name,
            create_parent_if_missing,
            parent_component_type,
            parent_position,
            parent_size
        )

    @mcp.tool()
    def set_widget_component_placement(
        ctx: Context,
        widget_name: str,
        component_name: str,
        position: List[float] = None,
        size: List[float] = None,
        alignment: List[float] = None
    ) -> Dict[str, object]:
        """
        Change the placement (position/size) of a widget component.

        Args:
            widget_name: Name of the target Widget Blueprint
            component_name: Name of the component to modify
            position: Optional [X, Y] new position in the canvas panel
            size: Optional [Width, Height] new size for the component
            alignment: Optional [X, Y] alignment values (0.0 to 1.0)

        Example:
            set_widget_component_placement(
                widget_name="MainMenu",
                component_name="TitleText",
                position=[350.0, 75.0]
            )
        """
        # Call aliased implementation
        return set_widget_component_placement_impl(ctx, widget_name, component_name, position, size, alignment)

    @mcp.tool()
    def add_widget_component_to_widget(
        ctx: Context,
        widget_name: str,
        component_name: str,
        component_type: str,
        position: List[float] = None,
        size: List[float] = None,
        **kwargs
    ) -> Dict[str, object]:
        """
        Unified function to add any type of widget component to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            component_name: Name to give the new component
            component_type: Type of component to add (e.g., "TextBlock", "Button", etc.)
            position: Optional [X, Y] position in the canvas panel
            size: Optional [Width, Height] of the component
            **kwargs: Additional parameters specific to the component type
                For Border components:
                - background_color/brush_color: [R, G, B, A] color values (0.0-1.0)
                  Note: To achieve transparent backgrounds, set the Alpha value (A) in the color array
                - opacity: Value between 0.0-1.0 setting the render opacity of the entire border
                  and its contents. This is separate from the brush color's alpha.
                - use_brush_transparency: Boolean (True/False) to enable the "Use Brush Transparency" option
                  Required for alpha transparency to work properly with rounded corners or other complex brushes
                - padding: [Left, Top, Right, Bottom] values

        Example:
            add_widget_component_to_widget(
                widget_name="MyWidget",
                component_name="HeaderText",
                component_type="TextBlock",
                position=[100, 50],
                size=[200, 50],
                text="Hello World",
                font_size=24
            )
        """
        # Call aliased implementation
        return add_widget_component_to_widget_impl(ctx, widget_name, component_name, component_type, position, size, **kwargs)

    @mcp.tool()
    def set_widget_component_property(
        ctx: Context,
        widget_name: str,
        component_name: str,
        **kwargs
    ) -> Dict[str, object]:
        """
        Set one or more properties on a specific component within a UMG Widget Blueprint.

        Parameters:
            widget_name: Name of the target Widget Blueprint
            component_name: Name of the component to modify
            kwargs: Properties to set (as keyword arguments or a dict)

        Examples:

            # Set the text and color of a TextBlock, including a struct property (ColorAndOpacity)
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
            # Simple properties can be passed directly; struct properties (like ColorAndOpacity) as dicts.

            # Set the brush color (including opacity) of a Border using a flat RGBA dictionary
            set_widget_component_property(
                ctx,
                "MyWidget",
                "MyBorder",
                BrushColor={
                    "R": 1.0,
                    "G": 1.0,
                    "B": 1.0,
                    "A": 0.3
                }
            )
        """
        logger.info(f"[DEBUG] TOOL ENTRY: set_widget_component_property called with widget_name={widget_name}, component_name={component_name}, kwargs={kwargs}")
        try:
            # Call aliased implementation
            return set_widget_component_property_impl(ctx, widget_name, component_name, **(kwargs if isinstance(kwargs, dict) else {"Text": kwargs}))
        except Exception as e:
            logger.error(f"[ERROR] Exception in set_widget_component_property: {e}")
            raise

    @mcp.tool()
    def get_widget_blueprint_metadata(
        ctx: Context,
        widget_name: str,
        fields: List[str] = None,
        container_name: str = "CanvasPanel_0"
    ) -> Dict[str, Any]:
        """
        Get comprehensive metadata about a Widget Blueprint.

        This is the consolidated metadata tool that replaces:
        - check_widget_component_exists → fields=["components"] then check component name
        - get_widget_component_layout → fields=["layout"]
        - get_widget_container_component_dimensions → fields=["dimensions"]

        Args:
            widget_name: Name of the target Widget Blueprint (e.g., "WBP_MainMenu", "/Game/UI/MyWidget")
            fields: List of fields to include. Options:
                - "components" - List all widget components with basic info (name, type, is_variable, is_visible)
                - "layout" - Full hierarchical layout with positions, sizes, anchors, slot properties
                - "dimensions" - Container dimensions (width, height)
                - "hierarchy" - Simplified parent/child widget tree (names and types only)
                - "bindings" - Property bindings for dynamic content
                - "events" - Bound events and delegates
                - "variables" - Blueprint variables
                - "functions" - Blueprint functions with inputs/outputs
                - "*" - Return all fields (default)
            container_name: Container name for dimensions field (default: "CanvasPanel_0")

        Returns:
            Dict with: success, widget_name, asset_path, parent_class, components, layout,
            dimensions, hierarchy, bindings, events, variables, functions

        Example:
            get_widget_blueprint_metadata(widget_name="WBP_MainMenu", fields=["components"])
        """
        return get_widget_blueprint_metadata_impl(ctx, widget_name, fields, container_name)

    @mcp.tool()
    def capture_widget_screenshot(
        ctx: Context,
        widget_name: str,
        width: int = 800,
        height: int = 600,
        format: str = "png"
    ) -> Dict[str, Any]:
        """
        Capture a screenshot of a UMG Widget Blueprint preview.

        This renders the widget to an image and returns it as base64-encoded data that AI can view.
        This is the BEST way for AI to understand widget layout visually.

        Args:
            widget_name: Name of the widget blueprint to capture
            width: Screenshot width in pixels (default: 800, range: 1-8192)
            height: Screenshot height in pixels (default: 600, range: 1-8192)
            format: Image format - "png" (default) or "jpg"

        Returns:
            Dict with: success, image_base64, width, height, format, image_size_bytes, message

        Example:
            result = capture_widget_screenshot(widget_name="WBP_MainMenu")
        """
        return capture_widget_screenshot_impl(ctx, widget_name, width, height, format)

    @mcp.tool()
    def create_widget_input_handler(
        ctx: Context,
        widget_name: str,
        input_type: str,
        input_event: str,
        trigger: str = "Pressed",
        handler_name: str = "",
        component_name: str = ""
    ) -> Dict[str, object]:
        """
        Create an input event handler in a Widget Blueprint.

        This creates handlers for input events that are NOT exposed as standard widget
        delegates (like OnClicked). Use this for handling:
        - Right mouse button clicks (for context menus)
        - Middle mouse button (for panning/special actions)
        - Keyboard shortcuts on focusable widgets
        - Touch gestures
        - Drag and drop operations
        - Focus changes

        The tool creates:
        1. A custom event function in the Widget Blueprint's event graph
        2. An override of the appropriate input handler (OnMouseButtonDown, OnKeyDown, etc.)
        3. Logic to detect the specific input and call your custom event

        Args:
            widget_name: Name of the target Widget Blueprint
            input_type: Type of input to handle:
                - "MouseButton" - Mouse button events (left, right, middle, thumb)
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
            trigger: When to trigger the handler (default: "Pressed"):
                - "Pressed" - On button/key press
                - "Released" - On button/key release
                - "DoubleClick" - On double click (mouse only)
            handler_name: Name of the custom event function to create.
                         If empty, auto-generates (e.g., "OnRightMouseButtonPressed")
            component_name: Optional widget component name for component-specific handling.
                           If empty, handles at widget blueprint level.

        Example:
            create_widget_input_handler(
                widget_name="WBP_InventorySlot",
                input_type="MouseButton",
                input_event="RightMouseButton",
                handler_name="OnSlotRightClicked"
            )
        """
        return create_widget_input_handler_impl(
            ctx, widget_name, input_type, input_event, trigger, handler_name, component_name
        )

    @mcp.tool()
    def remove_widget_function_graph(
        ctx: Context,
        widget_name: str,
        function_name: str
    ) -> Dict[str, object]:
        """
        Remove a function graph from a Widget Blueprint.

        Use this to clean up broken/corrupt function graphs that prevent compilation,
        remove unwanted override functions, or reset widget event handlers.

        Args:
            widget_name: Name of the target Widget Blueprint
            function_name: Name of the function graph to remove (e.g., "OnMouseButtonDown")

        Example:
            remove_widget_function_graph(
                widget_name="WBP_InventorySlot",
                function_name="OnMouseButtonDown"
            )
        """
        return remove_widget_function_graph_impl(ctx, widget_name, function_name)

    @mcp.tool()
    def reorder_widget_children(
        ctx: Context,
        widget_name: str,
        container_name: str,
        child_order: List[str]
    ) -> Dict[str, object]:
        """
        Reorder children within a container widget (HorizontalBox, VerticalBox, etc.).

        Use this when components are in wrong visual order within a container.
        Children will be arranged in the order specified in child_order list.

        Args:
            widget_name: Name of the target Widget Blueprint
            container_name: Name of the container component (e.g., "ContentBox", "ButtonsContainer")
            child_order: List of child component names in desired left-to-right (or top-to-bottom) order

        Example:
            reorder_widget_children(
                widget_name="WBP_InteractionIndicator",
                container_name="ContentBox",
                child_order=["KeyBadgeOuter", "PromptText"]
            )
        """
        return reorder_widget_children_impl(ctx, widget_name, container_name, child_order)

    logger.info("UMG tools registered successfully")

# Moved outside the function
logger.info("UMG tools module loaded")
