# Refactoring Test Plan - UnrealMCPCommonUtils

This document contains complete MCP tool payloads to test all refactored functionality after splitting UnrealMCPCommonUtils into 6 utility classes.

## Refactoring Summary

**Files Created:**
- `JsonUtils.h/.cpp` - JSON parsing and response creation
- `GeometryUtils.h/.cpp` - Vector, rotator, color parsing
- `PropertyUtils.h/.cpp` - UObject property manipulation
- `AssetUtils.h/.cpp` - Asset discovery and loading
- `GraphUtils.h/.cpp` - Blueprint graph manipulation
- `ActorUtils.h/.cpp` - Actor finding and serialization

**Original File:** `UnrealMCPCommonUtils.cpp`
- Before: 2119 lines
- After: 820 lines
- Reduction: 61.3%

---

## Test Suite

### Phase 1: JsonUtils Testing (JSON Response Creation)

**Test 1.1: Create Blueprint (Tests JSON response creation)**
```json
{
  "name": "BP_TestRefactor",
  "parent_class": "Actor",
  "folder_path": "/Game/Testing"
}
```
**Expected Result:**
- Blueprint created successfully
- JSON response with `success: true`
- Uses `FJsonUtils::CreateSuccessResponse()`

**Test 1.2: Create Blueprint with Invalid Parent (Tests error response)**
```json
{
  "name": "BP_TestInvalid",
  "parent_class": "NonExistentClass",
  "folder_path": "/Game/Testing"
}
```
**Expected Result:**
- Error response with `success: false`
- Uses `FJsonUtils::CreateErrorResponse()`

---

### Phase 2: GeometryUtils Testing (Vector/Rotator Parsing)

**Test 2.1: Spawn Actor with Location**
```json
{
  "actor_class": "Actor",
  "location": [100.0, 200.0, 300.0],
  "actor_name": "TestActor_Location"
}
```
**Expected Result:**
- Actor spawned at exact location (100, 200, 300)
- Uses `FGeometryUtils::ParseVector()`

**Test 2.2: Spawn Actor with Rotation**
```json
{
  "actor_class": "Actor",
  "rotation": [0.0, 90.0, 0.0],
  "actor_name": "TestActor_Rotation"
}
```
**Expected Result:**
- Actor spawned with 90-degree yaw rotation
- Uses `FGeometryUtils::ParseRotator()`

**Test 2.3: Add Static Mesh Component with Color**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "component_name": "ColoredMesh",
  "component_type": "StaticMeshComponent",
  "properties": {
    "material_color": [1.0, 0.0, 0.0, 1.0]
  }
}
```
**Expected Result:**
- Component added with red color (if property exists)
- Uses `FGeometryUtils::ParseLinearColor()`

---

### Phase 3: PropertyUtils Testing (Property Manipulation)

**Test 3.1: Add Variable to Blueprint (Bool Property)**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "variable_name": "bIsActive",
  "variable_type": "bool",
  "default_value": true
}
```
**Expected Result:**
- Boolean variable created and set to true
- Uses `FPropertyUtils::SetObjectProperty()` or `SetPropertyFromJson()`

**Test 3.2: Add Variable to Blueprint (String Property)**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "variable_name": "PlayerName",
  "variable_type": "string",
  "default_value": "TestPlayer"
}
```
**Expected Result:**
- String variable created with default value
- Uses `FPropertyUtils::SetPropertyFromJson()`

**Test 3.3: Add Variable to Blueprint (Float Property)**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "variable_name": "Speed",
  "variable_type": "float",
  "default_value": 100.5
}
```
**Expected Result:**
- Float variable created with value 100.5
- Uses `FPropertyUtils::SetPropertyFromJson()`

**Test 3.4: Add Component with Properties**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "component_name": "TestComponent",
  "component_type": "StaticMeshComponent",
  "properties": {
    "Mobility": 2,
    "bVisible": true
  }
}
```
**Expected Result:**
- Component added with mobility set to Movable (2)
- Uses `FPropertyUtils::SetObjectProperty()` for enum and bool

**Test 3.5: Add Component with Struct Property (Vector)**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "component_name": "TestTransform",
  "component_type": "SceneComponent",
  "properties": {
    "RelativeLocation": [50.0, 100.0, 150.0]
  }
}
```
**Expected Result:**
- Component added with relative location set
- Uses `FPropertyUtils::SetObjectProperty()` for FVector struct

---

### Phase 4: AssetUtils Testing (Asset Discovery & Loading)

**Test 4.1: Find Blueprint by Name**
```json
{
  "blueprint_name": "BP_TestRefactor"
}
```
**Expected Result:**
- Blueprint found and returned
- Uses `FAssetUtils::FindAssetByName()` or `FindBlueprint()`

**Test 4.2: Create Widget Blueprint**
```json
{
  "widget_name": "WBP_TestRefactor",
  "parent_class": "UserWidget",
  "folder_path": "/Game/Testing"
}
```
**Expected Result:**
- Widget Blueprint created
- Uses `FAssetUtils::FindWidgetClass()` internally

**Test 4.3: Add Widget to Widget Blueprint**
```json
{
  "widget_blueprint_name": "WBP_TestRefactor",
  "widget_name": "TestButton",
  "widget_type": "Button"
}
```
**Expected Result:**
- Button widget added
- Uses `FAssetUtils::FindWidgetClass()` for Button type discovery

**Test 4.4: Find All Widget Blueprints**
```json
{
  "search_path": "/Game"
}
```
**Tool:** `list_widget_blueprints`
**Expected Result:**
- List includes WBP_TestRefactor
- Uses `FAssetUtils::FindWidgetBlueprints()`

**Test 4.5: Create DataTable with Struct**
```json
{
  "table_name": "DT_TestRefactor",
  "struct_type": "Vector",
  "folder_path": "/Game/Testing"
}
```
**Expected Result:**
- DataTable created with FVector struct
- Uses `FAssetUtils::FindStructType()` to find built-in Vector type

**Test 4.6: Validate Asset Path**
```json
{
  "asset_path": "/Game/Testing/BP_TestRefactor"
}
```
**Tool:** Custom validation or blueprint existence check
**Expected Result:**
- Returns true if BP_TestRefactor exists
- Uses `FAssetUtils::IsValidAssetPath()` and `NormalizeAssetPath()`

---

### Phase 5: GraphUtils Testing (Blueprint Node Manipulation)

**Test 5.1: Create Event Node**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "event_name": "BeginPlay",
  "graph_name": "EventGraph"
}
```
**Expected Result:**
- BeginPlay event node created or found
- Uses `FGraphUtils::FindExistingEventNode()`

**Test 5.2: Create Function Call Node**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "function_name": "PrintString",
  "target_class": "KismetSystemLibrary",
  "graph_name": "EventGraph"
}
```
**Expected Result:**
- PrintString node created
- Ready for connection testing

**Test 5.3: Connect Nodes (Tests Pin Finding and Connection)**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "source_node": "BeginPlay",
  "source_pin": "then",
  "target_node": "PrintString",
  "target_pin": "execute",
  "graph_name": "EventGraph"
}
```
**Expected Result:**
- Nodes connected via execution pins
- Uses `FGraphUtils::ConnectGraphNodes()` and `FindPin()`

**Test 5.4: Create Variable Get and Connect**
```json
{
  "blueprint_name": "BP_TestRefactor",
  "variable_name": "Speed",
  "node_type": "VariableGet",
  "graph_name": "EventGraph",
  "position": [400, 200]
}
```
**Expected Result:**
- Variable Get node created
- Uses `FGraphUtils::FindPin()` to find output pin

---

### Phase 6: ActorUtils Testing (Actor Operations)

**Test 6.1: Spawn Actor**
```json
{
  "actor_class": "Actor",
  "actor_name": "TestActor_ActorUtils",
  "location": [0.0, 0.0, 100.0]
}
```
**Expected Result:**
- Actor spawned successfully
- Can be found by name

**Test 6.2: Find Actor by Name**
```json
{
  "actor_name": "TestActor_ActorUtils"
}
```
**Tool:** `find_actor` or similar
**Expected Result:**
- Actor found and returned
- Uses `FActorUtils::FindActorByName()`

**Test 6.3: List All Actors (Tests Actor JSON Serialization)**
```json
{
  "actor_class": "Actor"
}
```
**Tool:** `list_actors`
**Expected Result:**
- Returns JSON array with actor info
- Uses `FActorUtils::ActorToJson()` or `ActorToJsonObject()`

**Test 6.4: Get Actor Details**
```json
{
  "actor_name": "TestActor_ActorUtils",
  "detailed": true
}
```
**Expected Result:**
- Returns detailed JSON with location, rotation, scale
- Uses `FActorUtils::ActorToJsonObject(Actor, true)`

**Test 6.5: Call Function on Actor (if applicable)**
```json
{
  "actor_name": "TestActor_ActorUtils",
  "function_name": "K2_SetActorLocation",
  "parameters": ["[100.0, 100.0, 100.0]"]
}
```
**Expected Result:**
- Actor moves to new location
- Uses `FActorUtils::CallFunctionByName()`

---

## Integration Tests (Multi-Utility Testing)

### Integration Test 1: Complete Blueprint Workflow

**Step 1: Create Blueprint**
```json
{
  "name": "BP_IntegrationTest",
  "parent_class": "Actor",
  "folder_path": "/Game/Testing"
}
```
**Utilities Used:** JsonUtils (response)

**Step 2: Add Component with Properties**
```json
{
  "blueprint_name": "BP_IntegrationTest",
  "component_name": "TestMesh",
  "component_type": "StaticMeshComponent",
  "properties": {
    "RelativeLocation": [0.0, 0.0, 50.0],
    "bVisible": true
  }
}
```
**Utilities Used:** AssetUtils (find blueprint), PropertyUtils (set properties), GeometryUtils (parse vector)

**Step 3: Add Variable**
```json
{
  "blueprint_name": "BP_IntegrationTest",
  "variable_name": "TestValue",
  "variable_type": "float",
  "default_value": 42.0
}
```
**Utilities Used:** AssetUtils (find blueprint), PropertyUtils (set property)

**Step 4: Create Event Graph Node**
```json
{
  "blueprint_name": "BP_IntegrationTest",
  "event_name": "BeginPlay",
  "graph_name": "EventGraph"
}
```
**Utilities Used:** AssetUtils (find blueprint), GraphUtils (find/create event)

**Step 5: Compile Blueprint**
```json
{
  "blueprint_name": "BP_IntegrationTest"
}
```
**Utilities Used:** AssetUtils (find blueprint), JsonUtils (response)

**Step 6: Spawn Actor Instance**
```json
{
  "blueprint_class": "BP_IntegrationTest",
  "actor_name": "IntegrationTestActor",
  "location": [200.0, 200.0, 100.0]
}
```
**Utilities Used:** AssetUtils (find blueprint), GeometryUtils (parse location), ActorUtils (spawn)

**Step 7: Verify Actor**
```json
{
  "actor_name": "IntegrationTestActor"
}
```
**Utilities Used:** ActorUtils (find, serialize to JSON), JsonUtils (response format)

---

### Integration Test 2: Widget Blueprint Workflow

**Step 1: Create Widget Blueprint**
```json
{
  "widget_name": "WBP_IntegrationTest",
  "parent_class": "UserWidget",
  "folder_path": "/Game/Testing"
}
```
**Utilities Used:** AssetUtils (widget class finding), JsonUtils (response)

**Step 2: Add Button Widget**
```json
{
  "widget_blueprint_name": "WBP_IntegrationTest",
  "widget_name": "TestButton",
  "widget_type": "Button",
  "properties": {
    "position": [100.0, 100.0]
  }
}
```
**Utilities Used:** AssetUtils (find widget blueprint, find Button class), PropertyUtils (set properties), GeometryUtils (parse position)

**Step 3: Compile Widget**
```json
{
  "blueprint_name": "WBP_IntegrationTest"
}
```
**Utilities Used:** AssetUtils (find), JsonUtils (response)

---

## Critical Validation Points

### ✅ Checklist for Each Test

For **every test above**, verify:

1. **Compilation Success** - No C++ errors
2. **Functionality Preserved** - Same behavior as before refactoring
3. **JSON Response Format** - Proper success/error responses (JsonUtils)
4. **No Memory Leaks** - All TSharedPtr usage correct
5. **Logging Works** - UE_LOG statements execute properly
6. **Error Handling** - Invalid inputs return proper errors

### Utilities Coverage Matrix

| Utility | Primary Tests | Integration Tests | Coverage |
|---------|--------------|-------------------|----------|
| JsonUtils | 1.1, 1.2 | All (response format) | ✅ |
| GeometryUtils | 2.1, 2.2, 2.3 | Integration 1 (Step 2, 6), Integration 2 (Step 2) | ✅ |
| PropertyUtils | 3.1-3.5 | Integration 1 (Step 2, 3), Integration 2 (Step 2) | ✅ |
| AssetUtils | 4.1-4.6 | Integration 1 (Step 1-6), Integration 2 (Step 1-3) | ✅ |
| GraphUtils | 5.1-5.4 | Integration 1 (Step 4) | ✅ |
| ActorUtils | 6.1-6.5 | Integration 1 (Step 6, 7) | ✅ |

---

## Test Execution Order

**Recommended execution sequence:**

1. **Phase 1** (JsonUtils) - Foundation for all responses
2. **Phase 2** (GeometryUtils) - Required for spatial operations
3. **Phase 3** (PropertyUtils) - Core property manipulation
4. **Phase 4** (AssetUtils) - Asset discovery and loading
5. **Phase 5** (GraphUtils) - Blueprint graph operations
6. **Phase 6** (ActorUtils) - Runtime actor operations
7. **Integration Test 1** - Complete Blueprint workflow
8. **Integration Test 2** - Widget Blueprint workflow

---

## Success Criteria

✅ **Refactoring is successful if:**

1. All 30+ individual tests pass
2. Both integration tests complete without errors
3. No compilation warnings introduced
4. No functionality regressions detected
5. Performance is equal or better
6. File sizes: All under 1000 lines (UnrealMCPCommonUtils.cpp now 820 lines)

---

## Notes for Tester

- **Environment**: Unreal Engine 5.7 Editor must be running
- **MCP Servers**: All 7 FastMCP servers should be active
- **Test Project**: MCPGameProject with UnrealMCP plugin loaded
- **Cleanup**: Delete `/Game/Testing` folder before starting tests
- **Verification**: Check Unreal Editor logs for any errors during tests

---