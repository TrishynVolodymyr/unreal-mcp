# Dialogue System Implementation Plan

## Overview
Build a complete dialogue system using natural language commands through the Unreal MCP plugin: widgets for displaying dialogue, DataTable for content, NPC detection radius on ThirdPersonCharacter, interaction popup above NPC, and dialogue display on key press. All implementation will be done via AI commands to the MCP server to test the plugin's functionality and document any issues encountered.

## Task Requirements (from User)
1. Create a simple widget for displaying dialogue text
2. Create structure and datatable for dialogues with example English text
3. Add radius detection to ThirdPersonCharacter for dialogue-capable NPCs
4. Add popup above NPC when in range showing interaction prompt with key binding
5. On interaction key press, display dialogue widget on screen

## Implementation Steps

### Step 1: Create Dialogue UI Widget and Data Structures
**Goal**: Create `WBP_DialogueBox` widget and `DT_Dialogues` DataTable with English example content

**Actions**:
- Use `umg_mcp_server.py` to create `WBP_DialogueBox` widget blueprint
- Add TextBlock components for dialogue text display
- Use `datatable_mcp_server.py` to create `DialogueEntry` struct with fields:
  - DialogueID (String)
  - DialogueText (String)
  - SpeakerName (String)
  - NextDialogueID (String)
- Create `DT_Dialogues` DataTable based on struct
- Add English example dialogue rows (greeting, merchant conversation, quest dialogue)

**Files Modified**:
- `MCPGameProject/Content/UI/WBP_DialogueBox.uasset` (new)
- `MCPGameProject/Content/Data/DialogueEntry.uasset` (new)
- `MCPGameProject/Content/Data/DT_Dialogues.uasset` (new)

**Tools**: `create_umg_widget_blueprint()`, `add_widget_component_to_widget()`, `create_struct()`, `create_datatable()`, `add_rows_to_datatable()`

---

### Step 2: Add Interaction Radius to ThirdPersonCharacter
**Goal**: Add sphere collision component for detecting nearby dialogue NPCs

**Actions**:
- Use `blueprint_mcp_server.py` to add `SphereComponent` named `InteractionRadius` to `BP_ThirdPersonCharacter`
- Set sphere radius to 200.0 units
- Configure collision profile for overlap detection (OverlapAll)
- Add Blueprint variables:
  - `CurrentInteractableNPC` (Actor reference)
  - `DialogueWidgetClass` (Class<UserWidget>, exposed to editor)
  - `CurrentDialogueWidget` (UserWidget reference)

**Files Modified**:
- `MCPGameProject/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`

**Tools**: `add_component_to_blueprint()`, `set_component_property()`, `add_blueprint_variable()`

---

### Step 3: Create NPC Blueprint with Interaction Prompt
**Goal**: Create `BP_DialogueNPC` with widget component showing interaction prompt above head

**Actions**:
- Create new Actor Blueprint `BP_DialogueNPC` (parent class: Actor)
- Add StaticMeshComponent for visual representation
- Add `WidgetComponent` named `InteractionPrompt`:
  - Position: [0, 0, 100] (above actor)
  - Widget class: Create simple `WBP_InteractionPrompt` with text "Press E to Talk"
  - Initial visibility: hidden
  - Space: Screen

**Files Modified**:
- `MCPGameProject/Content/Blueprints/BP_DialogueNPC.uasset` (new)
- `MCPGameProject/Content/UI/WBP_InteractionPrompt.uasset` (new)

**Tools**: `create_blueprint()`, `add_component_to_blueprint()`, `create_umg_widget_blueprint()`, `add_widget_component_to_widget()`

---

### Step 4: Implement Overlap Detection and Prompt Logic
**Goal**: Show/hide interaction prompt when player enters/exits NPC radius

**Actions**:
- In `BP_ThirdPersonCharacter`, create event graph nodes:
  - `ComponentBeginOverlap` event (for InteractionRadius component)
  - Cast to `BP_DialogueNPC` node
  - Branch node to check if cast succeeded
  - Set `CurrentInteractableNPC` variable
  - Get `InteractionPrompt` component from NPC
  - Set Widget Component Visibility to true
- Create `ComponentEndOverlap` event with mirror logic to hide prompt

**Files Modified**:
- `MCPGameProject/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`

**Tools**: `create_node_by_action_name()`, `find_blueprint_nodes()`, `connect_blueprint_nodes()`

**Node Creation Sequence**:
1. Create all event and function nodes
2. Use `find_blueprint_nodes()` to get node IDs
3. Use `connect_blueprint_nodes()` for batch connection

---

### Step 5: Add Input Handling and Dialogue Display
**Goal**: Display dialogue widget when player presses interaction key while near NPC

**Actions**:
- Verify `IA_Interact` Enhanced Input Action exists in `MCPGameProject/Content/ThirdPerson/Input/Actions/`
- In `BP_ThirdPersonCharacter`, create event graph nodes:
  - `IA_Interact` Enhanced Input Action event
  - Branch node to check if `CurrentInteractableNPC` is valid
  - "Create Widget" node (WidgetBlueprintLibrary):
    - Class: `DialogueWidgetClass` variable
    - Owning Player: Get Player Controller
  - Set `CurrentDialogueWidget` variable
  - "Add to Viewport" node with Z-order 10
- Connect all nodes in logical flow
- Compile both `BP_ThirdPersonCharacter` and `BP_DialogueNPC`

**Files Modified**:
- `MCPGameProject/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`

**Tools**: `create_node_by_action_name()`, `find_blueprint_nodes()`, `connect_blueprint_nodes()`, `compile_blueprint()`

---

### Step 6: Document All Issues Encountered
**Goal**: Create comprehensive log of plugin usability issues for improvement

**Actions**:
- Create `dialogue_system_implementation_log.md` file
- Document each issue with:
  - **Issue Description**: Clear explanation of problem
  - **Reproduction Steps**: Exact commands/actions taken
  - **Expected Behavior**: What should have happened
  - **Actual Behavior**: What actually happened
  - **Workaround**: Any solution found (if applicable)
  - **Severity**: Critical/High/Medium/Low
  - **Category**: Error/Unclear Workflow/Missing Feature/Usability

**Files Created**:
- `dialogue_system_implementation_log.md`

---

## Technical Considerations

### Node Connection Strategy
**Decision**: Create all nodes first, then batch connect
**Rationale**: Allows collecting all node IDs before connection, reduces errors from missing IDs

### NPC Blueprint Design
**Decision**: Use Actor Blueprint (not Character)
**Rationale**: NPCs don't need movement/character functionality, simpler Actor suffices

### Widget Display Method
**Decision**: Start with WidgetComponent for 3D popup, use Add to Viewport for dialogue box
**Rationale**: WidgetComponent is simpler for world-space prompts, viewport widgets better for full-screen UI

### Enhanced Input Verification
**Decision**: Verify `IA_Interact` exists before creating event nodes
**Rationale**: Avoids node creation errors if action missing

---

## Available Tools Reference

### Widget Tools (`umg_mcp_server.py`)
- `create_umg_widget_blueprint(widget_name, parent_class, path)`
- `add_widget_component_to_widget(widget_name, component_name, component_type, position, size, text, font_size)`
- `add_widget_to_viewport(widget_name, z_order)`
- `bind_widget_component_event(widget_name, widget_component_name, event_name)`

### DataTable Tools (`datatable_mcp_server.py`)
- `create_struct(struct_name, properties, path, description)`
- `create_datatable(datatable_name, row_struct_name, path, description)`
- `get_datatable_row_names(datatable_path)`
- `add_rows_to_datatable(datatable_path, rows)`

### Blueprint Tools (`blueprint_mcp_server.py`)
- `create_blueprint(name, parent_class, folder_path)`
- `add_component_to_blueprint(blueprint_name, component_type, component_name, location, rotation, scale, component_properties)`
- `set_component_property(blueprint_name, component_name, kwargs)`
- `add_blueprint_variable(blueprint_name, variable_name, variable_type, is_exposed)`
- `compile_blueprint(blueprint_name)`

### Node Tools (`node_mcp_server.py`, `blueprint_action_mcp_server.py`)
- `create_node_by_action_name(blueprint_name, function_name, class_name, node_position, target_graph, **kwargs)`
- `find_blueprint_nodes(blueprint_name, node_type, event_type, target_graph)`
- `connect_blueprint_nodes(blueprint_name, connections, target_graph)`
- `search_blueprint_actions(search_query, category, max_results)`

### Project Tools (`project_mcp_server.py`)
- `create_enhanced_input_action(action_name, path, description, value_type)`
- `create_input_mapping_context(context_name, path, description)`

---

## Expected Challenges

1. **Node Pin Naming**: May need to use `inspect_node_pin_connection()` to find exact pin names for connections
2. **Component Property Format**: `set_component_property()` requires JSON string format via `kwargs` parameter
3. **DataTable GUID Properties**: Must call `get_datatable_row_names()` before adding rows to get GUID-based field names
4. **Widget Component Visibility**: Setting visibility on widget components may require specific property names
5. **Blueprint Compilation Errors**: May encounter missing connection errors requiring iterative fixing

---

## Success Criteria

- [ ] `WBP_DialogueBox` widget created with TextBlock component
- [ ] `DT_Dialogues` DataTable created with at least 3 English dialogue entries
- [ ] `BP_ThirdPersonCharacter` has `InteractionRadius` SphereComponent (radius 200)
- [ ] `BP_DialogueNPC` created with widget component for interaction prompt
- [ ] Overlap detection logic implemented (BeginOverlap/EndOverlap events)
- [ ] Input action handling implemented (IA_Interact event)
- [ ] Widget display logic implemented (Create Widget + Add to Viewport)
- [ ] Both Blueprints compile without errors
- [ ] All issues documented in log file with reproduction steps

---

## Next Actions

1. Start with Step 1 (Widget and DataTable creation) - standalone, no dependencies
2. Proceed to Step 2 (Character component setup) - prepares for event logic
3. Create NPC blueprint (Step 3) - requires completion of Step 1 for widget reference
4. Implement overlap logic (Step 4) - requires Steps 2-3 complete
5. Add input handling (Step 5) - final integration of all components
6. Continuously document issues (Step 6) - ongoing throughout process
