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

- **add_widget_to_viewport(widget_name, z_order=0)**
  
  Add a Widget Blueprint instance to the viewport.
  
  Args:
    - widget_name: Name of the Widget Blueprint to add
    - z_order: Z-order for the widget (higher numbers appear on top)
  
  Returns: Dict containing success status and widget instance information
  
  Example:
    add_widget_to_viewport(widget_name="MainMenu")

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

- **check_widget_component_exists(widget_name, component_name)**
  
  Check if a component exists in the specified widget blueprint.
  
  Args:
    - widget_name: Name of the target Widget Blueprint
    - component_name: Name of the component to check
  
  Returns: Dict containing existence status of the component
  
  Example:
    check_widget_component_exists(widget_name="MyWidget", component_name="HeaderText")

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

- **get_widget_container_component_dimensions(widget_name, container_name="CanvasPanel_0")**
  
  Get the dimensions of a container widget in a UMG Widget Blueprint.
  
  Args:
    - widget_name: Name of the target Widget Blueprint
    - container_name: Name of the container widget (defaults to "CanvasPanel_0" for the root canvas panel)
  
  Returns: Dict containing the container dimensions and position
  
  Example:
    get_widget_container_component_dimensions(widget_name="MainMenu")

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
  
  Returns: Dict containing success status and property update info
  
  Example:
    set_widget_component_property("MyWidget", "MyTextBlock", Text="Red Text", ColorAndOpacity={"SpecifiedColor": {"R": 1.0, "G": 0.0, "B": 0.0, "A": 1.0}})

- **get_widget_component_layout(widget_name)**
  
  Get hierarchical layout information for all components within a UMG Widget Blueprint.
  
  Args:
    - widget_name: Name of the target Widget Blueprint (e.g., "WBP_MainMenu", "/Game/UI/MyWidget")
  
  Returns: Dict containing:
    - success (bool): True if the operation succeeded.
    - message (str): Status message.
    - hierarchy (dict): Root component with the following structure:
        - name (str): Component name.
        - type (str): Component class name (e.g., "TextBlock", "Button").
        - slot_properties (dict): Layout properties based on the slot type.
        - children (list[dict]): List of child components with the same structure.
  
  Example:
    layout = get_widget_component_layout(widget_name="WBP_PricingPage")
    if layout.get("success"):
        hierarchy = layout.get("hierarchy", {})
        print(f"Root component: {hierarchy.get('name')} ({hierarchy.get('type')})")

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