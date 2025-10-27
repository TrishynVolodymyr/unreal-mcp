# Unreal MCP Blueprint Action Tools

This document provides detailed information about the Blueprint Action tools available in the Unreal MCP integration.

## Overview

Blueprint Action tools allow you to dynamically discover available Blueprint actions using Unreal's FBlueprintActionDatabase, explore what nodes and functions are available for different pin types and classes, get detailed pin information, and create Blueprint nodes using discovered action names.

## Blueprint Action Tools

### get_actions_for_pin

Get all available Blueprint actions for a specific pin type with search filtering.

**Parameters:**
- `pin_type` (string) **REQUIRED** - The type of pin (object, int, float, bool, string, struct, etc.)
- `pin_subcategory` (string, optional) - The subcategory/class name for object pins (e.g., "PlayerController", "Pawn")
- `search_filter` (string, optional) - Optional search string to filter results (searches in name, keywords, category)
- `max_results` (int, optional) - Maximum number of results to return, defaults to 50

**⚠️ Common Issues:**
- Pin types are case-sensitive: use "bool" not "boolean", "int" not "integer"
- For object pins, pin_subcategory is highly recommended to get relevant results
- Empty results may indicate incorrect pin_type or pin_subcategory spelling

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
- `blueprint_name` (string) **REQUIRED** - Name of the target Blueprint (e.g., "BP_MyActor")
- `function_name` (string) **REQUIRED** - Name of the function to create a node for (from discovered actions)
- `kwargs` (string) **REQUIRED** - JSON string with additional parameters (use "{}" if no special params needed)
- `class_name` (string, optional) - Optional class name if the function is from a specific class (e.g., "KismetMathLibrary")
- `node_position` (array, optional) - Optional [X, Y] position in the graph (e.g., [100, 200])
- `target_graph` (string, optional) - Optional specific graph to create node in (e.g., "EventGraph", "UpdateDialogueText")

**⚠️ Common Issues:**
- Function names must match exactly as discovered (case-sensitive)
- Some functions require class_name (e.g., "AddToViewport" needs class_name="UserWidget")
- Use search_blueprint_actions first to find correct function names
- For control flow nodes like "CustomEvent", use kwargs to specify event_name
- For cast nodes, use kwargs to specify target_type

**✅ Working Node Types:**
- Function calls (KismetMathLibrary, GameplayStatics, etc.)
- For Each Loop (Map) - UK2Node_MapForEach
- For Each Loop (Set) - UK2Node_SetForEach
- Control flow nodes (Branch, Sequence, etc.)
- Variable get/set nodes
- Custom events
- Cast nodes
- **Enhanced Input Action events** (e.g., IA_Jump, IA_Move, IA_Interact)

**💡 Enhanced Input Actions:**
Enhanced Input Action event nodes can be created in two ways:
1. **Just function_name**: `function_name="IA_Interact"` (recommended)
2. **With class_name**: `function_name="IA_Jump"`, `class_name="EnhancedInputAction"`

Both methods work identically. Use `search_blueprint_actions(search_query="IA_")` to discover available Input Actions.

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

### connect_blueprint_nodes

Connect nodes in a Blueprint's event graph. Supports both single connection (legacy) and batch connections (recommended).

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `source_node_id` (string, optional) - ID of the source node (legacy, single connection)
- `source_pin` (string, optional) - Name of the output pin on the source node (legacy, single connection)
- `target_node_id` (string, optional) - ID of the target node (legacy, single connection)
- `target_pin` (string, optional) - Name of the input pin on the target node (legacy, single connection)
- `connections` (array, optional) - List of connection objects for batch mode. Each object must have:
    - `source_node_id` (string)
    - `source_pin` (string)
    - `target_node_id` (string)
    - `target_pin` (string)

**Returns:**
- In single mode: Dict indicating success or failure for the connection.
- In batch mode: Dict with a `results` array, each entry indicating success or failure for each connection.

**Examples:**

_Batch connection:_
```json
{
  "command": "connect_blueprint_nodes",
  "params": {
    "blueprint_name": "BP_MyActor",
    "connections": [
      {"source_node_id": "GUID1", "source_pin": "Exec", "target_node_id": "GUID2", "target_pin": "Then"},
      {"source_node_id": "GUID3", "source_pin": "Out", "target_node_id": "GUID4", "target_pin": "In"}
    ]
  }
}
```

## Creating Getter and Setter Nodes for Blueprint Variables

You can programmatically create getter and setter nodes for any Blueprint variable using the `create_node_by_action_name` tool. The function name must follow these patterns:

- **Getter node:** `get <VariableName>`
- **Setter node:** `set <VariableName>`

This will create the corresponding variable get/set node in the Blueprint graph, just as if you dragged the variable into the graph in the Unreal Editor.

**Example: Create a getter node for a variable named `Health`**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_PlayerCharacter",
    "function_name": "get Health"
  }
}
```

**Example: Create a setter node for a variable named `Health`**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_PlayerCharacter",
    "function_name": "set Health"
  }
}
```

> **Note:** The variable name is case-sensitive and must match exactly as defined in the Blueprint. This pattern works for all variable types (Boolean, Integer, Float, Struct, Object Reference, etc.).

You can use this approach for any variable in any Blueprint, including UMG Widget Blueprints and Actor Blueprints.

## Common Usage Patterns

### Discovery Workflow

1. **Discover Available Actions**: Use `get_actions_for_pin`, `get_actions_for_class`, or `search_blueprint_actions` to find available functions
2. **Understand Pin Requirements**: Use `get_node_pin_info` to understand what connections are needed
3. **Create Nodes**: Use `create_node_by_action_name` to create the discovered nodes
4. **Connect Nodes**: Use Node Tools to connect the created nodes

### Practical Examples from Testing

**Example 1: Finding the correct function name**
```json
// WRONG: This will fail
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_GamePlayer",
    "function_name": "Add to Viewport",
    "kwargs": "{}"
  }
}

// CORRECT: Search first, then use exact name
{
  "command": "search_blueprint_actions",
  "params": {
    "search_query": "viewport"
  }
}
// Result shows "AddToViewport" is the correct name

{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_GamePlayer",
    "function_name": "AddToViewport",
    "class_name": "UserWidget",
    "kwargs": "{}"
  }
}
```

**Example 2: Creating custom events**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_GamePlayer",
    "function_name": "CustomEvent",
    "kwargs": "{\"event_name\": \"OnInteractPressed\"}",
    "node_position": [200, 300]
  }
}
```

**Example 3: Creating Enhanced Input Action events**

First, discover available Input Actions:
```json
{
  "command": "search_blueprint_actions",
  "params": {
    "search_query": "IA_",
    "max_results": 20
  }
}
// Result shows: IA_Jump, IA_Move, IA_Look, IA_Interact, etc.
```

Then create the event node (Method 1 - just function_name):
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_ThirdPersonCharacter",
    "function_name": "IA_Interact",
    "kwargs": "{}",
    "node_position": [100, 200]
  }
}
```

Or Method 2 (with class_name):
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_ThirdPersonCharacter",
    "function_name": "IA_Jump",
    "class_name": "EnhancedInputAction",
    "kwargs": "{}",
    "node_position": [100, 300]
  }
}
```

**Example 4: Creating cast nodes**
```json
{
  "command": "create_node_by_action_name",
  "params": {
    "blueprint_name": "BP_MyActor",
    "function_name": "Cast",
    "kwargs": "{\"target_type\": \"PlayerController\"}"
  }
}
```

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

## Troubleshooting and Error Handling

### Common Errors and Solutions

**Error: "Function 'X' not found and not a recognized control flow node"**
- **Cause**: Function name doesn't match exactly or doesn't exist
- **Solution**: Use `search_blueprint_actions` to find the correct function name
- **Example**: "Add to Viewport" should be "AddToViewport"

**Error: Node creation succeeds but has wrong pins**
- **Cause**: Missing or incorrect class_name parameter
- **Solution**: Some functions require specific class_name (e.g., AddToViewport needs "UserWidget")

**Error: Empty results from get_actions_for_pin**
- **Cause**: Incorrect pin_type or missing pin_subcategory for object pins
- **Solution**: Check pin_type spelling, add pin_subcategory for object pins

**Error: Blueprint compilation fails after node creation**
- **Cause**: Nodes created but not properly connected or missing required connections
- **Solution**: Use get_node_pin_info to understand required connections, then use connect_blueprint_nodes

### Best Practices

1. **Always search first**: Use search_blueprint_actions before creating nodes to ensure correct function names
2. **Check pin requirements**: Use get_node_pin_info to understand what connections are needed
3. **Use specific searches**: Include search_filter to narrow down results to relevant functions
4. **Test incrementally**: Create and test nodes one at a time rather than creating complex graphs at once
5. **Handle class names**: Some functions require class_name parameter - check the action discovery results

### Debugging Workflow

1. **Search for function**: `search_blueprint_actions` with relevant keywords
2. **Check pin info**: `get_node_pin_info` for the target node type
3. **Create node**: `create_node_by_action_name` with exact function name
4. **Verify creation**: Check returned node_id and pins list
5. **Connect nodes**: Use Node Tools to make connections
6. **Compile Blueprint**: Use Blueprint Tools to compile and check for errors 