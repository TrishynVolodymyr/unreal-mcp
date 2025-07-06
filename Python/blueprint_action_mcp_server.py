"""
Blueprint Action MCP Server

Exposes Blueprint Action discovery tools for Unreal Engine via MCP.
These tools allow dynamic discovery of available Blueprint actions using Unreal's FBlueprintActionDatabase.

## Blueprint Action Discovery Tools

- **get_actions_for_pin(pin_type, pin_subcategory="")**
  
  Get all available Blueprint actions for a specific pin type.
  
  This tool uses Unreal's FBlueprintActionDatabase to dynamically discover what nodes/functions
  are available when connecting to a specific pin type, similar to what you see in the context
  menu when dragging from a pin in the Blueprint editor.
  
  Args:
    - pin_type (str): The type of pin (object, int, float, bool, string, struct, etc.)
    - pin_subcategory (str): The subcategory/class name for object pins (e.g., "PlayerController", "Pawn")
  
  Returns:
    Dict containing:
      - success: Boolean indicating if the operation succeeded
      - actions: List of available actions with title, tooltip, category, keywords
      - pin_type: The pin type that was queried
      - pin_subcategory: The pin subcategory that was queried
      - action_count: Number of actions found
      - message: Status message
  
  Examples:
    get_actions_for_pin(pin_type="object", pin_subcategory="PlayerController")
    get_actions_for_pin(pin_type="float")
    get_actions_for_pin(pin_type="struct", pin_subcategory="Vector")

- **get_actions_for_class(class_name)**
  
  Get all available Blueprint actions for a specific class.
  
  This tool uses Unreal's FBlueprintActionDatabase to discover what functions and nodes
  are available for a specific class. This includes all the BlueprintCallable functions,
  events, and other actions that can be used with instances of this class.
  
  Args:
    - class_name (str): Name or path of the class to get actions for (e.g., "PlayerController", "Pawn", "Actor")
  
  Returns:
    Dict containing:
      - success: Boolean indicating if the operation succeeded
      - actions: List of available actions with title, tooltip, category, keywords
      - class_name: The class name that was queried
      - action_count: Number of actions found
      - message: Status message
  
  Examples:
    get_actions_for_class(class_name="PlayerController")
    get_actions_for_class(class_name="Actor")
    get_actions_for_class(class_name="/Script/Engine.Pawn")

- **get_actions_for_class_hierarchy(class_name)**
  
  Get all available Blueprint actions for a class and its entire inheritance hierarchy.
  
  This tool uses Unreal's FBlueprintActionDatabase to discover what functions and nodes
  are available for a class and all of its parent classes. This gives you the complete
  picture of all available actions including inherited functionality.
  
  Args:
    - class_name (str): Name or path of the class to get actions for (e.g., "PlayerController", "Pawn", "Actor")
  
  Returns:
    Dict containing:
      - success: Boolean indicating if the operation succeeded
      - actions: List of available actions with title, tooltip, category, keywords
      - class_hierarchy: List of classes in the inheritance chain
      - category_counts: Count of actions per category
      - class_name: The class name that was queried
      - action_count: Number of actions found
      - message: Status message
  
  Examples:
    get_actions_for_class_hierarchy(class_name="PlayerController")
    get_actions_for_class_hierarchy(class_name="Pawn")
    get_actions_for_class_hierarchy(class_name="StaticMeshActor")

## Use Cases

These tools are particularly useful for:
- **Blueprint Code Generation**: Dynamically discover what functions are available for a given class
- **Context-Aware Suggestions**: Provide intelligent suggestions based on pin types in Blueprint workflows  
- **Documentation**: Generate comprehensive lists of available functions and their capabilities
- **IDE Integration**: Power intelligent autocomplete and suggestion systems for Blueprint development
- **Dynamic Workflow Creation**: Build tools that can adapt to the available actions for different classes

The tools mirror the functionality that drives Unreal Engine's own context menus and action palettes
in the Blueprint editor, giving you programmatic access to the same rich action discovery system.
"""
from mcp.server.fastmcp import FastMCP
from blueprint_action_tools.blueprint_action_tools import register_blueprint_action_tools

mcp = FastMCP(
    "blueprintActionMCP", 
    description="Blueprint action discovery tools for Unreal via MCP"
)

# Register the blueprint action tools
register_blueprint_action_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio') 