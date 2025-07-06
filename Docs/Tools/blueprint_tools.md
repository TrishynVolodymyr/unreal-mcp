# Unreal MCP Blueprint Tools

This document provides detailed information about the Blueprint tools available in the Unreal MCP integration.

## Overview

Blueprint tools allow you to create and manipulate Blueprint assets in Unreal Engine, including creating new Blueprint classes, adding components, setting properties, and spawning Blueprint actors in the level.

## Blueprint Tools

### create_blueprint

Create a new Blueprint class.

**Parameters:**
- `name` (string) - The name for the new Blueprint class
- `parent_class` (string) - The parent class for the Blueprint

**Returns:**
- Information about the created Blueprint including success status and message

**Example:**
```json
{
  "command": "create_blueprint",
  "params": {
    "name": "MyActor",
    "parent_class": "Actor"
  }
}
```

### add_component_to_blueprint

Add a component to a Blueprint.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint
- `component_type` (string) - The type of component to add (use component class name without U prefix)
- `component_name` (string) - The name for the new component
- `location` (array, optional) - [X, Y, Z] coordinates for component's position, defaults to [0, 0, 0]
- `rotation` (array, optional) - [Pitch, Yaw, Roll] values for component's rotation, defaults to [0, 0, 0]
- `scale` (array, optional) - [X, Y, Z] values for component's scale, defaults to [1, 1, 1]
- `component_properties` (object, optional) - Additional properties to set on the component

**Returns:**
- Information about the added component including success status and message

**Example:**
```json
{
  "command": "add_component_to_blueprint",
  "params": {
    "blueprint_name": "MyActor",
    "component_type": "StaticMeshComponent",
    "component_name": "Mesh",
    "location": [0, 0, 0],
    "rotation": [0, 0, 0],
    "scale": [1, 1, 1],
    "component_properties": {
      "bVisible": true
    }
  }
}
```

### set_static_mesh_properties

Set the mesh for a StaticMeshComponent.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint
- `component_name` (string) - The name of the StaticMeshComponent
- `static_mesh` (string, default: "/Engine/BasicShapes/Cube.Cube") - Path to the static mesh asset

**Returns:**
- Result of the mesh setting operation including success status and message

**Example:**
```json
{
  "command": "set_static_mesh_properties",
  "params": {
    "blueprint_name": "MyActor",
    "component_name": "Mesh",
    "static_mesh": "/Engine/BasicShapes/Sphere.Sphere"
  }
}
```

### set_component_property

Set one or more properties on a component in a Blueprint.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint
- `component_name` (string) - The name of the component
- `kwargs` (object) - Dictionary of property names and values to set. You can pass properties as direct keyword arguments (recommended), or as a single dict using `kwargs={...}`. If double-wrapped (i.e., `kwargs={'kwargs': {...}}`), the function will automatically flatten it for convenience. This matches the widget property setter pattern.

**Returns:**
- Result of the property setting operation including lists of success_properties and failed_properties

**Examples:**

Preferred usage (direct keyword arguments):
```json
{
  "command": "set_component_property",
  "params": {
    "blueprint_name": "MyActor",
    "component_name": "Mesh",
    "kwargs": {
      "StaticMesh": "/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube",
      "Mobility": "Movable"
    }
  }
}
```

Also supported (dict):
```python
set_component_property(ctx, "MyActor", "Mesh", kwargs={"StaticMesh": "/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube", "Mobility": "Movable"})
```

If double-wrapped (rare, but handled):
```python
set_component_property(ctx, "MyActor", "Mesh", kwargs={"kwargs": {"StaticMesh": "/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube", "Mobility": "Movable"}})
```

All styles above will be handled correctly and set the properties as expected.

### set_physics_properties

Set physics properties on a component.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint
- `component_name` (string) - The name of the component
- `simulate_physics` (boolean, optional) - Whether to simulate physics, defaults to true
- `gravity_enabled` (boolean, optional) - Whether gravity is enabled, defaults to true
- `mass` (float, optional) - The mass of the component, defaults to 1.0
- `linear_damping` (float, optional) - Linear damping value, defaults to 0.01
- `angular_damping` (float, optional) - Angular damping value, defaults to 0.0

**Returns:**
- Result of the physics properties setting operation including success status and message

**Example:**
```json
{
  "command": "set_physics_properties",
  "params": {
    "blueprint_name": "MyActor",
    "component_name": "Mesh",
    "simulate_physics": true,
    "gravity_enabled": true,
    "mass": 10.0,
    "linear_damping": 0.05,
    "angular_damping": 0.1
  }
}
```

### compile_blueprint

Compile a Blueprint.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint to compile

**Returns:**
- Result of the compilation operation including success status and message

**Example:**
```json
{
  "command": "compile_blueprint",
  "params": {
    "blueprint_name": "MyActor"
  }
}
```

### set_blueprint_property

Set a property on a Blueprint class default object.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint
- `property_name` (string) - The name of the property to set
- `property_value` (any) - The value to set for the property

**Returns:**
- Result of the property setting operation including success status and message

**Example:**
```json
{
  "command": "set_blueprint_property",
  "params": {
    "blueprint_name": "MyActor",
    "property_name": "bCanBeDamaged",
    "property_value": true
  }
}
```

### set_pawn_properties

Set common Pawn properties on a Blueprint.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint (must be a Pawn or Character)
- `auto_possess_player` (string, optional) - Auto possess player setting (None, "Disabled", "Player0", "Player1", etc.), defaults to empty string
- `use_controller_rotation_yaw` (boolean, optional) - Whether the pawn should use the controller's yaw rotation, defaults to false
- `use_controller_rotation_pitch` (boolean, optional) - Whether the pawn should use the controller's pitch rotation, defaults to false
- `use_controller_rotation_roll` (boolean, optional) - Whether the pawn should use the controller's roll rotation, defaults to false
- `can_be_damaged` (boolean, optional) - Whether the pawn can be damaged, defaults to true

**Returns:**
- Response indicating success or failure with detailed results for each property

**Example:**
```json
{
  "command": "set_pawn_properties",
  "params": {
    "blueprint_name": "MyPawn",
    "auto_possess_player": "Player0",
    "use_controller_rotation_yaw": true,
    "can_be_damaged": true
  }
}
```

### spawn_blueprint_actor

Spawn an actor from a Blueprint.

**Parameters:**
- `blueprint_name` (string) - The name of the Blueprint to spawn
- `actor_name` (string) - The name for the spawned actor
- `location` (array, optional) - [X, Y, Z] coordinates for the actor's position, defaults to [0, 0, 0]
- `rotation` (array, optional) - [Pitch, Yaw, Roll] values for the actor's rotation, defaults to [0, 0, 0]
- `scale` (array, optional) - [X, Y, Z] values for the actor's scale, defaults to [1, 1, 1]

**Returns:**
- Information about the spawned actor including success status and message

**Example:**
```json
{
  "command": "spawn_blueprint_actor",
  "params": {
    "blueprint_name": "MyActor",
    "actor_name": "MyActorInstance",
    "location": [0, 0, 100],
    "rotation": [0, 45, 0],
    "scale": [1, 1, 1]
  }
}
```

### add_blueprint_variable

Add a variable to a Blueprint. Supports built-in types, arrays, and custom structures.

**Parameters:**
- `blueprint_name` (string) - The name of the target Blueprint
- `variable_name` (string) - Name of the variable to create
- `variable_type` (string) - Type of the variable (see supported types below)
- `is_exposed` (boolean, optional) - Whether to expose the variable to the editor, defaults to false

**Supported Variable Types:**

1. Basic Types:
- `Boolean` - Boolean value (true/false)
- `Integer` or `Int` - 32-bit integer
- `Float` - Floating point number
- `String` - Text string
- `Name` - Unreal Engine FName type
- `Text` - Unreal Engine localized text
- `Vector` - 3D vector (X, Y, Z)
- `Rotator` - Rotation in Euler angles
- `Transform` - Complete transform (Location, Rotation, Scale)
- `Color` - Linear color (RGBA)

2. Array Types:
Add `[]` suffix to any basic type to create an array:
- `String[]` - Array of strings
- `Integer[]` - Array of integers
- `Float[]` - Array of floats
etc.

3. Custom Structs:
For custom structs, use either:
- Full path: `/Game/Path/To/StructName`
- Struct name with F prefix: `FStructName`
- Plain struct name: `StructName`

**Returns:**
- Result of the variable creation operation including success status and variable information

**Examples:**

1. Basic Types:
```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "PlayerBlueprint",
    "variable_name": "Score",
    "variable_type": "Integer",
    "is_exposed": true
  }
}
```

```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "PlayerBlueprint",
    "variable_name": "PlayerName",
    "variable_type": "String",
    "is_exposed": true
  }
}
```

```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "PlayerBlueprint",
    "variable_name": "Position",
    "variable_type": "Vector",
    "is_exposed": true
  }
}
```

2. Array Types:
```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "PlayerBlueprint",
    "variable_name": "Inventory",
    "variable_type": "String[]",
    "is_exposed": true
  }
}
```

```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "PlayerBlueprint",
    "variable_name": "HighScores",
    "variable_type": "Integer[]",
    "is_exposed": true
  }
}
```

3. Custom Struct Types:
First create the struct:
```json
{
  "command": "create_struct",
  "params": {
    "struct_name": "PlayerStats",
    "properties": [
      {"name": "Health", "type": "Float"},
      {"name": "Armor", "type": "Float"},
      {"name": "Level", "type": "Integer"}
    ],
    "path": "/Game/DataStructures"
  }
}
```

Then add the struct variable:
```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "PlayerBlueprint",
    "variable_name": "Stats",
    "variable_type": "/Game/DataStructures/PlayerStats",
    "is_exposed": true
  }
}
```

**Notes:**
1. Variable names must be unique within the Blueprint
2. Variable types are case-insensitive (e.g., "integer" and "Integer" are equivalent)
3. When using custom structs, it's recommended to use the full path to avoid ambiguity
4. The `is_exposed` parameter determines if the variable is visible and editable in the editor's Details panel
5. Array variables are created by appending `[]` to the base type
6. Custom struct variables require the struct to exist before creating the variable

## Error Handling

All command responses include a "success" field indicating whether the operation succeeded, and a "message" field with details in case of failure.

```json
{
  "success": false,
  "message": "Failed to connect to Unreal Engine"
}
```

## Implementation Notes

- All transform arrays (location, rotation, scale) must contain exactly 3 float values
- Empty lists for transform parameters will be automatically converted to default values
- The server maintains detailed logging of all operations
- All commands require a successful connection to the Unreal Engine editor
- Failed operations will return detailed error messages in the response
- Component types should be specified without the 'U' prefix (e.g., "StaticMeshComponent" instead of "UStaticMeshComponent")
- For socket-based communication, refer to the test scripts in unreal-mcp/Python/scripts/blueprints for examples
