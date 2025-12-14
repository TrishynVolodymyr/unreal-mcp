# Blueprint Extension Features Testing Plan

This document provides a complete testing plan for the newly implemented Blueprint-to-Blueprint inheritance and metadata introspection features. All tests use MCP tool payloads that can be executed by any AI assistant with access to the Unreal MCP server.

## Prerequisites

1. Unreal Editor must be running with the UnrealMCP plugin loaded
2. Python MCP servers must be running (blueprint_mcp_server.py)
3. Test in the MCPGameProject or any UE 5.7 project with the plugin

---

## Phase 1: Blueprint-to-Blueprint Inheritance Tests

### Test 1.1: Create Base Blueprint with Native C++ Parent

**Objective**: Verify basic blueprint creation still works with native classes.

**MCP Tool**: `create_blueprint`

**Payload**:
```json
{
  "name": "BP_TestBase",
  "parent_class": "Actor",
  "folder_path": "Testing/Blueprints"
}
```

**Expected Result**:
- Blueprint created at `/Game/Testing/Blueprints/BP_TestBase`
- Response: `{"success": true, "name": "BP_TestBase", "path": "...", "already_exists": false}`

---

### Test 1.2: Create Child Blueprint Extending Another Blueprint (By Name)

**Objective**: Test Blueprint-to-Blueprint inheritance using simple Blueprint name.

**MCP Tool**: `create_blueprint`

**Payload**:
```json
{
  "name": "BP_TestChild",
  "parent_class": "BP_TestBase",
  "folder_path": "Testing/Blueprints"
}
```

**Expected Result**:
- Blueprint created with BP_TestBase as parent
- Response: `{"success": true, "name": "BP_TestChild", ...}`
- Verify in editor: BP_TestChild inherits from BP_TestBase's GeneratedClass

---

### Test 1.3: Create Child Blueprint Extending Another Blueprint (By Full Path)

**Objective**: Test Blueprint-to-Blueprint inheritance using full asset path.

**MCP Tool**: `create_blueprint`

**Payload**:
```json
{
  "name": "BP_TestGrandchild",
  "parent_class": "/Game/Testing/Blueprints/BP_TestChild",
  "folder_path": "Testing/Blueprints"
}
```

**Expected Result**:
- Blueprint created with BP_TestChild as parent
- Response: `{"success": true, "name": "BP_TestGrandchild", ...}`
- Inheritance chain: BP_TestGrandchild → BP_TestChild → BP_TestBase → AActor

---

### Test 1.4: Add Components to Base Blueprint

**Objective**: Set up base blueprint with components for inheritance testing.

**MCP Tool**: `add_component_to_blueprint`

**Payload 1** (Add Static Mesh):
```json
{
  "blueprint_name": "BP_TestBase",
  "component_type": "StaticMeshComponent",
  "component_name": "BaseMesh",
  "location": [0, 0, 0],
  "scale": [1, 1, 1]
}
```

**Payload 2** (Add Sphere Collision):
```json
{
  "blueprint_name": "BP_TestBase",
  "component_type": "SphereComponent",
  "component_name": "CollisionSphere",
  "location": [0, 0, 100]
}
```

**Expected Result**:
- Components added to BP_TestBase
- Child blueprints (BP_TestChild, BP_TestGrandchild) should inherit these components

---

### Test 1.5: Add Variables to Base Blueprint

**Objective**: Test variable inheritance across Blueprint hierarchy.

**MCP Tool**: `add_blueprint_variable`

**Payload 1** (Base Health):
```json
{
  "blueprint_name": "BP_TestBase",
  "variable_name": "BaseHealth",
  "variable_type": "Float",
  "is_exposed": true
}
```

**Payload 2** (Base Name):
```json
{
  "blueprint_name": "BP_TestBase",
  "variable_name": "BaseName",
  "variable_type": "String",
  "is_exposed": true
}
```

**Expected Result**:
- Variables added to BP_TestBase
- Child blueprints inherit these variables

---

### Test 1.6: Multi-Level Inheritance Chain

**Objective**: Create a 4-level deep inheritance chain.

**MCP Tool**: `create_blueprint` (multiple calls)

**Payload Sequence**:

1. Create level 1:
```json
{
  "name": "BP_Enemy",
  "parent_class": "Character",
  "folder_path": "Testing/Inheritance"
}
```

2. Create level 2:
```json
{
  "name": "BP_EnemyMelee",
  "parent_class": "BP_Enemy",
  "folder_path": "Testing/Inheritance"
}
```

3. Create level 3:
```json
{
  "name": "BP_EnemyBrute",
  "parent_class": "BP_EnemyMelee",
  "folder_path": "Testing/Inheritance"
}
```

4. Create level 4:
```json
{
  "name": "BP_EnemyBossBrute",
  "parent_class": "BP_EnemyBrute",
  "folder_path": "Testing/Inheritance"
}
```

**Expected Result**:
- All blueprints created successfully
- Inheritance chain: BP_EnemyBossBrute → BP_EnemyBrute → BP_EnemyMelee → BP_Enemy → ACharacter

---

### Test 1.7: Verify Compilation After Inheritance

**Objective**: Ensure child blueprints compile successfully.

**MCP Tool**: `compile_blueprint`

**Payload**:
```json
{
  "blueprint_name": "BP_TestGrandchild"
}
```

**Expected Result**:
- Response: `{"success": true, "status": "UpToDate" or "UpToDateWithWarnings", "errors": [], "warnings": []}`
- No compilation errors related to parent class resolution

---

## Phase 2: Blueprint Metadata Introspection Tests

### Test 2.1: Get All Metadata from Base Blueprint

**Objective**: Retrieve complete metadata including all fields.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["*"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "parent_class": {
      "name": "Actor",
      "type": "Native",
      "path": "/Script/Engine.Actor",
      "inheritance_chain": [...]
    },
    "interfaces": {
      "interfaces": [],
      "count": 0
    },
    "variables": {
      "variables": [
        {"name": "BaseHealth", "type": "float", "category": "Default", "instance_editable": true, ...},
        {"name": "BaseName", "type": "string", "category": "Default", "instance_editable": true, ...}
      ],
      "count": 2
    },
    "components": {
      "components": [
        {"name": "BaseMesh", "type": "StaticMeshComponent"},
        {"name": "CollisionSphere", "type": "SphereComponent"}
      ],
      "count": 2
    },
    "functions": {...},
    "graphs": {...},
    "status": {"status": "UpToDate", "blueprint_type": "Normal"},
    "metadata": {...},
    "timelines": {...},
    "asset_info": {"asset_path": "/Game/Testing/Blueprints/BP_TestBase", ...}
  }
}
```

---

### Test 2.2: Get Selective Metadata (Parent Class Only)

**Objective**: Test selective field querying for performance.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestChild",
  "fields": ["parent_class"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "parent_class": {
      "name": "BP_TestBase_C",
      "type": "Blueprint",
      "path": "/Game/Testing/Blueprints/BP_TestBase.BP_TestBase_C",
      "inheritance_chain": [
        {"name": "BP_TestBase_C", "path": "..."},
        {"name": "Actor", "path": "/Script/Engine.Actor"},
        {"name": "Object", "path": "/Script/CoreUObject.Object"}
      ]
    }
  }
}
```

**Validation**:
- Parent class type should be "Blueprint" (not "Native")
- Parent class name should reference BP_TestBase's GeneratedClass
- Inheritance chain should include both Blueprint and native parents

---

### Test 2.3: Get Selective Metadata (Variables and Components)

**Objective**: Query multiple specific fields.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["variables", "components"]
}
```

**Expected Result**:
- Only "variables" and "components" fields returned
- No "parent_class", "interfaces", etc.
- Variables and components lists populated with correct data

---

### Test 2.3.1: Get Components Only (Replaces list_blueprint_components)

**Objective**: Demonstrate that `get_blueprint_metadata` with `fields=["components"]` provides the same functionality as the deprecated `list_blueprint_components` tool.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["components"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "components": {
      "components": [
        {"name": "BaseMesh", "type": "StaticMeshComponent"},
        {"name": "CollisionSphere", "type": "SphereComponent"}
      ],
      "count": 2
    }
  }
}
```

**Note**: This test verifies that the unified `get_blueprint_metadata` command with `fields=["components"]` returns comprehensive component data using the same `BlueprintService.GetBlueprintComponents()` method as the old `list_blueprint_components` command, making that command redundant.

---

### Test 2.4: Create and Introspect Blueprint Interface

**Objective**: Test interface metadata introspection.

**Setup - Create Interface**:

**MCP Tool**: `create_blueprint_interface`

**Payload**:
```json
{
  "name": "BPI_Interactable",
  "folder_path": "Testing/Interfaces"
}
```

**Setup - Add Interface to Blueprint**:

**MCP Tool**: `add_interface_to_blueprint`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "interface_name": "BPI_Interactable"
}
```

**Test - Get Interface Metadata**:

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["interfaces"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "interfaces": {
      "interfaces": [
        {
          "name": "BPI_Interactable",
          "path": "/Game/Testing/Interfaces/BPI_Interactable",
          "functions": [
            {"name": "OnInteract", "implemented": false},
            {"name": "CanInteract", "implemented": false}
          ]
        }
      ],
      "count": 1
    }
  }
}
```

**Validation**:
- Interface name and path correct
- Functions listed with implementation status
- Implemented status shows false if not implemented

---

### Test 2.5: Get Status Information

**Objective**: Verify Blueprint compilation status reporting.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["status"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "status": {
      "status": "UpToDate",
      "blueprint_type": "Normal"
    }
  }
}
```

**Possible Status Values**:
- "Unknown", "Dirty", "Error", "UpToDate", "BeingCreated", "UpToDateWithWarnings"

---

### Test 2.6: Get Asset Information

**Objective**: Retrieve file system and package information.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["asset_info"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "asset_info": {
      "asset_path": "/Game/Testing/Blueprints/BP_TestBase",
      "package_name": "/Game/Testing/Blueprints/BP_TestBase",
      "disk_size_bytes": 12345
    }
  }
}
```

---

### Test 2.7: Verify Inheritance Chain Through Metadata

**Objective**: Confirm complete inheritance chain visibility.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestGrandchild",
  "fields": ["parent_class"]
}
```

**Expected Result**:
```json
{
  "success": true,
  "metadata": {
    "parent_class": {
      "name": "BP_TestChild_C",
      "type": "Blueprint",
      "path": "/Game/Testing/Blueprints/BP_TestChild.BP_TestChild_C",
      "inheritance_chain": [
        {"name": "BP_TestChild_C", "path": "..."},
        {"name": "BP_TestBase_C", "path": "..."},
        {"name": "Actor", "path": "/Script/Engine.Actor"},
        {"name": "Object", "path": "/Script/CoreUObject.Object"}
      ]
    }
  }
}
```

**Validation**:
- Full chain visible: BP_TestChild_C → BP_TestBase_C → Actor → Object
- Both Blueprint and Native classes in chain
- Paths correctly formatted

---

## Phase 3: Edge Cases and Error Handling

### Test 3.1: Create Blueprint with Non-Existent Blueprint Parent

**Objective**: Verify error handling for invalid Blueprint parent.

**MCP Tool**: `create_blueprint`

**Payload**:
```json
{
  "name": "BP_FailTest",
  "parent_class": "BP_DoesNotExist",
  "folder_path": "Testing/Errors"
}
```

**Expected Result**:
- Blueprint created with fallback to AActor parent
- Warning logged about unable to find Blueprint parent
- Response: `{"success": true, "name": "BP_FailTest", ...}` (falls back to Actor)

---

### Test 3.2: Get Metadata from Non-Existent Blueprint

**Objective**: Test error handling for missing Blueprint.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_DoesNotExist",
  "fields": ["*"]
}
```

**Expected Result**:
```json
{
  "success": false,
  "error": "Blueprint 'BP_DoesNotExist' not found"
}
```

---

### Test 3.3: Get Metadata with Invalid Field Names

**Objective**: Verify behavior with invalid field requests.

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_TestBase",
  "fields": ["invalid_field", "parent_class", "nonexistent"]
}
```

**Expected Result**:
- Only valid fields returned ("parent_class")
- Invalid fields silently ignored
- No error thrown

---

### Test 3.4: Create Blueprint with Interface as Parent

**Objective**: Ensure interface Blueprints cannot be used as parents.

**MCP Tool**: `create_blueprint`

**Payload**:
```json
{
  "name": "BP_InterfaceParentTest",
  "parent_class": "BPI_Interactable",
  "folder_path": "Testing/Errors"
}
```

**Expected Result**:
- Blueprint creation fails or falls back to AActor
- Interface Blueprints (BPTYPE_Interface) filtered out in FindBlueprintParentClass
- Warning/error about invalid parent type

---

## Phase 4: Performance and Scale Tests

### Test 4.1: Deep Inheritance Chain (10 Levels)

**Objective**: Test performance with deep inheritance hierarchies.

**MCP Tool**: `create_blueprint` (10 sequential calls)

**Payload Pattern** (repeat for Level_01 through Level_10):
```json
{
  "name": "BP_Level_<N>",
  "parent_class": "BP_Level_<N-1>",
  "folder_path": "Testing/Performance"
}
```

Start with:
```json
{
  "name": "BP_Level_01",
  "parent_class": "Actor",
  "folder_path": "Testing/Performance"
}
```

**Expected Result**:
- All 10 levels created successfully
- Each level resolves parent correctly
- Final blueprint (BP_Level_10) has 10-level inheritance chain

**Validation** (Get metadata from deepest level):
```json
{
  "blueprint_name": "BP_Level_10",
  "fields": ["parent_class"]
}
```

Expected inheritance_chain length: 12+ (10 Blueprint levels + Actor + Object)

---

### Test 4.2: Large Metadata Query

**Objective**: Test performance of retrieving all metadata from complex Blueprint.

**Setup**: Create Blueprint with many components, variables, functions

**MCP Tool**: `get_blueprint_metadata`

**Payload**:
```json
{
  "blueprint_name": "BP_ComplexBlueprint",
  "fields": ["*"]
}
```

**Expected Result**:
- Response returned within reasonable time (<5 seconds)
- All fields populated
- No timeouts or errors

---

## Phase 5: Integration Tests

### Test 5.1: Complete Workflow - Enemy Hierarchy

**Objective**: Real-world use case - create enemy class hierarchy.

**Workflow**:

1. Create base enemy:
```json
{
  "name": "BP_Enemy",
  "parent_class": "Character",
  "folder_path": "Game/Enemies"
}
```

2. Add base components:
```json
{
  "blueprint_name": "BP_Enemy",
  "component_type": "SphereComponent",
  "component_name": "DetectionRadius",
  "location": [0, 0, 0]
}
```

3. Add base variables:
```json
{
  "blueprint_name": "BP_Enemy",
  "variable_name": "Health",
  "variable_type": "Float",
  "is_exposed": true
}
```

4. Create melee enemy variant:
```json
{
  "name": "BP_Enemy_Melee",
  "parent_class": "BP_Enemy",
  "folder_path": "Game/Enemies"
}
```

5. Create ranged enemy variant:
```json
{
  "name": "BP_Enemy_Ranged",
  "parent_class": "BP_Enemy",
  "folder_path": "Game/Enemies"
}
```

6. Verify inheritance in both:
```json
{
  "blueprint_name": "BP_Enemy_Melee",
  "fields": ["parent_class", "components", "variables"]
}
```

**Expected Result**:
- Both variants inherit DetectionRadius component and Health variable
- Parent class correctly shows BP_Enemy
- Inheritance chain includes Character → Pawn → Actor → Object

---

### Test 5.2: Interface Contract Implementation

**Objective**: Test interface implementation metadata tracking.

**Workflow**:

1. Create damage interface:
```json
{
  "name": "BPI_Damageable",
  "folder_path": "Game/Interfaces"
}
```

2. Create test actor:
```json
{
  "name": "BP_DamageableActor",
  "parent_class": "Actor",
  "folder_path": "Game/Testing"
}
```

3. Add interface to actor:
```json
{
  "blueprint_name": "BP_DamageableActor",
  "interface_name": "BPI_Damageable"
}
```

4. Check implementation status:
```json
{
  "blueprint_name": "BP_DamageableActor",
  "fields": ["interfaces"]
}
```

**Expected Result**:
- Interface listed in metadata
- Functions listed with implemented: false (since we only added interface, didn't implement functions)
- Can track contract compliance

---

## Success Criteria Summary

### Phase 1 Success Criteria:
- ✅ Blueprints can be created with other Blueprints as parents
- ✅ Both simple names and full paths work for Blueprint parents
- ✅ Multi-level inheritance chains work correctly
- ✅ Child Blueprints inherit components and variables
- ✅ Child Blueprints compile successfully

### Phase 2 Success Criteria:
- ✅ All 10 metadata fields can be queried
- ✅ Selective field querying works (performance optimization)
- ✅ Parent class metadata shows Blueprint vs Native distinction
- ✅ Inheritance chains fully visible through metadata
- ✅ Interface implementation status tracked
- ✅ Asset information retrievable

### Phase 3 Success Criteria:
- ✅ Graceful error handling for missing Blueprints
- ✅ Fallback to AActor for invalid Blueprint parents
- ✅ Invalid field names handled without errors
- ✅ Interface Blueprints cannot be used as parents

### Phase 4 Success Criteria:
- ✅ Deep inheritance chains (10+ levels) supported
- ✅ Large metadata queries complete in reasonable time
- ✅ No performance degradation with complex hierarchies

### Phase 5 Success Criteria:
- ✅ Real-world workflows function correctly
- ✅ Interface contract tracking works
- ✅ Enemy hierarchy example succeeds end-to-end

---

## Notes for Test Execution

1. **Execute tests in order** - Later tests depend on Blueprints created in earlier tests
2. **Check Unreal Editor** - After each test, optionally verify in editor that hierarchy is correct
3. **Log files** - Check `MCPGameProject/Saved/Logs/MCPGameProject.log` for detailed logging
4. **Cleanup** - Delete Testing folder between test runs for consistent results: `/Game/Testing/`
5. **Performance** - Note response times for metadata queries, should be <1 second for most queries

## Troubleshooting

If tests fail:
1. Ensure Unreal Editor is running with plugin loaded
2. Check that Python MCP server is connected
3. Verify TCP connection on 127.0.0.1:55557
4. Check log files for detailed error messages
5. Ensure no existing Blueprints conflict with test names
