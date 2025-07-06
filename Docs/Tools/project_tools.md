# Unreal MCP Project Tools

This document provides detailed information about the Project tools available in the Unreal MCP integration.

## Overview

Project tools allow you to manage project-wide settings and configuration in Unreal Engine, including input mappings, Enhanced Input systems, folder management, struct creation and management, and project organization.

## Project Tools

### create_input_mapping

Create an input mapping for the project (legacy input system).

**Parameters:**
- `action_name` (string) - Name of the input action
- `key` (string) - Key to bind (SpaceBar, LeftMouseButton, etc.)
- `input_type` (string, optional) - Type of input mapping (Action or Axis), defaults to "Action"

**Returns:**
- Dict containing success status and mapping information

**Example:**
```json
{
  "command": "create_input_mapping",
  "params": {
    "action_name": "Jump",
    "key": "SpaceBar",
    "input_type": "Action"
  }
}
```

### create_enhanced_input_action

Create an Enhanced Input Action asset.

**Parameters:**
- `action_name` (string) - Name of the input action (will add IA_ prefix if not present)
- `path` (string, optional) - Path where to create the action asset, defaults to "/Game/Input/Actions"
- `description` (string, optional) - Optional description for the action
- `value_type` (string, optional) - Type of input action ("Digital", "Analog", "Axis2D", "Axis3D"), defaults to "Digital"

**Returns:**
- Dict containing success status and asset details

**Example:**
```json
{
  "command": "create_enhanced_input_action",
  "params": {
    "action_name": "Jump",
    "path": "/Game/Input/Actions",
    "description": "Player jump action",
    "value_type": "Digital"
  }
}
```

### create_input_mapping_context

Create an Input Mapping Context asset.

**Parameters:**
- `context_name` (string) - Name of the mapping context (will add IMC_ prefix if not present)
- `path` (string, optional) - Path where to create the context asset, defaults to "/Game/Input"
- `description` (string, optional) - Optional description for the context

**Returns:**
- Dict containing success status and asset details

**Example:**
```json
{
  "command": "create_input_mapping_context",
  "params": {
    "context_name": "Default",
    "path": "/Game/Input",
    "description": "Default input mapping context"
  }
}
```

### add_mapping_to_context

Add a key mapping to an Input Mapping Context.

**Parameters:**
- `context_path` (string) - Full path to the Input Mapping Context asset
- `action_path` (string) - Full path to the Input Action asset
- `key` (string) - Key to bind (SpaceBar, LeftMouseButton, etc.)
- `shift` (boolean, optional) - Whether Shift modifier is required, defaults to false
- `ctrl` (boolean, optional) - Whether Ctrl modifier is required, defaults to false
- `alt` (boolean, optional) - Whether Alt modifier is required, defaults to false
- `cmd` (boolean, optional) - Whether Cmd modifier is required, defaults to false

**Returns:**
- Dict containing success status

**Example:**
```json
{
  "command": "add_mapping_to_context",
  "params": {
    "context_path": "/Game/Input/IMC_Default",
    "action_path": "/Game/Input/Actions/IA_Jump",
    "key": "SpaceBar",
    "shift": false,
    "ctrl": false,
    "alt": false,
    "cmd": false
  }
}
```

### list_input_actions

List all Enhanced Input Action assets in the project.

**Parameters:**
- `path` (string, optional) - Path to search for Input Actions (searches recursively), defaults to "/Game"

**Returns:**
- Dict containing list of Input Action assets and their details

**Example:**
```json
{
  "command": "list_input_actions",
  "params": {
    "path": "/Game/Input"
  }
}
```

### list_input_mapping_contexts

List all Input Mapping Context assets in the project.

**Parameters:**
- `path` (string, optional) - Path to search for Input Mapping Contexts (searches recursively), defaults to "/Game"

**Returns:**
- Dict containing list of Input Mapping Context assets and their details

**Example:**
```json
{
  "command": "list_input_mapping_contexts",
  "params": {
    "path": "/Game/Input"
  }
}
```

### create_folder

Create a new folder in the Unreal project.

**Parameters:**
- `folder_path` (string) - Path to create, relative to project root. Use "Content/..." prefix for content browser folders

**Returns:**
- Dict containing creation status and folder path

**Example:**
```json
{
  "command": "create_folder",
  "params": {
    "folder_path": "Content/MyGameContent"
  }
}
```

### create_struct

Create a new Unreal struct.

**Parameters:**
- `struct_name` (string) - Name of the struct to create
- `properties` (array) - List of property dictionaries, each containing:
  - `name` (string) - Property name
  - `type` (string) - Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
  - `description` (string, optional) - Property description
- `path` (string, optional) - Path where to create the struct, defaults to "/Game/Blueprints"
- `description` (string, optional) - Optional description for the struct

**Returns:**
- Dict containing creation status and struct path

**Example:**
```json
{
  "command": "create_struct",
  "params": {
    "struct_name": "PlayerStats",
    "properties": [
      {
        "name": "Health",
        "type": "Float",
        "description": "Player's current health"
      },
      {
        "name": "MaxHealth",
        "type": "Float",
        "description": "Player's maximum health"
      },
      {
        "name": "Level",
        "type": "Integer",
        "description": "Player's current level"
      },
      {
        "name": "PlayerName",
        "type": "String",
        "description": "Player's display name"
      }
    ],
    "path": "/Game/DataStructures",
    "description": "Structure containing player statistics"
  }
}
```

### update_struct

Update an existing Unreal struct.

**Parameters:**
- `struct_name` (string) - Name of the struct to update
- `properties` (array) - List of property dictionaries, each containing:
  - `name` (string) - Property name
  - `type` (string) - Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
  - `description` (string, optional) - Property description
- `path` (string, optional) - Path where the struct exists, defaults to "/Game/Blueprints"
- `description` (string, optional) - Optional description for the struct

**Returns:**
- Dict containing update status and struct path

**Example:**
```json
{
  "command": "update_struct",
  "params": {
    "struct_name": "PlayerStats",
    "properties": [
      {
        "name": "Health",
        "type": "Float",
        "description": "Player's current health"
      },
      {
        "name": "MaxHealth",
        "type": "Float",
        "description": "Player's maximum health"
      },
      {
        "name": "Level",
        "type": "Integer",
        "description": "Player's current level"
      },
      {
        "name": "PlayerName",
        "type": "String",
        "description": "Player's display name"
      },
      {
        "name": "Experience",
        "type": "Integer",
        "description": "Player's current experience points"
      }
    ],
    "path": "/Game/DataStructures"
  }
}
```

### show_struct_variables

Show variables and types of a struct in Unreal Engine.

**Parameters:**
- `struct_name` (string) - Name of the struct to inspect
- `path` (string, optional) - Path where the struct exists, defaults to "/Game/Blueprints"

**Returns:**
- Dict containing struct variable information

**Example:**
```json
{
  "command": "show_struct_variables",
  "params": {
    "struct_name": "PlayerStats",
    "path": "/Game/DataStructures"
  }
}
```

### list_folder_contents

List the contents of a folder in the Unreal project.

**Parameters:**
- `folder_path` (string) - Path to the folder (e.g., "/Game/Blueprints" or "Content/MyFolder" or "Intermediate/MyTools")

**Returns:**
- Dict containing arrays of subfolders and files/assets

**Example:**
```json
{
  "command": "list_folder_contents",
  "params": {
    "folder_path": "/Game/Blueprints"
  }
}
```

## Common Usage Patterns

### Enhanced Input System Setup

1. **Create Input Actions**: Use `create_enhanced_input_action` for each input you need
2. **Create Mapping Context**: Use `create_input_mapping_context` to group related inputs
3. **Add Mappings**: Use `add_mapping_to_context` to bind keys to actions
4. **List Assets**: Use `list_input_actions` and `list_input_mapping_contexts` to verify

```json
// Step 1: Create actions
{
  "command": "create_enhanced_input_action",
  "params": {
    "action_name": "Move",
    "value_type": "Axis2D"
  }
}

// Step 2: Create context
{
  "command": "create_input_mapping_context",
  "params": {
    "context_name": "PlayerInput"
  }
}

// Step 3: Add mappings
{
  "command": "add_mapping_to_context",
  "params": {
    "context_path": "/Game/Input/IMC_PlayerInput",
    "action_path": "/Game/Input/Actions/IA_Move",
    "key": "W"
  }
}
```

### Struct Management Workflow

1. **Create Struct**: Use `create_struct` to define data structure
2. **Inspect Structure**: Use `show_struct_variables` to see current properties
3. **Update Structure**: Use `update_struct` to modify properties
4. **Use in DataTables**: Reference struct in `create_datatable` operations

```json
// Create a game item struct
{
  "command": "create_struct",
  "params": {
    "struct_name": "GameItem",
    "properties": [
      {"name": "ItemID", "type": "String"},
      {"name": "DisplayName", "type": "String"},
      {"name": "Description", "type": "String"},
      {"name": "Price", "type": "Float"},
      {"name": "Weight", "type": "Float"},
      {"name": "IsStackable", "type": "Boolean"},
      {"name": "MaxStackSize", "type": "Integer"},
      {"name": "ItemType", "type": "String"},
      {"name": "Rarity", "type": "String"}
    ],
    "path": "/Game/DataStructures"
  }
}
```

### Project Organization

1. **Create Folders**: Use `create_folder` to organize content
2. **List Contents**: Use `list_folder_contents` to explore structure
3. **Place Assets**: Create assets in appropriate folders using path parameters

```json
// Create organized folder structure
{
  "command": "create_folder",
  "params": {
    "folder_path": "Content/GameData"
  }
}

{
  "command": "create_folder",
  "params": {
    "folder_path": "Content/GameData/Structures"
  }
}

{
  "command": "create_folder",
  "params": {
    "folder_path": "Content/GameData/Tables"
  }
}
```

### Input System Types

**Enhanced Input Action Value Types:**
- `Digital` - Simple on/off input (buttons, keys)
- `Analog` - Single-axis input with range (triggers, scroll wheels)
- `Axis2D` - Two-axis input (mouse movement, gamepad sticks)
- `Axis3D` - Three-axis input (3D motion controllers)

**Common Key Names:**
- **Keyboard**: A, B, C, ..., Z, SpaceBar, Enter, Escape, Tab, LeftShift, RightShift, LeftControl, RightControl
- **Mouse**: LeftMouseButton, RightMouseButton, MiddleMouseButton, MouseWheelUp, MouseWheelDown
- **Gamepad**: Gamepad_LeftX, Gamepad_LeftY, Gamepad_RightX, Gamepad_RightY, Gamepad_LeftTrigger, Gamepad_RightTrigger

### Struct Property Types

**Basic Types:**
- `Boolean` - True/false values
- `Integer` - Whole numbers (int32)
- `Float` - Decimal numbers (float)
- `String` - Text strings
- `Name` - Unreal name type (optimized strings)

**Unreal Engine Types:**
- `Vector` - 3D vector (X, Y, Z)
- `Vector2D` - 2D vector (X, Y)
- `Rotator` - Rotation (Pitch, Yaw, Roll)
- `Transform` - Location, rotation, and scale
- `Color` - RGBA color values
- `LinearColor` - Linear color space RGBA

**Advanced Types:**
- `Array<Type>` - Arrays of any type
- `Map<KeyType, ValueType>` - Key-value maps
- `Set<Type>` - Sets of unique values
- Object references to other assets

### Error Handling

Always check return values for operation success:
```json
// Typical successful response
{
  "success": true,
  "message": "Operation completed successfully",
  "asset_path": "/Game/Input/Actions/IA_Jump",
  "details": {...}
}

// Typical error response
{
  "success": false,
  "message": "Failed to create asset",
  "error": "Path does not exist: /Invalid/Path"
}
```

### Integration with Other Tools

Project tools work seamlessly with other tool categories:
- **Blueprint Tools**: Use created structs as variable types
- **DataTable Tools**: Use created structs as DataTable row structures
- **UMG Tools**: Reference input actions in widget event bindings
- **Editor Tools**: Manage project assets and organization 