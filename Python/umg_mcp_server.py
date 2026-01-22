"""
UMG MCP Server

Exposes UMG (Widget Blueprint) tools for Unreal Engine via MCP.

## UMG (Widget Blueprint) Tools

- **create_umg_widget_blueprint(widget_name, parent_class="UserWidget", path="/Game/Widgets")**

  Create a new UMG Widget Blueprint.

  Args:
    - widget_name: Name of the widget blueprint to create
    - parent_class: Parent class for the widget (default: UserWidget)
    - path: Content browser path where the widget should be created

  Returns: Dict containing success status and widget path

  Example:
    create_umg_widget_blueprint(widget_name="MyWidget")

- **bind_widget_component_event(widget_name, widget_component_name, event_name, function_name="")**

  Bind an event on a widget component to a function.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - widget_component_name: Name of the widget component (button, etc.)
    - event_name: Name of the event to bind (OnClicked, etc.)
    - function_name: Name of the function to create/bind to (optional)

  Returns: Dict containing success status and binding information

  Example:
    bind_widget_component_event(widget_name="LoginScreen", widget_component_name="LoginButton", event_name="OnClicked")

- **set_text_block_widget_component_binding(widget_name, text_block_name, binding_property, binding_type="Text")**

  Set up a property binding for a Text Block widget.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - text_block_name: Name of the Text Block to bind
    - binding_property: Name of the property to bind to
    - binding_type: Type of binding (Text, Visibility, etc.)

  Returns: Dict containing success status and binding information

  Example:
    set_text_block_widget_component_binding(widget_name="PlayerHUD", text_block_name="ScoreText", binding_property="CurrentScore")

- **add_child_widget_component_to_parent(widget_name, parent_component_name, child_component_name, create_parent_if_missing=False, parent_component_type="Border", parent_position=[0.0, 0.0], parent_size=[300.0, 200.0])**

  Add a widget component as a child to another component. Can create the parent if missing.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - parent_component_name: Name of the parent component
    - child_component_name: Name of the child component to add to the parent
    - create_parent_if_missing: Whether to create the parent component if it doesn't exist
    - parent_component_type: Type of parent component to create if needed (e.g., "Border", "VerticalBox")
    - parent_position: [X, Y] position of the parent component if created
    - parent_size: [Width, Height] of the parent component if created

  Returns: Dict containing success status and component relationship information

  Example:
    add_child_widget_component_to_parent(widget_name="MyWidget", parent_component_name="ContentBorder", child_component_name="HeaderText")

- **create_parent_and_child_widget_components(widget_name, parent_component_name, child_component_name, parent_component_type="Border", child_component_type="TextBlock", parent_position=[0.0, 0.0], parent_size=[300.0, 200.0], child_attributes=None)**

  Create a new parent widget component with a new child component (one parent, one child).

  Args:
    - widget_name: Name of the target Widget Blueprint
    - parent_component_name: Name for the new parent component
    - child_component_name: Name for the new child component
    - parent_component_type: Type of parent component to create (e.g., "Border", "VerticalBox")
    - child_component_type: Type of child component to create (e.g., "TextBlock", "Button")
    - parent_position: [X, Y] position of the parent component
    - parent_size: [Width, Height] of the parent component
    - child_attributes: Additional attributes for the child component (content, colors, etc.)

  Returns: Dict containing success status and component creation information

  Example:
    create_parent_and_child_widget_components(widget_name="MyWidget", parent_component_name="HeaderBorder", child_component_name="TitleText", parent_component_type="Border", child_component_type="TextBlock", parent_position=[50.0, 50.0], parent_size=[400.0, 100.0], child_attributes={"text": "Welcome to My Game", "font_size": 24})

- **set_widget_component_placement(widget_name, component_name, position=None, size=None, alignment=None)**

  Change the placement (position/size) of a widget component.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - component_name: Name of the component to modify
    - position: Optional [X, Y] new position in the canvas panel
    - size: Optional [Width, Height] new size for the component
    - alignment: Optional [X, Y] alignment values (0.0 to 1.0)

  Returns: Dict containing success status and updated placement information

  Example:
    set_widget_component_placement(widget_name="MainMenu", component_name="TitleText", position=[350.0, 75.0])

- **add_widget_component_to_widget(widget_name, component_name, component_type, position=None, size=None, **kwargs)**

  Unified function to add any type of widget component to a UMG Widget Blueprint.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - component_name: Name to give the new component
    - component_type: Type of component to add (e.g., "TextBlock", "Button", etc.)
    - position: Optional [X, Y] position in the canvas panel
    - size: Optional [Width, Height] of the component
    - **kwargs: Additional parameters specific to the component type

  Returns: Dict containing success status and component properties

  Example:
    add_widget_component_to_widget(widget_name="MyWidget", component_name="HeaderText", component_type="TextBlock", position=[100, 50], size=[200, 50], text="Hello World", font_size=24)

- **set_widget_component_property(widget_name, component_name, **kwargs)**

  Set one or more properties on a specific component within a UMG Widget Blueprint.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - component_name: Name of the component to modify
    - **kwargs: Properties to set (as keyword arguments or a dict)
        For widget properties: Text, ColorAndOpacity, Font, etc.
        For slot properties: Use "Slot." prefix:

        HorizontalBox/VerticalBox children:
        - Slot.SizeRule: "Auto" or "Fill"
        - Slot.VerticalAlignment / Slot.VAlign: "Top", "Center", "Bottom", "Fill"
        - Slot.HorizontalAlignment / Slot.HAlign: "Left", "Center", "Right", "Fill"
        - Slot.Padding: [left, top, right, bottom] array or single number for uniform padding
        - Slot.SizeValue: Numeric value for fill ratio

        CanvasPanel children:
        - Slot.Anchors: {"Minimum": {"X": 0, "Y": 0}, "Maximum": {"X": 1, "Y": 1}}
        - Slot.Offsets: {"Left": 0, "Top": 0, "Right": 0, "Bottom": 0} or [left, top, right, bottom]
        - Slot.Position: [X, Y] position in canvas
        - Slot.Size: [Width, Height] size
        - Slot.Alignment: [X, Y] anchor point (0.0-1.0)
        - Slot.AutoSize: true/false for auto-sizing
        - Slot.ZOrder: Integer for rendering order

  Returns: Dict containing success status and property update info

  Examples:
    # Set widget properties
    set_widget_component_property("MyWidget", "MyTextBlock", Text="Red Text", ColorAndOpacity={"SpecifiedColor": {"R": 1.0, "G": 0.0, "B": 0.0, "A": 1.0}})

    # Set slot properties for HorizontalBox/VerticalBox children
    set_widget_component_property("WBP_AnswerButton", "NumberBadgeWrapper", **{"Slot.SizeRule": "Auto", "Slot.VerticalAlignment": "Center"})
    set_widget_component_property("WBP_AnswerButton", "AnswerText", **{"Slot.SizeRule": "Fill", "Slot.VerticalAlignment": "Center"})

    # Set slot properties for CanvasPanel children (stretch to fill parent)
    set_widget_component_property("WBP_AnswerButton", "MainButton", **{"Slot.Anchors": {"Minimum": {"X": 0, "Y": 0}, "Maximum": {"X": 1, "Y": 1}}, "Slot.Offsets": {"Left": 0, "Top": 0, "Right": 0, "Bottom": 0}})

- **get_widget_blueprint_metadata(widget_name, fields=None, container_name="CanvasPanel_0")**

  Get comprehensive metadata about a Widget Blueprint. This consolidated tool replaces
  check_widget_component_exists, get_widget_component_layout, and get_widget_container_component_dimensions.

  Args:
    - widget_name: Name of the target Widget Blueprint (e.g., "WBP_MainMenu", "/Game/UI/MyWidget")
    - fields: List of fields to include. Options: "components", "layout", "dimensions", "hierarchy",
              "bindings", "events", "variables", "functions", "*" (all). Default: all fields.
    - container_name: Container name for dimensions field (default: "CanvasPanel_0")

  Returns: Dict containing:
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

  Examples:
    # Get all metadata
    metadata = get_widget_blueprint_metadata(widget_name="WBP_MainMenu")

    # Check if a component exists (replaces check_widget_component_exists)
    metadata = get_widget_blueprint_metadata(widget_name="WBP_MainMenu", fields=["components"])
    exists = any(c["name"] == "HeaderText" for c in metadata.get("components", {}).get("components", []))

    # Get layout info (replaces get_widget_component_layout)
    metadata = get_widget_blueprint_metadata(widget_name="WBP_MainMenu", fields=["layout"])

    # Get container dimensions (replaces get_widget_container_component_dimensions)
    metadata = get_widget_blueprint_metadata(widget_name="WBP_MainMenu", fields=["dimensions"])

- **capture_widget_screenshot(widget_name, width=800, height=600, format="png")**

  Capture a screenshot of a UMG Widget Blueprint preview.

  Args:
    - widget_name: Name of the widget blueprint to capture
    - width: Screenshot width in pixels (default: 800)
    - height: Screenshot height in pixels (default: 600)
    - format: Image format - "png" (default) or "jpg"

  Returns: Dict containing:
    - success: Whether the screenshot was captured
    - image_base64: Base64-encoded image data (viewable by AI)
    - width, height: Actual image dimensions
    - format: Image format used
    - image_size_bytes: Size of the compressed image

  Example:
    result = capture_widget_screenshot(widget_name="WBP_MainMenu")

- **create_widget_input_handler(widget_name, input_type, input_event, trigger="Pressed", handler_name="", component_name="")**

  Create an input event handler in a Widget Blueprint for events not exposed as standard delegates.
  Use this for right-click context menus, keyboard shortcuts, touch gestures, etc.

  Args:
    - widget_name: Name of the target Widget Blueprint
    - input_type: Type of input - "MouseButton", "Key", "Touch", "Focus", "Drag"
    - input_event: Specific event:
        - MouseButton: "LeftMouseButton", "RightMouseButton", "MiddleMouseButton", etc.
        - Key: Any key name ("Enter", "Escape", "SpaceBar", "A", "F1", etc.)
        - Touch: "Touch", "Pinch", "Swipe"
        - Focus: "FocusReceived", "FocusLost"
        - Drag: "DragDetected", "DragEnter", "DragLeave", "DragOver", "Drop"
    - trigger: When to trigger - "Pressed", "Released", "DoubleClick"
    - handler_name: Name for the custom event (auto-generated if empty)
    - component_name: Optional component for component-specific handling

  Returns: Dict containing success status and handler information

  Examples:
    # Right-click for context menu
    create_widget_input_handler(widget_name="WBP_InventorySlot", input_type="MouseButton",
                                input_event="RightMouseButton", handler_name="OnSlotRightClicked")

    # Escape key to close menu
    create_widget_input_handler(widget_name="WBP_PauseMenu", input_type="Key",
                                input_event="Escape", handler_name="OnCloseMenu")

See the main server or tool docstrings for argument details and examples.
"""
from mcp.server.fastmcp import FastMCP
from umg_tools.umg_tools import register_umg_tools

mcp = FastMCP(
    "umgMCP",
    description="UMG (Widget) tools for Unreal via MCP"
)

register_umg_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')
