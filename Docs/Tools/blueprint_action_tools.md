# Unreal MCP Blueprint Action Tools

This document provides detailed information about the Blueprint Action tools available in the Unreal MCP integration.

## Overview

Blueprint Action tools allow you to dynamically discover available Blueprint actions using Unreal's FBlueprintActionDatabase, explore what nodes and functions are available for different pin types and classes, get detailed pin information, and create Blueprint nodes using discovered action names.

## Blueprint Action Tools

### get_actions_for_pin

Get all available Blueprint actions for a specific pin type with search filtering.

**Parameters:**
- `pin_type` (string) - The type of pin (object, int, float, bool, string, struct, etc.)
- `pin_subcategory` (string, optional) - The subcategory/class name for object pins (e.g., "PlayerController", "Pawn")
- `search_filter` (string, optional) - Optional search string to filter results (searches in name, keywords, category)
- `max_results` (int, optional) - Maximum number of results to return, defaults to 50

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `actions` (array) - List of available actions with title, tooltip, category, keywords
  - `pin_type` (string) - The pin type that was queried
  - `pin_subcategory` (string) - The pin subcategory that was queried
  - `action_count` (int) - Number of actions found
  - `message` (string) - Status message

**Example:**
```json
{
  "command": "get_actions_for_pin",
  "params": {
    "pin_type": "object",
    "pin_subcategory": "PlayerController",
    "search_filter": "input",
    "max_results": 25
  }
}
```

### get_actions_for_class

Get all available Blueprint actions for a specific class with search filtering.

**Parameters:**
- `class_name` (string) - Name or path of the class to get actions for (e.g., "PlayerController", "Pawn", "Actor")
- `search_filter` (string, optional) - Optional search string to filter results (searches in name, keywords, category)
- `max_results` (int, optional) - Maximum number of results to return, defaults to 50

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `actions` (array) - List of available actions with title, tooltip, category, keywords
  - `class_name` (string) - The class name that was queried
  - `action_count` (int) - Number of actions found
  - `message` (string) - Status message

**Example:**
```json
{
  "command": "get_actions_for_class",
  "params": {
    "class_name": "PlayerController",
    "search_filter": "movement",
    "max_results": 30
  }
}
```

### get_actions_for_class_hierarchy

Get all available Blueprint actions for a class and its entire inheritance hierarchy with search filtering.

**Parameters:**
- `class_name` (string) - Name or path of the class to get actions for (e.g., "PlayerController", "Pawn", "Actor")
- `search_filter` (string, optional) - Optional search string to filter results (searches in name, keywords, category)
- `max_results` (int, optional) - Maximum number of results to return, defaults to 50

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `actions` (array) - List of available actions with title, tooltip, category, keywords
  - `class_hierarchy` (array) - List of classes in the inheritance chain
  - `category_counts` (object) - Count of actions per category
  - `class_name` (string) - The class name that was queried
  - `action_count` (int) - Number of actions found
  - `message` (string) - Status message

**Example:**
```json
{
  "command": "get_actions_for_class_hierarchy",
  "params": {
    "class_name": "Character",
    "search_filter": "transform",
    "max_results": 50
  }
}
```

### search_blueprint_actions

Search for Blueprint actions using keywords.

**Parameters:**
- `search_query` (string) - Search string to find actions (searches in name, keywords, category, tooltip)
- `category` (string, optional) - Optional category filter (Flow Control, Math, Utilities, etc.)
- `max_results` (int, optional) - Maximum number of results to return, defaults to 50
- `blueprint_name` (string, optional) - Optional name of the Blueprint asset for local variable discovery

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `actions` (array) - List of matching actions with title, tooltip, category, keywords
  - `search_query` (string) - The search query that was used
  - `category_filter` (string) - The category filter that was applied
  - `action_count` (int) - Number of actions found
  - `message` (string) - Status message

**Example:**
```json
{
  "command": "search_blueprint_actions",
  "params": {
    "search_query": "add",
    "category": "Math",
    "max_results": 20,
    "blueprint_name": "BP_Calculator"
  }
}
```

### get_node_pin_info

Get specific information about a Blueprint node's pin including expected types.

**Parameters:**
- `node_name` (string) - Name of the Blueprint node (e.g., "Create Widget", "Get Controller", "Cast to PlayerController")
- `pin_name` (string) - Name of the specific pin (e.g., "Owning Player", "Class", "Return Value", "Target")

**Returns:**
- Dict containing:
  - `success` (boolean) - Whether the operation succeeded
  - `node_name` (string) - The node name that was queried
  - `pin_name` (string) - The pin name that was queried
  - `pin_info` (object) - Detailed information about the pin including:
    - `pin_type` (string) - Type category (object, class, exec, etc.)
    - `expected_type` (string) - Specific type expected (PlayerController, Class<UserWidget>, etc.)
    - `description` (string) - Description of the pin's purpose
    - `is_required` (boolean) - Whether the pin must be connected
    - `is_input` (boolean) - Whether it's an input (true) or output (false) pin
  - `message` (string) - Status message
  - `available_pins` (array) - List of available pins if the node is known but pin is not found

**Example:**
```json
{
  "command": "get_node_pin_info",
  "params": {
    "node_name": "Create Widget",
    "pin_name": "Owning Player"
  }
}
```

### create_node_by_action_name

Create a blueprint node by discovered action/function name.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint (e.g., "BP_MyActor")
- `function_name` (string) - Name of the function to create a node for (from discovered actions)
- `class_name` (string, optional) - Optional class name if the function is from a specific class (e.g., "KismetMathLibrary")
- `node_position` (array, optional) - Optional [X, Y] position in the graph (e.g., [100, 200])
- `json_params` (string, optional) - Optional JSON string with additional parameters for special nodes

**Returns:**
- Dict containing node creation result with node info and pins

**Example:**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_Calculator",
    "function_name": "Add_FloatFloat",
    "class_name": "KismetMathLibrary",
    "node_position": [100, 200]
  }
}
```

**Example with JSON parameters:**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_MyActor",
    "function_name": "CustomEvent",
    "json_params": "{\"event_name\": \"OnPlayerDied\"}"
  }
}
```

## Common Usage Patterns

### Discovery Workflow

1. **Discover Available Actions**: Use `get_actions_for_pin`, `get_actions_for_class`, or `search_blueprint_actions` to find available functions
2. **Understand Pin Requirements**: Use `get_node_pin_info` to understand what connections are needed
3. **Create Nodes**: Use `create_node_by_action_name` to create the discovered nodes
4. **Connect Nodes**: Use Node Tools to connect the created nodes

### Pin Type Reference

Common pin types for `get_actions_for_pin`:
- `object` - Object references (requires pin_subcategory like "PlayerController", "Pawn", "Actor")
- `struct` - Struct types (requires pin_subcategory like "Vector", "Transform", "Rotator")
- `float` - Floating point numbers
- `int` - Integer numbers
- `bool` - Boolean values
- `string` - String/text values
- `exec` - Execution flow pins

### Search Categories

Common categories for filtering:
- `Math` - Mathematical operations and functions
- `Flow Control` - Branching, loops, and flow control
- `Utilities` - General utility functions
- `Input` - Input handling and events
- `Movement` - Movement and transform operations
- `Physics` - Physics and collision functions
- `Audio` - Sound and audio functions
- `Rendering` - Graphics and rendering functions

### Class Reference

Common classes for action discovery:
- `Actor` - Base actor functionality
- `Pawn` - Basic pawn functionality
- `Character` - Character movement and abilities
- `PlayerController` - Player input and control
- `GameMode` - Game rules and state
- `StaticMeshComponent` - Static mesh rendering
- `AudioComponent` - Audio playback
- `LightComponent` - Lighting functionality 