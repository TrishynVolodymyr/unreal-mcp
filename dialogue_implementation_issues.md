# Dialogue Implementation - Issues and Bugs Found

## Testing Date
November 2, 2025

## Summary
Testing the creation of dialogue functionality for Third Person Character revealed several issues and inconveniences with the Unreal MCP plugin.

## Issues Found

### 1. DataTable Struct Path Resolution
**Status**: ✅ RESOLVED (workaround found)

**Description**: When creating a DataTable, using just the struct name doesn't work. Need to use full path.

**Steps to Reproduce**:
```python
mcp_datatablemcp_create_datatable(
    datatable_name="DT_Dialogues",
    row_struct_name="DialogueData"  # This fails
)
```

**Error**:
```
Struct 'DialogueData' not found. Tried paths: ['/Script/Engine.DialogueData', '/Script/CoreUObject.DialogueData', ...]
```

**Workaround**:
Use full path: `/Game/Dialogue/DialogueData`

**Recommendation**: Plugin should auto-resolve recently created structs or search in project folders.

---

### 2. Cannot Add Existing Component to Parent Widget
**Status**: ⚠️ PARTIAL - Functionality Limited

**Description**: The `add_child_widget_component_to_parent` function fails when trying to add an existing component to a parent container.

**Steps to Reproduce**:
```python
# Create component first
mcp_umgmcp_add_widget_component_to_widget(
    widget_name="WBP_DialogueWidget",
    component_name="SpeakerNameText",
    component_type="TextBlock"
)

# Try to add it to parent - FAILS
mcp_umgmcp_add_child_widget_component_to_parent(
    widget_name="WBP_DialogueWidget",
    parent_component_name="DialogueContainer",
    child_component_name="SpeakerNameText"
)
```

**Error**:
```
Command execution failed
Failed to add child widget 'SpeakerNameText' to parent 'DialogueContainer'
error_code: 3002
```

**Workaround**: Use `create_parent_and_child_widget_components` to create both at once.

**Recommendation**: Either fix the function to properly reparent existing components, or clearly document that it only works with new components.

---

### 3. Cannot Set Variable Value Immediately After Creation
**Status**: ✅ RESOLVED (workaround found)

**Description**: Setting a Blueprint variable value right after creating it fails. Need to compile first.

**Steps to Reproduce**:
```python
mcp_blueprintmcp_add_blueprint_variable(
    blueprint_name="BP_DialogueNPC",
    variable_name="InteractionRadius",
    variable_type="Float"
)

# This fails immediately after creation
mcp_blueprintmcp_set_blueprint_variable_value(
    blueprint_name="BP_DialogueNPC",
    variable_name="InteractionRadius",
    value=500
)
```

**Error**:
```
Property 'InteractionRadius' not found on object 'Default__BP_DialogueNPC_C'
```

**Workaround**: Call `compile_blueprint` before setting value.

**Recommendation**: Auto-compile when setting variable values, or better handle uncompiled state.

---

### 4. For Each Loop Node Creation
**Status**: ✅ RESOLVED

**Description**: Creating "For Each Loop (Array)" fails. The correct name is just "For Each Loop".

**Steps to Reproduce**:
```python
mcp_blueprintacti_create_node_by_action_name(
    function_name="For Each Loop (Array)"  # WRONG
)
```

**Solution**:
```python
mcp_blueprintacti_create_node_by_action_name(
    function_name="For Each Loop"  # CORRECT
)
```

**Recommendation**: Better error messages or fuzzy matching for common node names.

---

### 5. GetActorLocation Function Name Mismatch
**Status**: ✅ RESOLVED

**Description**: The actual function name is `K2_GetActorLocation`, not `GetActorLocation`.

**Steps to Reproduce**:
```python
mcp_blueprintacti_create_node_by_action_name(
    function_name="GetActorLocation",  # WRONG
    class_name="Actor"
)
```

**Error**:
```
Function 'GetActorLocation' not found
```

**Solution**:
Use `search_blueprint_actions` first to find correct name: `K2_GetActorLocation`

**Recommendation**: Add aliases for common K2_ prefixed functions.

---

### 6. No Built-in "Self" Reference Node
**Status**: ⚠️ WORKAROUND EXISTS

**Description**: Cannot create a "Get Self" or "This" reference node. In Blueprint Editor, pins default to self when unplugged.

**Attempted**:
```python
mcp_blueprintacti_create_node_by_action_name(
    function_name="Get",
    kwargs={"variable_name": "self"}  # FAILS
)
```

**Error**:
```
Variable or component 'self' not found
```

**Workaround**: Leave 'self' pins unconnected - they automatically reference self.

**Recommendation**: Add a dedicated tool for creating self-reference nodes, or document this behavior.

---

### 7. IsValid Node Has Wrong Signature
**Status**: ⚠️ CONFUSING

**Description**: Creating IsValid node returns a variant with unexpected pins (Image struct input instead of Object input).

**Steps to Reproduce**:
```python
mcp_blueprintacti_create_node_by_action_name(
    function_name="IsValid"
)
```

**Result**: Gets `SharedImageConstRefBlueprintFns::IsValid` instead of object validation.

**Workaround**: Use comparison operators or Branch directly.

**Recommendation**: When multiple functions share a name, prefer common/utility variants by default (like KismetSystemLibrary functions).

---

### 8. Cannot Create Enhanced Input Action Event Nodes
**Status**: ❌ BLOCKING

**Description**: Cannot create event nodes for Enhanced Input Actions (IA_Interact, IA_Jump, etc.).

**Steps to Reproduce**:
```python
mcp_blueprintacti_create_node_by_action_name(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="IA_Interact"
)
```

**Error**:
```
Function 'IA_Interact' not found and not a recognized control flow node
```

**Status**: This is a critical gap. Enhanced Input is the standard input system in UE5.

**Recommendation**: Add support for creating Enhanced Input Action event nodes. These should be discoverable through `search_blueprint_actions` with "IA_" prefix.

---

## Implementation Progress

### ✅ Completed
1. Created DialogueData struct with SpeakerName, DialogueText, ResponseOptions
2. Created DataTable (DT_Dialogues) with 3 sample dialogue entries
3. Created WBP_DialogueWidget with Border and TextBlock components
4. Created WBP_InteractionPrompt with "Press E to talk" UI
5. Created BP_DialogueNPC Character with WidgetComponent and SphereComponent
6. Set WidgetComponent to display WBP_InteractionPrompt
7. Created CheckForNearbyNPC function in BP_ThirdPersonCharacter
   - Uses GetAllActorsOfClass to find BP_DialogueNPC actors
   - For Each Loop through actors
   - Calculates distance between player and each NPC
   - Returns first NPC within 500 units
8. Added Event Tick logic to call CheckForNearbyNPC
9. Set CurrentDialogueNPC variable when NPC found

### ⚠️ Blocked/Incomplete
1. **Cannot create IA_Interact input event** - This is blocking the interaction trigger
2. Need to show/hide interaction prompt based on proximity
3. Need to create and display dialogue widget on interaction
4. Need to populate dialogue widget with DataTable data

## Workarounds Used
- Compiling Blueprint after variable creation before setting values
- Using full paths for struct references
- Using `search_blueprint_actions` to find correct function names
- Leaving self pins unconnected instead of explicit self reference

## Recommended Plugin Improvements
1. Auto-compile Blueprints when necessary (before setting values, etc.)
2. Better path resolution for recently created assets
3. Support for Enhanced Input Action event node creation
4. Function name aliases for K2_ prefixed functions
5. Better default function selection when multiple variants exist
6. Fix or document widget component reparenting limitations
7. Add tool for creating self-reference nodes
