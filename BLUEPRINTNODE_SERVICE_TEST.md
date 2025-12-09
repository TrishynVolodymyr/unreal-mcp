# BlueprintNodeService Refactoring Test Plan

This document contains MCP tool payloads to test all refactored functionality after splitting BlueprintNodeService into BlueprintNodeConnectionService and BlueprintNodeQueryService.

## Refactoring Summary

**Original File:** `BlueprintNodeService.cpp` (1612 lines)

**After Refactoring:**
- `BlueprintNodeService.cpp` → 265 lines (83.6% reduction)
- `BlueprintNodeConnectionService.cpp` → 896 lines (connection logic & auto-casting)
- `BlueprintNodeQueryService.cpp` → 566 lines (query & lookup operations)

**All functionality preserved via delegation pattern.**

---

## Prerequisites

1. Unreal Engine 5.7 Editor running with UnrealMCP plugin loaded
2. MCPGameProject open
3. All 7 FastMCP servers active
4. Test Blueprint: `BP_RefactorTest` (will be created in tests)

---

## Test Suite

### Phase 1: Query Operations (BlueprintNodeQueryService)

These tests verify that query methods still work after extraction to BlueprintNodeQueryService.

#### Test 1.1: Create Test Blueprint
**Tool:** `create_blueprint`
```json
{
  "name": "BP_RefactorTest",
  "parent_class": "Actor",
  "folder_path": "/Game/Testing"
}
```
**Expected Result:**
- Blueprint created successfully
- Response: `{"success": true, "blueprint_path": "/Game/Testing/BP_RefactorTest"}`

---

#### Test 1.2: Get Blueprint Graphs
**Tool:** `get_blueprint_graphs` (or equivalent)
```json
{
  "blueprint_name": "BP_RefactorTest"
}
```
**Expected Result:**
- Returns list of graph names
- Should include: `["EventGraph"]`
- Uses `FBlueprintNodeQueryService::GetBlueprintGraphs()`

---

#### Test 1.3: Add Variable to Blueprint
**Tool:** `add_blueprint_variable`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "variable_name": "TestSpeed",
  "variable_type": "float",
  "default_value": 100.0
}
```
**Expected Result:**
- Variable added successfully
- Variable appears in Blueprint variable list

---

#### Test 1.4: Get Variable Info
**Tool:** `get_variable_info` (or equivalent)
```json
{
  "blueprint_name": "BP_RefactorTest",
  "variable_name": "TestSpeed"
}
```
**Expected Result:**
- Returns variable type: `"float"`
- Returns additional info: `{"is_array": false, "is_reference": false}`
- Uses `FBlueprintNodeQueryService::GetVariableInfo()`

---

#### Test 1.5: Add Event Node
**Tool:** `add_event_node` or `create_node_by_action_name`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "event_type": "BeginPlay",
  "graph_name": "EventGraph",
  "position": [100, 100]
}
```
**Expected Result:**
- BeginPlay event node created
- Returns node ID
- Node visible in EventGraph

---

#### Test 1.6: Add Function Call Node
**Tool:** `add_function_call_node` or `create_node_by_action_name`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "function_name": "PrintString",
  "class_name": "KismetSystemLibrary",
  "graph_name": "EventGraph",
  "position": [400, 100]
}
```
**Expected Result:**
- PrintString node created
- Returns node ID
- Node has correct pins (execute, InString, etc.)

---

#### Test 1.7: Find All Nodes (No Filter)
**Tool:** `find_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "node_type": "",
  "event_type": "",
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Returns all nodes in EventGraph
- Should include: BeginPlay, PrintString
- Each node has: `{"node_id": "...", "node_title": "...", "node_type": "...", "position": [...], "pins": [...]}`
- Uses `FBlueprintNodeQueryService::FindBlueprintNodes()`

---

#### Test 1.8: Find Event Nodes Only
**Tool:** `find_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "node_type": "Event",
  "event_type": "",
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Returns only event nodes (BeginPlay)
- Uses filtered search in `FBlueprintNodeQueryService::FindBlueprintNodes()`

---

#### Test 1.9: Find Function Call Nodes Only
**Tool:** `find_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "node_type": "Function",
  "event_type": "",
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Returns only function call nodes (PrintString)

---

### Phase 2: Connection Operations (BlueprintNodeConnectionService)

These tests verify that connection methods still work after extraction to BlueprintNodeConnectionService.

#### Test 2.1: Connect BeginPlay to PrintString (Simple Connection)
**Tool:** `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<BeginPlay_NodeId>",
      "source_pin": "then",
      "target_node_id": "<PrintString_NodeId>",
      "target_pin": "execute"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Execution pins connected successfully
- Response: `{"success": true, "results": [true]}`
- Uses `FBlueprintNodeConnectionService::ConnectBlueprintNodes()`

**Note:** Replace `<BeginPlay_NodeId>` and `<PrintString_NodeId>` with actual node IDs from Test 1.7

---

#### Test 2.2: Add Variable Get Node
**Tool:** `add_variable_node` or `create_node_by_action_name`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "variable_name": "TestSpeed",
  "is_getter": true,
  "graph_name": "EventGraph",
  "position": [400, 250]
}
```
**Expected Result:**
- Variable Get node created for TestSpeed
- Returns node ID

---

#### Test 2.3: Connect Variable to PrintString (Type Conversion Test)
**Tool:** `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<TestSpeed_Get_NodeId>",
      "source_pin": "TestSpeed",
      "target_node_id": "<PrintString_NodeId>",
      "target_pin": "InString"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- **Auto-cast node created** (Float → String conversion)
- Connection succeeds with intermediate cast node
- Uses `FBlueprintNodeConnectionService::ConnectNodesWithAutoCast()`
- Verifies `CreateFloatToStringCast()` method works

---

#### Test 2.4: Add Integer Variable
**Tool:** `add_blueprint_variable`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "variable_name": "TestCount",
  "variable_type": "int",
  "default_value": 42
}
```
**Expected Result:**
- Integer variable created

---

#### Test 2.5: Add Variable Get for Integer
**Tool:** `add_variable_node`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "variable_name": "TestCount",
  "is_getter": true,
  "graph_name": "EventGraph",
  "position": [400, 400]
}
```
**Expected Result:**
- Variable Get node created for TestCount

---

#### Test 2.6: Add Second PrintString
**Tool:** `add_function_call_node`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "function_name": "PrintString",
  "class_name": "KismetSystemLibrary",
  "graph_name": "EventGraph",
  "position": [700, 400]
}
```
**Expected Result:**
- Second PrintString node created

---

#### Test 2.7: Connect Integer to PrintString (Int→String Cast)
**Tool:** `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<TestCount_Get_NodeId>",
      "source_pin": "TestCount",
      "target_node_id": "<PrintString2_NodeId>",
      "target_pin": "InString"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- **Auto-cast node created** (Int → String conversion)
- Verifies `CreateIntToStringCast()` method works

---

#### Test 2.8: Add Boolean Variable
**Tool:** `add_blueprint_variable`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "variable_name": "bIsActive",
  "variable_type": "bool",
  "default_value": true
}
```
**Expected Result:**
- Boolean variable created

---

#### Test 2.9: Connect Boolean to PrintString (Bool→String Cast)
**Tool:** Similar to Test 2.7, but with boolean variable
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<bIsActive_Get_NodeId>",
      "source_pin": "bIsActive",
      "target_node_id": "<PrintString3_NodeId>",
      "target_pin": "InString"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- **Auto-cast node created** (Bool → String conversion)
- Verifies `CreateBoolToStringCast()` method works

---

### Phase 3: Object Casting Test

#### Test 3.1: Spawn Actor Node
**Tool:** `add_function_call_node`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "function_name": "SpawnActor",
  "class_name": "GameplayStatics",
  "graph_name": "EventGraph",
  "position": [100, 600]
}
```
**Expected Result:**
- SpawnActor node created (returns AActor*)

---

#### Test 3.2: Cast to Specific Actor Type
**Tool:** `connect_blueprint_nodes` (with object cast)
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<SpawnActor_NodeId>",
      "source_pin": "ReturnValue",
      "target_node_id": "<TargetNode_RequiringSpecificType>",
      "target_pin": "Target"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- **Object cast node created** if types don't match
- Verifies `CreateObjectCast()` method works
- Uses dynamic cast (K2Node_DynamicCast)

---

### Phase 4: Multiple Connections Test

#### Test 4.1: Connect Multiple Nodes at Once
**Tool:** `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<Node1_Id>",
      "source_pin": "then",
      "target_node_id": "<Node2_Id>",
      "target_pin": "execute"
    },
    {
      "source_node_id": "<Node2_Id>",
      "source_pin": "then",
      "target_node_id": "<Node3_Id>",
      "target_pin": "execute"
    },
    {
      "source_node_id": "<VarGet_Id>",
      "source_pin": "Variable",
      "target_node_id": "<Node3_Id>",
      "target_pin": "InputPin"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- All connections succeed
- Response: `{"success": true, "results": [true, true, true]}`
- Verifies batch connection handling

---

### Phase 5: Pin Type Compatibility Tests

#### Test 5.1: Connect Compatible Types (No Cast Needed)
**Tool:** `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "<FloatVariable_NodeId>",
      "source_pin": "FloatVar",
      "target_node_id": "<FloatInput_NodeId>",
      "target_pin": "FloatParam"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Direct connection (no cast node)
- Verifies `ArePinTypesCompatible()` returns true for same types

---

#### Test 5.2: Wildcard Pin Connection (PromotableOperator)
**Tool:** `add_function_call_node` + `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "function_name": "Add",
  "class_name": "KismetMathLibrary",
  "graph_name": "EventGraph",
  "position": [400, 700]
}
```
Then connect to it:
```json
{
  "connections": [
    {
      "source_node_id": "<FloatVar_NodeId>",
      "source_pin": "FloatVar",
      "target_node_id": "<Add_NodeId>",
      "target_pin": "A"
    }
  ]
}
```
**Expected Result:**
- Wildcard pin adapts to Float type
- Verifies PromotableOperator handling in `ConnectNodesWithAutoCast()`

---

### Phase 6: Find Node by ID/Type Tests

#### Test 6.1: Find FunctionEntry Node
**Tool:** `find_blueprint_nodes` or direct node search
```json
{
  "blueprint_name": "BP_RefactorTest",
  "node_id_or_type": "FunctionEntry",
  "target_graph": "SomeFunction"
}
```
**Expected Result:**
- Finds function entry node
- Verifies `FindNodeByIdOrType()` special identifier handling

---

#### Test 6.2: Find FunctionResult Node
**Tool:** Similar to Test 6.1
```json
{
  "node_id_or_type": "FunctionResult"
}
```
**Expected Result:**
- Finds function result/return node
- Verifies special case handling

---

### Phase 7: Compile and Verify

#### Test 7.1: Compile Blueprint
**Tool:** `compile_blueprint`
```json
{
  "blueprint_name": "BP_RefactorTest"
}
```
**Expected Result:**
- Blueprint compiles successfully
- No errors or warnings from connections
- All cast nodes properly integrated

---

#### Test 7.2: Verify All Nodes Present
**Tool:** `find_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "node_type": "",
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Returns all nodes including:
  - Original nodes (BeginPlay, PrintString, etc.)
  - Auto-generated cast nodes (Float→String, Int→String, Bool→String)
  - All connections intact

---

### Phase 8: Edge Cases

#### Test 8.1: Connect to Non-Existent Node (Error Handling)
**Tool:** `connect_blueprint_nodes`
```json
{
  "blueprint_name": "BP_RefactorTest",
  "connections": [
    {
      "source_node_id": "InvalidNodeId123",
      "source_pin": "then",
      "target_node_id": "<ValidNodeId>",
      "target_pin": "execute"
    }
  ],
  "target_graph": "EventGraph"
}
```
**Expected Result:**
- Connection fails gracefully
- Response: `{"success": false, "error": "Node not found"}`
- Verifies error handling in connection service

---

#### Test 8.2: Connect Incompatible Types (No Cast Available)
**Tool:** `connect_blueprint_nodes`
```json
{
  "connections": [
    {
      "source_node_id": "<ComplexStruct_NodeId>",
      "source_pin": "StructOutput",
      "target_node_id": "<SimpleType_NodeId>",
      "target_pin": "IntInput"
    }
  ]
}
```
**Expected Result:**
- Connection fails (no valid cast)
- Response: `{"success": false, "results": [false]}`
- Verifies `CreateCastNode()` returns false when no cast available

---

#### Test 8.3: Break Execution Pin Link (Only One Outgoing)
**Tool:** `connect_blueprint_nodes`
```json
{
  "connections": [
    {
      "source_node_id": "<BeginPlay_NodeId>",
      "source_pin": "then",
      "target_node_id": "<NewNode_Id>",
      "target_pin": "execute"
    }
  ]
}
```
**Expected Result:**
- Previous execution connection broken automatically
- New connection established
- Verifies execution pin handling (breaks old connections)

---

## Success Criteria

✅ **Refactoring is successful if:**

1. **All query operations work** (Phase 1: Tests 1.1-1.9)
   - GetBlueprintGraphs returns correct graph list
   - FindBlueprintNodes finds all nodes with correct filtering
   - GetVariableInfo returns variable details

2. **All connection operations work** (Phase 2-3: Tests 2.1-3.2)
   - Simple pin connections succeed
   - Auto-cast nodes created for type mismatches
   - All 6 cast methods work (Int↔String, Float↔String, Bool→String, Object→Object)

3. **Special features preserved** (Phase 4-6)
   - Multiple connections in one call
   - Pin type compatibility checking
   - FindNodeByIdOrType handles special identifiers

4. **Error handling intact** (Phase 8)
   - Invalid connections fail gracefully
   - Incompatible types handled correctly

5. **Blueprint compiles** (Phase 7)
   - All nodes and connections valid
   - No compilation errors

---

## Testing Instructions

### For AI Testing Agent:

1. **Execute tests sequentially** - each phase builds on previous
2. **Record node IDs** - save node IDs from query results for connection tests
3. **Replace placeholders** - substitute `<NodeId>` with actual IDs from responses
4. **Check responses** - verify `success: true` and expected data structure
5. **Log failures** - note any test that doesn't match expected result
6. **Compile at end** - final compilation confirms all changes valid

### Expected MCP Tool Names:

The exact tool names may vary based on your MCP server configuration. Common names:
- `create_blueprint`
- `add_blueprint_variable`
- `get_variable_info`
- `add_event_node` or `create_node_by_action_name`
- `add_function_call_node` or `create_node_by_action_name`
- `find_blueprint_nodes`
- `connect_blueprint_nodes`
- `compile_blueprint`
- `get_blueprint_graphs`

Adjust tool names to match your actual MCP server implementation.

---

## Verification Checklist

After running all tests, verify:

- [ ] All 30+ tests executed
- [ ] BlueprintNodeQueryService methods called (query operations)
- [ ] BlueprintNodeConnectionService methods called (connection operations)
- [ ] Auto-cast nodes created for type mismatches
- [ ] GetSafeNodeId helper works (node IDs generated correctly)
- [ ] No crashes or Unreal Editor errors
- [ ] Blueprint compiles without warnings
- [ ] All functionality preserved (100% behavior match)

---

## Notes

- **Exact Copy-Paste Refactoring**: All methods use identical logic to original
- **Delegation Pattern**: BlueprintNodeService delegates to specialized services
- **No Behavior Changes**: All tests should pass identically to pre-refactoring state
- **Line Count**: Original 1612 lines split into 265 + 896 + 566 = 1727 total (more maintainable)

---

## Cleanup

After testing, optionally delete test blueprint:
```json
{
  "blueprint_path": "/Game/Testing/BP_RefactorTest"
}
```

Use your MCP server's delete/remove blueprint tool.
