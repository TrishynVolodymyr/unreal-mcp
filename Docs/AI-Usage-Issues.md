# AI Usage Issues with Unreal MCP Tools

## Critical Problems Encountered During AI-Driven Blueprint Creation

### 1. **Add Component by Class Node - Missing Class Parameter Setup**
**Problem**: When creating `Add Component by Class` node, there's no MCP tool to set the required `Class` pin value (e.g., SplineMeshComponent class reference).

**Impact**: AI cannot complete dynamic component creation workflows. Node creates but compilation fails with "must have a class specified" error.

**What AI Needs**: 
- Tool to set literal class values on pins
- Or tool to create class reference nodes
- Or parameter in `create_node_by_action_name` to specify class type for component nodes

### 2. **Variable Type Resolution - Arrays Not Supported**
**Problem**: `add_blueprint_variable` fails for array types like `SplineMeshComponent[]` with "Could not resolve variable type" error.

**Impact**: AI cannot create essential data structures for iterative operations (e.g., storing multiple generated components).

**What AI Needs**: Proper array type syntax or separate tool for array variables.

### 3. **Node Connection Type Mismatches**
**Problem**: When connecting nodes, type casting isn't automatic:
- `Get Component by Class` returns `ActorComponent`
- Spline methods need `SplineComponent`
- No intermediate Cast node automatically inserted

**Impact**: AI must manually create Cast nodes and manage type conversions, increasing complexity and error rate.

**What AI Needs**: 
- Auto-casting when connecting compatible types
- Or clearer error messages indicating which Cast node to insert

### 4. **Literal Value Creation**
**Problem**: No simple way to create literal int/float values for node inputs:
- `Make Literal Int` creates wrong node type
- Setting default pin values not supported
- Constant value nodes are complex to create

**Impact**: AI cannot set simple numeric values (like "1" for increment operations) without trial and error.

**What AI Needs**: Simple tool like `set_node_pin_default_value(node_id, pin_name, value)`.

### 5. **Component Access from Blueprint**
**Problem**: Variables for components (like `RoadSpline`, `SplineMesh_Template`) don't appear in blueprint action search.

**Impact**: AI cannot easily get references to components that exist in the blueprint hierarchy.

**What AI Needs**: Component variables to show up in `search_blueprint_actions` with blueprint_name parameter.

### 6. **Execution Flow Chain Breaking**
**Problem**: When reconnecting execution pins, old connections persist causing "failed building connection with 'Replace existing output connections'" errors.

**Impact**: AI cannot reliably modify execution flow without manual cleanup.

**What AI Needs**: Connection tool should auto-disconnect conflicting connections or provide `replace=True` parameter.

### 7. **Missing Construction Script Support**
**Problem**: No direct way to create or find ConstructionScript event node (essential for editor-time updates).

**Impact**: AI must use BeginPlay instead, which only works at runtime, not in editor viewport.

**What AI Needs**: Tool to create/find ConstructionScript event or better event node discovery.

## Recommended MCP Tool Improvements

1. ~~`set_node_pin_value(blueprint_name, node_id, pin_name, value)` - Set literal values~~ ✅ **IMPLEMENTED**
2. `create_node_by_action_name` - Add `pin_defaults` parameter for initial values
3. `add_blueprint_variable` - Support array syntax: `Type[]` or `Array<Type>`
4. `connect_blueprint_nodes` - Add `auto_cast=True` and `replace_existing=True` parameters
5. `search_blueprint_actions` - Include component variables from blueprint hierarchy
6. `create_event_node(blueprint_name, event_type, graph_name)` - Dedicated event creation

## Newly Added Tools

### `set_node_pin_value` ✅
Allows AI to set dropdown values, class references, enum values, and literal constants on node pins.

**Usage:**
```python
# Set class on Get Component by Class
set_node_pin_value(
    blueprint_name="BP_MyActor",
    node_id="node_guid",
    pin_name="ComponentClass", 
    value="SplineComponent"
)
```

Supports:
- Class references (full paths or short names)
- Enum values (with or without enum prefix)
- Literal values (int, float, bool, string)

## Workarounds Currently Required

- Dynamic component creation → Use pre-created components
- Array variables → Create multiple individual variables
- Type mismatches → Manual Cast node insertion
- Literal values → Search for "Make Literal" nodes
- Construction Script → Skip and use BeginPlay only
