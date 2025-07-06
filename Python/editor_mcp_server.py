"""
Editor MCP Server

Exposes Editor-related tools for Unreal Engine via MCP.

## Tools

- get_actors_in_level()
    Get a list of all actors in the current level.
- find_actors_by_name(pattern)
    Find actors by name pattern.
- spawn_actor(name, type, location=None, rotation=None)
    Create a new basic Unreal Engine actor in the current level.
- delete_actor(name)
    Delete an actor by name.
- set_actor_transform(name, location=None, rotation=None, scale=None)
    Set the transform of an actor.
- get_actor_properties(name)
    Get all properties of an actor.
- set_actor_property(name, property_name, property_value)
    Set a property on an actor.
- set_light_property(name, property_name, property_value)
    Set a property on a light component.

## Editor Tools
### Viewport and Screenshots
- `focus_viewport(target, location, distance, orientation)` - Focus viewport
- `take_screenshot(filename, show_ui, resolution)` - Capture screenshots

### Actor Management
- **get_actors_in_level()**
  
  Get a list of all actors in the current level.
  
  Returns: List of actors in the current level with their properties.
  
  Example:
    actors = get_actors_in_level()
    for actor in actors:
        print(actor["name"])

- **find_actors_by_name(pattern)**
  
  Find actors by name pattern (supports wildcards using *).
  
  Args:
    - pattern (str): Name pattern to search for
  
  Returns: List of actor names matching the pattern.
  
  Example:
    lights = find_actors_by_name("*PointLight*")
    player = find_actors_by_name("Player*")

- **spawn_actor(name, type, location=None, rotation=None)**
  
  Create a new basic Unreal Engine actor in the current level (built-in types only).
  
  Args:
    - name (str): The name to give the new actor (must be unique)
    - type (str): The type of built-in actor to create (StaticMeshActor, PointLight, SpotLight, DirectionalLight, CameraActor)
    - location (list): The [x, y, z] world location to spawn at
    - rotation (list): The [pitch, yaw, roll] rotation in degrees
  
  Returns: Dict containing the created actor's properties.
  
  Example:
    spawn_actor(name="MyLight", type="PointLight")
    spawn_actor(name="MyCube", type="StaticMeshActor", location=[100, 200, 50], rotation=[0, 45, 0])

- **delete_actor(name)**
  
  Delete an actor by name.
  
  Args:
    - name (str): Name of the actor to delete
  
  Returns: Dict containing response information.
  
  Example:
    delete_actor(name="MyCube")

- **set_actor_transform(name, location=None, rotation=None, scale=None)**
  
  Set the transform of an actor.
  
  Args:
    - name (str): Name of the actor
    - location (list): Optional [X, Y, Z] position
    - rotation (list): Optional [Pitch, Yaw, Roll] rotation in degrees
    - scale (list): Optional [X, Y, Z] scale
  
  Returns: Dict containing response information.
  
  Example:
    set_actor_transform(name="MyCube", location=[100, 200, 50])
    set_actor_transform(name="MyCube", rotation=[0, 0, 45])
    set_actor_transform(name="MyCube", scale=[2.0, 2.0, 2.0])
    set_actor_transform(name="MyCube", location=[100, 200, 50], rotation=[0, 0, 45], scale=[2.0, 2.0, 2.0])

- **get_actor_properties(name)**
  
  Get all properties of an actor.
  
  Args:
    - name (str): Name of the actor
  
  Returns: Dict containing actor properties.
  
  Example:
    props = get_actor_properties(name="MyCube")
    print(props["transform"]["location"])

- **set_actor_property(name, property_name, property_value)**
  
  Set a property on an actor.
  
  Args:
    - name (str): Name of the actor
    - property_name (str): Name of the property to set
    - property_value: Value to set the property to (various types)
  
  Returns: Dict containing response information.
  
  Example:
    set_actor_property(name="MyPointLight", property_name="LightColor", property_value=[255, 0, 0, 255])
    set_actor_property(name="MyCube", property_name="Mobility", property_value="Movable")
    set_actor_property(name="MyCube", property_name="bHidden", property_value=True)
    set_actor_property(name="PointLightTest", property_name="Intensity", property_value=5000.0)

See the main server or tool docstrings for argument details and examples.
"""
from mcp.server.fastmcp import FastMCP
from editor_tools.editor_tools import register_editor_tools

mcp = FastMCP(
    "editorMCP",
    description="Editor tools for Unreal via MCP"
)

register_editor_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio') 