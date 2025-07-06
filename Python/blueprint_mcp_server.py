"""
Blueprint MCP Server

Exposes Blueprint-related tools for Unreal Engine via MCP.

## Blueprint Management

- **create_blueprint(name, parent_class, folder_path="")**
  
  Create a new Blueprint class.
  
  Args:
    - name (str): Name of the new Blueprint class (can include path with "/")
    - parent_class (str): Parent class for the Blueprint (e.g., "Actor", "Pawn")
    - folder_path (str): Optional folder path where the blueprint should be created
  
  Returns: Dictionary containing information about the created Blueprint including path and success status.
  
  Example:
    create_blueprint(name="MyBlueprint", parent_class="Actor")
    create_blueprint(name="System/Blueprints/my_bp", parent_class="Actor")
    create_blueprint(name="MyBlueprint", parent_class="Actor", folder_path="Success")

- **add_blueprint_variable(blueprint_name, variable_name, variable_type, is_exposed=False)**
  
  Add a variable to a Blueprint. Supports built-in, user-defined struct, and delegate types.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
    - variable_name (str): Name of the variable
    - variable_type (str): Type of the variable (Boolean, Integer, Float, Vector, StructName, StructName[], Delegate, etc.)
    - is_exposed (bool): Whether to expose the variable to the editor
  
  Returns: Response indicating success or failure.
  
  Example:
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="Score", variable_type="Integer", is_exposed=True)
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="Inventory", variable_type="String[]", is_exposed=True)
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="WBP_Widget", variable_type="Game/Widget/WBP_Widget", is_exposed=True)
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="MyStruct", variable_type="Game/DataStructures/MyStruct", is_exposed=True)
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="MyStructs", variable_type="Game/DataStructures/MyStruct[]", is_exposed=True)
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="Datatable", variable_type="Game/DataTables/MyDatatable", is_exposed=True)
    add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="BP_Manager", variable_type="Game/Blueprints/BP_Manager", is_exposed=True)

- **add_component_to_blueprint(blueprint_name, component_type, component_name, location=None, rotation=None, scale=None, component_properties=None)**
  
  Add a component to a Blueprint.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
    - component_type (str): Type of component to add (use component class name without U prefix)
    - component_name (str): Name for the new component
    - location (list): [X, Y, Z] coordinates for component's position
    - rotation (list): [Pitch, Yaw, Roll] values for component's rotation
    - scale (list): [X, Y, Z] values for component's scale
    - component_properties (dict): Additional properties to set on the component
  
  Returns: Information about the added component.

- **set_static_mesh_properties(blueprint_name, component_name, static_mesh="/Engine/BasicShapes/Cube.Cube")**
  
  Set static mesh properties on a StaticMeshComponent.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
    - component_name (str): Name of the StaticMeshComponent
    - static_mesh (str): Path to the static mesh asset
  
  Returns: Response indicating success or failure.

- **set_component_property(blueprint_name, component_name, **kwargs)**
  
  Set one or more properties on a specific component within a Blueprint.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
    - component_name (str): Name of the component to modify
    - kwargs: Properties to set as a JSON string. Must be formatted as a valid JSON string.
  
  Returns: Dict containing:
    - success: Boolean indicating overall success
    - success_properties: List of property names successfully set
    - failed_properties: List of property names that failed to set, with error messages
  
  Example:
    set_component_property(blueprint_name="MyBlueprint", component_name="PointLight1", kwargs='{"Intensity": 5000.0, "AttenuationRadius": 1000.0}')
    set_component_property(blueprint_name="MyBlueprint", component_name="Mesh1", kwargs='{"RelativeLocation": [100.0, 200.0, 50.0]}')

- **set_physics_properties(blueprint_name, component_name, simulate_physics=True, gravity_enabled=True, mass=1.0, linear_damping=0.01, angular_damping=0.0)**
  
  Set physics properties on a component.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
    - component_name (str): Name of the component
    - simulate_physics (bool): Whether to simulate physics on the component
    - gravity_enabled (bool): Whether gravity affects the component
    - mass (float): Mass of the component in kg
    - linear_damping (float): Linear damping factor
    - angular_damping (float): Angular damping factor
  
  Returns: Response indicating success or failure.

- **compile_blueprint(blueprint_name)**
  
  Compile a Blueprint.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
  
  Returns: Response indicating success or failure.

- **set_blueprint_property(blueprint_name, property_name, property_value)**
  
  Set a property on a Blueprint class default object.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint
    - property_name (str): Name of the property to set
    - property_value: Value to set the property to
  
  Returns: Response indicating success or failure.

- **set_pawn_properties(blueprint_name, auto_possess_player="", use_controller_rotation_yaw=None, use_controller_rotation_pitch=None, use_controller_rotation_roll=None, can_be_damaged=None)**
  
  Set common Pawn properties on a Blueprint. This is a utility function that sets multiple pawn-related properties at once.
  
  Args:
    - blueprint_name (str): Name of the target Blueprint (must be a Pawn or Character)
    - auto_possess_player (str): Auto possess player setting (None, "Disabled", "Player0", "Player1", etc.)
    - use_controller_rotation_yaw (bool): Whether the pawn should use the controller's yaw rotation
    - use_controller_rotation_pitch (bool): Whether the pawn should use the controller's pitch rotation
    - use_controller_rotation_roll (bool): Whether the pawn should use the controller's roll rotation
    - can_be_damaged (bool): Whether the pawn can be damaged
  
  Returns: Response indicating success or failure with detailed results for each property.

## Tools

- call_blueprint_function(target_name, function_name, string_params=None)
    Call a BlueprintCallable function by name.
- add_interface_to_blueprint(blueprint_name, interface_name)
    Add an interface to a Blueprint.
- create_blueprint_interface(name, folder_path="")
    Create a new Blueprint Interface asset.

## Node Creation

For advanced node creation including custom events, function calls, and complex Blueprint logic, 
use the Blueprint Action Commands which provide universal dynamic node creation via the 
Blueprint Action Database.

See the main server or tool docstrings for argument details and examples.
"""
from mcp.server.fastmcp import FastMCP
from blueprint_tools.blueprint_tools import register_blueprint_tools

mcp = FastMCP(
    "blueprintMCP",
    description="Blueprint tools for Unreal via MCP"
)

register_blueprint_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio') 