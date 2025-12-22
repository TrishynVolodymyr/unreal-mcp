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
    create_parent_and_child_widget_components as create_parent_and_child_widget_components_impl,
    set_widget_component_placement as set_widget_component_placement_impl,
    add_widget_component_to_widget as add_widget_component_to_widget_impl,
    set_widget_component_property as set_widget_component_property_impl,
    set_text_block_widget_component_binding as set_text_block_widget_component_binding_impl,
    get_widget_blueprint_metadata_impl,
    create_widget_input_handler as create_widget_input_handler_impl,
    remove_widget_function_graph as remove_widget_function_graph_impl
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
            
        Returns:
            Dict containing success status and widget path
            
        Examples:
            # Create a basic UserWidget blueprint
            create_umg_widget_blueprint(widget_name="MyWidget")
            
            # Create a widget with custom parent class
            create_umg_widget_blueprint(widget_name="MyCustomWidget", parent_class="MyBaseUserWidget")
            
            # Create a widget in a custom folder
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
            
        Returns:
            Dict containing success status and binding information
            
        Examples:
            # Bind button click event
            bind_widget_component_event(
                widget_name="LoginScreen",
                widget_component_name="LoginButton",
                event_name="OnClicked"
            )
            
            # Bind with custom function name
            bind_widget_component_event(
                widget_name="MainMenu",
                widget_component_name="QuitButton",
                event_name="OnClicked",
                function_name="ExitApplication"
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
            
        Returns:
            Dict containing success status and binding information
            
        Examples:
            # Set basic text binding
            set_text_block_widget_component_binding(
                widget_name="PlayerHUD",
                text_block_name="ScoreText",
                binding_property="CurrentScore"
            )
            
            # Set binding with specific binding type and variable type
            set_text_block_widget_component_binding(
                widget_name="GameUI",
                text_block_name="TimerText",
                binding_property="RemainingTime",
                binding_type="Text",
                variable_type="Int"
            )
            
            # Set binding with custom variable type
            set_text_block_widget_component_binding(
                widget_name="ShopUI",
                text_block_name="PriceText",
                binding_property="ItemPrice",
                binding_type="Text",
                variable_type="Float"
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
            
        Returns:
            Dict containing success status and component relationship information
            
        Examples:
            # Add an existing text block inside an existing border
            add_child_widget_component_to_parent(
                widget_name="MyWidget",
                parent_component_name="ContentBorder",
                child_component_name="HeaderText"
            )
            
            # Add an existing button to a new vertical box (creates if missing)
            add_child_widget_component_to_parent(
                widget_name="GameMenu",
                parent_component_name="ButtonsContainer",
                child_component_name="StartButton",
                create_parent_if_missing=True,
                parent_component_type="VerticalBox",
                parent_position=[100.0, 100.0],
                parent_size=[300.0, 400.0]
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
    def create_parent_and_child_widget_components(
        ctx: Context,
        widget_name: str,
        parent_component_name: str,
        child_component_name: str,
        parent_component_type: str = "Border",
        child_component_type: str = "TextBlock",
        parent_position: List[float] = [0.0, 0.0],
        parent_size: List[float] = [300.0, 200.0],
        child_attributes: Dict[str, object] = None
    ) -> Dict[str, object]:
        """
        Create a new parent widget component with a new child component.
        
        This function creates both components from scratch:
        1. Creates the parent component
        2. Creates the child component
        3. Adds the child inside the parent
        Note: This function creates exactly one parent and one child component.
              It cannot be used to create multiple nested levels in a single call.

        Args:
            widget_name: Name of the target Widget Blueprint
            parent_component_name: Name for the new parent component
            child_component_name: Name for the new child component
            parent_component_type: Type of parent component to create (e.g., "Border", "VerticalBox")
            child_component_type: Type of child component to create (e.g., "TextBlock", "Button")
            parent_position: [X, Y] position of the parent component
            parent_size: [Width, Height] of the parent component
            child_attributes: Additional attributes for the child component (content, colors, etc.)
            
        Returns:
            Dict containing success status and component creation information
            
        Examples:
            # Create a border with a text block inside
            create_parent_and_child_widget_components(
                widget_name="MyWidget",
                parent_component_name="HeaderBorder",
                child_component_name="TitleText",
                parent_component_type="Border",
                child_component_type="TextBlock",
                parent_position=[50.0, 50.0],
                parent_size=[400.0, 100.0],
                child_attributes={"text": "Welcome to My Game", "font_size": 24}
            )
            
            # Create a scroll box with a vertical box inside
            create_parent_and_child_widget_components(
                widget_name="InventoryScreen",
                parent_component_name="ItemsScrollBox",
                child_component_name="ItemsContainer",
                parent_component_type="ScrollBox",
                child_component_type="VerticalBox",
                parent_position=[100.0, 200.0],
                parent_size=[300.0, 400.0]
            )
        """
        # Call aliased implementation
        return create_parent_and_child_widget_components_impl(
            ctx, 
            widget_name, 
            parent_component_name, 
            child_component_name, 
            parent_component_type, 
            child_component_type, 
            parent_position, 
            parent_size,
            child_attributes
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
            
        Returns:
            Dict containing success status and updated placement information
            
        Examples:
            # Change just the position of a component
            set_widget_component_placement(
                widget_name="MainMenu",
                component_name="TitleText",
                position=[350.0, 75.0]
            )
            
            # Change both position and size
            set_widget_component_placement(
                widget_name="HUD",
                component_name="HealthBar",
                position=[50.0, 25.0],
                size=[250.0, 30.0]
            )
            
            # Change alignment (center-align)
            set_widget_component_placement(
                widget_name="Inventory",
                component_name="ItemName",
                alignment=[0.5, 0.5]
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
            
        Returns:
            Dict containing success status and component properties
            
        Examples:
            # Add a text block
            add_widget_component_to_widget(
                widget_name="MyWidget",
                component_name="HeaderText",
                component_type="TextBlock",
                position=[100, 50],
                size=[200, 50],
                text="Hello World",
                font_size=24
            )
            
            # Add a button
            add_widget_component_to_widget(
                widget_name="MyWidget",
                component_name="SubmitButton",
                component_type="Button",
                position=[100, 100],
                size=[200, 50],
                text="Submit",
                background_color=[0.2, 0.4, 0.8, 1.0]
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
            fields: List of fields to include. If not specified, returns all fields. Options:
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
            Dict containing:
                - success (bool): True if the operation succeeded
                - widget_name (str): Name of the widget blueprint
                - asset_path (str): Full asset path
                - parent_class (str): Parent class name
                - components (dict): Component list with count (if requested)
                - layout (dict): Hierarchical layout info (if requested)
                - dimensions (dict): Container dimensions (if requested)
                - hierarchy (dict): Simple widget tree (if requested)
                - bindings (dict): Property bindings (if requested)
                - events (dict): Event bindings (if requested)
                - variables (dict): Blueprint variables (if requested)
                - functions (dict): Blueprint functions (if requested)
                - error (str): Error message if failed

        Examples:
            # Get all metadata
            metadata = get_widget_blueprint_metadata(widget_name="WBP_MainMenu")

            # Check if a component exists (replaces check_widget_component_exists)
            metadata = get_widget_blueprint_metadata(
                widget_name="WBP_MainMenu",
                fields=["components"]
            )
            exists = any(c["name"] == "HeaderText" for c in metadata.get("components", {}).get("components", []))

            # Get layout info (replaces get_widget_component_layout)
            metadata = get_widget_blueprint_metadata(
                widget_name="WBP_MainMenu",
                fields=["layout"]
            )
            layout = metadata.get("layout", {})

            # Get container dimensions (replaces get_widget_container_component_dimensions)
            metadata = get_widget_blueprint_metadata(
                widget_name="WBP_MainMenu",
                fields=["dimensions"],
                container_name="CanvasPanel_0"
            )
            width = metadata.get("dimensions", {}).get("width", 1920)
            height = metadata.get("dimensions", {}).get("height", 1080)

            # Get multiple specific fields
            metadata = get_widget_blueprint_metadata(
                widget_name="WBP_MainMenu",
                fields=["components", "variables", "bindings"]
            )
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
            Dict containing:
                - success: Whether the screenshot was captured
                - image_base64: Base64-encoded image data (viewable by AI)
                - width: Actual image width
                - height: Actual image height
                - format: Image format used
                - image_size_bytes: Size of the compressed image
                - message: Success or error message

        Examples:
            # Capture a widget screenshot
            result = capture_widget_screenshot(widget_name="WBP_MainMenu")
            if result["success"]:
                print(f"Screenshot captured: {result['width']}x{result['height']}, "
                      f"{result['image_size_bytes']} bytes")
                # AI can now view the widget layout visually

            # Capture at higher resolution
            result = capture_widget_screenshot(
                widget_name="WBP_LoginScreen",
                width=1920,
                height=1080
            )

            # Capture as JPEG (smaller file size)
            result = capture_widget_screenshot(
                widget_name="WBP_Settings",
                width=1024,
                height=768,
                format="jpg"
            )
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

        Returns:
            Dict containing:
                - success: Whether the handler was created
                - handler_name: The actual name of the created handler function
                - input_type: The input type configured
                - input_event: The specific event that triggers the handler
                - message: Description of what was created

        Examples:
            # Create right-click handler for inventory context menu
            create_widget_input_handler(
                widget_name="WBP_InventorySlot",
                input_type="MouseButton",
                input_event="RightMouseButton",
                handler_name="OnSlotRightClicked"
            )

            # Create Escape key handler to close a menu
            create_widget_input_handler(
                widget_name="WBP_PauseMenu",
                input_type="Key",
                input_event="Escape",
                handler_name="OnCloseMenu"
            )

            # Create middle mouse button handler for map panning
            create_widget_input_handler(
                widget_name="WBP_WorldMap",
                input_type="MouseButton",
                input_event="MiddleMouseButton",
                trigger="Pressed",
                handler_name="OnStartPan"
            )

            # Create double-click handler
            create_widget_input_handler(
                widget_name="WBP_ItemList",
                input_type="MouseButton",
                input_event="LeftMouseButton",
                trigger="DoubleClick",
                handler_name="OnItemDoubleClicked"
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

        Returns:
            Dict containing success status and removal information

        Examples:
            # Remove a broken OnMouseButtonDown override that prevents compilation
            remove_widget_function_graph(
                widget_name="WBP_InventorySlot",
                function_name="OnMouseButtonDown"
            )

            # Remove a custom event handler
            remove_widget_function_graph(
                widget_name="WBP_MainMenu",
                function_name="OnRightClickHandler"
            )

            # Clean up an override function to start fresh
            remove_widget_function_graph(
                widget_name="WBP_InteractiveWidget",
                function_name="OnKeyDown"
            )
        """
        return remove_widget_function_graph_impl(ctx, widget_name, function_name)

    logger.info("UMG tools registered successfully")

# Moved outside the function
logger.info("UMG tools module loaded")