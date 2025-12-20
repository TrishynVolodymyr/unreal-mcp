# Dialogue System Implementation Plan

## ⚠️ CRITICAL REQUIREMENT: MCP TOOLS ONLY

**THIS ENTIRE IMPLEMENTATION MUST BE DONE EXCLUSIVELY USING MCP TOOLS.**

**NO FILES SHOULD BE CREATED OR MODIFIED MANUALLY.**

**The sole purpose of this plan is to test the Unreal MCP plugin's capabilities. The entire project exists to create an MCP plugin for Unreal Engine that enables AI-driven development. This dialogue system is a comprehensive test case to validate that AI can build complete gameplay systems through MCP tools alone.**

**If any step cannot be completed with available MCP tools, document the limitation and report it - DO NOT work around it manually.**

---

## Overview
A simple dialogue system for testing the Unreal MCP plugin. Allows the Third Person Character to interact with NPCs by pressing E key when within interaction radius. Shows a dialogue widget with NPC name and multiple answer options.

## Architecture

### Components
1. **Dialogue Data Structure** (C++ Struct)
2. **Dialogue Data Table** (one per NPC)
3. **Dialogue Actor Component** (C++ Component)
4. **Base NPC Blueprint** (Blueprint Actor)
5. **Dialogue Widget Blueprint** (UMG Widget)
6. **Interaction Indicator Widget** (UMG Widget - "Press E")

---

## Phase 1: Data Structure & DataTable

### 1.1 Create Dialogue Answer Structure

**Using MCP Tool**: `create_struct`

```python
# MCP command (AI MUST execute this - NO manual creation allowed)
create_struct(
    struct_name="DialogueAnswer",
    properties=[
        {
            "name": "AnswerText",
            "type": "String",
            "description": "The text displayed for this answer option"
        },
        {
            "name": "AnswerID",
            "type": "Name",
            "description": "Optional ID for future expansion (branching, actions, etc.)"
        }
    ],
    path="/Game/Dialogue/Structs",
    description="Represents a single answer option in a dialogue"
)
```

### 1.2 Create Dialogue Row Structure

**Using MCP Tool**: `create_struct`

```python
# MCP command (AI MUST execute this - NO manual creation allowed)
create_struct(
    struct_name="DialogueRow",
    properties=[
        {
            "name": "NPCText",
            "type": "String",
            "description": "The NPC's dialogue text (question or statement)"
        },
        {
            "name": "PlayerAnswers",
            "type": "DialogueAnswer[]",
            "description": "List of answer options the player can choose from"
        }
    ],
    path="/Game/Dialogue/Structs",
    description="Represents a dialogue entry (NPC question/statement + player answer options)"
)
```

### 1.3 Create DataTable Asset

**Using MCP Tool**: `create_datatable`

```python
# MCP command (AI MUST execute this - NO manual creation allowed)
create_datatable(
    name="DT_TestNPCDialogue",
    struct_name="DialogueRow",
    folder_path="/Game/Dialogue/DataTables"
)
```

**NO MANUAL SETUP ALLOWED - MCP TOOLS ONLY**

### 1.4 Populate Sample Dialogue Data

Add rows to `DT_TestNPCDialogue`:

| Row Name | NPC Text | Player Answers |
|----------|----------|----------------|
| Greeting | "Hello, traveler! What brings you here?" | [0]: "Just passing through." [1]: "I'm looking for adventure." [2]: "Goodbye." |
| ShopTalk | "Would you like to see my wares?" | [0]: "Yes, show me." [1]: "Not right now." |

---

## Phase 2: Base NPC Blueprint with Dialogue Logic

**NOTE**: Instead of creating a C++ DialogueComponent, we'll implement dialogue logic entirely in Blueprint using MCP tools. This proves the MCP plugin can handle complex gameplay systems without manual C++ coding.

### 2.1 Create BP_DialogueNPC Blueprint

**Using MCP Tool**: `create_blueprint`

```python
create_blueprint(
    name="BP_DialogueNPC",
    parent_class="/Script/Engine.Actor",
    folder_path="/Game/Dialogue/Blueprints"
)
```

### 2.2 Add Blueprint Variables

**Using MCP Tool**: `add_blueprint_variable`

```python
# NPC Name variable
add_blueprint_variable(
    blueprint_name="BP_DialogueNPC",
    variable_name="NPCName",
    variable_type="String",
    is_exposed=True
)

# Dialogue DataTable reference
add_blueprint_variable(
    blueprint_name="BP_DialogueNPC",
    variable_name="DialogueTable",
    variable_type="DataTable",
    is_exposed=True
)

# Interaction radius
add_blueprint_variable(
    blueprint_name="BP_DialogueNPC",
    variable_name="InteractionRadius",
    variable_type="Float",
    is_exposed=True
)

# Is dialogue active flag
add_blueprint_variable(
    blueprint_name="BP_DialogueNPC",
    variable_name="bDialogueActive",
    variable_type="Boolean",
    is_exposed=False
)
```

### 2.3 Add Components to BP_DialogueNPC

**Using MCP Tool**: `add_component_to_blueprint`

```python
# Static Mesh Component (Root) - representing the NPC body
add_component_to_blueprint(
    blueprint_name="BP_DialogueNPC",
    component_type="StaticMeshComponent",
    component_name="NPCMesh",
    location=[0, 0, 0],
    rotation=[0, 0, 0],
    scale=[1, 1, 2]  # Tall to represent humanoid
)

# Set the mesh to a simple cube
set_static_mesh_properties(
    blueprint_name="BP_DialogueNPC",
    component_name="NPCMesh",
    static_mesh="/Engine/BasicShapes/Cube.Cube"
)

# Sphere Component for interaction detection
add_component_to_blueprint(
    blueprint_name="BP_DialogueNPC",
    component_type="SphereComponent",
    component_name="InteractionSphere",
    location=[0, 0, 0],
    rotation=[0, 0, 0],
    scale=[1, 1, 1]
)

# Set sphere radius to match interaction radius (300cm)
set_component_property(
    blueprint_name="BP_DialogueNPC",
    component_name="InteractionSphere",
    kwargs='{"SphereRadius": 300.0}'
)

# Widget Component for "Press E" indicator
add_component_to_blueprint(
    blueprint_name="BP_DialogueNPC",
    component_type="WidgetComponent",
    component_name="InteractionWidget",
    location=[0, 0, 120],  # Above NPC head
    rotation=[0, 0, 0],
    scale=[1, 1, 1]
)
```

### 2.4 Blueprint Event Graph Logic

**NOTE**: The following Blueprint node graph logic will need to be created using `node_tools` MCP commands. This includes:

**Event BeginPlay**:
- Set InteractionWidget visibility to false
- Get Player Character and store as variable
- Set default values (InteractionRadius = 300, NPCName = "Test NPC")

**Event Tick**:
- If NOT bDialogueActive:
  - Get distance between player and self
  - If distance <= InteractionRadius: Show InteractionWidget
  - Else: Hide InteractionWidget

**Custom Function: StartDialogue**:
- Check if DialogueTable is valid
- Check if NOT bDialogueActive
- If valid: Set bDialogueActive = true, Hide InteractionWidget, Get first row from DialogueTable, Broadcast OnDialogueStarted event

**Custom Function: EndDialogue**:
- Set bDialogueActive = false
- Show InteractionWidget if player still in range

**NOTE**: Detailed node creation will be added in implementation phase using `create_node`, `connect_nodes`, etc.

### 2.5 ⚠️ COMPILATION CHECKPOINT #1

**CRITICAL**: After completing all node creation and connections for BP_DialogueNPC, compile the blueprint immediately.

```python
# Compile BP_DialogueNPC to verify all nodes and connections are valid
compile_blueprint(blueprint_name="BP_DialogueNPC")
```

**Verify**:
- [ ] Compilation succeeds without errors
- [ ] No "unconnected pin" warnings for required pins
- [ ] No type mismatch errors between connected pins
- [ ] All custom functions compile correctly

**If compilation fails**:
1. Document the specific error message
2. Identify which node or connection caused the issue
3. Fix the problem before proceeding to Phase 3
4. Re-compile until successful

**DO NOT proceed to Phase 3 until BP_DialogueNPC compiles successfully.**

---

## Phase 3: Widget Blueprints

### 3.1 Create Interaction Indicator Widget

**Using MCP Tool**: `create_widget_blueprint`

```python
# Create the widget blueprint
create_widget_blueprint(
    name="WBP_InteractionIndicator",
    folder_path="/Game/Dialogue/UI"
)

# Add Canvas Panel (root)
add_widget_to_blueprint(
    blueprint_name="WBP_InteractionIndicator",
    widget_type="CanvasPanel",
    widget_name="CanvasPanel_Root"
)

# Add Text Block for "Press E to Talk"
add_widget_to_blueprint(
    blueprint_name="WBP_InteractionIndicator",
    widget_type="TextBlock",
    widget_name="TXT_InteractionPrompt",
    parent_name="CanvasPanel_Root"
)

# Set text properties
set_widget_property(
    blueprint_name="WBP_InteractionIndicator",
    widget_name="TXT_InteractionPrompt",
    properties={
        "Text": "Press E to Talk",
        "FontSize": 18,
        "Justification": "Center"
    }
)
```

**Simple Design**: Just centered text, no fancy styling needed for testing.

### 3.1.1 ⚠️ COMPILATION CHECKPOINT #2a

```python
# Compile WBP_InteractionIndicator to verify widget structure
compile_blueprint(blueprint_name="WBP_InteractionIndicator")
```

**Verify**:
- [ ] Widget compiles without errors
- [ ] Widget hierarchy is valid (CanvasPanel → TextBlock)

### 3.2 Create Dialogue Widget

**Using MCP Tool**: `create_widget_blueprint` (via UMG tools)

```python
# Create main dialogue widget
create_widget_blueprint(
    name="WBP_DialogueWindow",
    folder_path="/Game/Dialogue/UI"
)

# Add widget hierarchy using umg_tools
# Canvas Panel → Border → Vertical Box → Text blocks and Scroll Box
# (Detailed widget hierarchy creation via MCP tools to be done during implementation)
```

**Widget Structure** (to be created with MCP tools):
- Canvas Panel (root)
  - Border (background dimming)
    - Vertical Box
      - Text Block: TXT_NPCName (Font Size: 24)
      - Text Block: TXT_NPCDialogue (Font Size: 18, Auto Wrap)
      - Scroll Box: ScrollBox_Answers (for answer buttons)

### 3.3 Create Answer Button Widget

**Using MCP Tool**: `create_widget_blueprint`

```python
# Create answer button widget
create_widget_blueprint(
    name="WBP_AnswerButton",
    folder_path="/Game/Dialogue/UI"
)

# Add Button component with Text Block child
# (Detailed widget creation via MCP tools to be done during implementation)

# Add variables
add_blueprint_variable(
    blueprint_name="WBP_AnswerButton",
    variable_name="AnswerIndex",
    variable_type="Integer",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_AnswerButton",
    variable_name="DialogueWidget",
    variable_type="WBP_DialogueWindow",
    is_exposed=False
)
```

**Widget Structure** (to be created with MCP tools):
- Button: BTN_Answer
  - Text Block: TXT_AnswerText (Font Size: 16)

**Event Graph** (to be created with node_tools):
- BTN_Answer.OnClicked → Call DialogueWidget.OnAnswerSelected(AnswerIndex)

### 3.3.1 ⚠️ COMPILATION CHECKPOINT #2b

```python
# Compile WBP_AnswerButton to verify widget structure and event binding
compile_blueprint(blueprint_name="WBP_AnswerButton")
```

**Verify**:
- [ ] Widget compiles without errors
- [ ] Button OnClicked event is properly bound
- [ ] Widget hierarchy is valid (Button → TextBlock)

### 3.4 Add Variables to Dialogue Widget

**Using MCP Tool**: `add_blueprint_variable`

```python
# Reference to the NPC Blueprint
add_blueprint_variable(
    blueprint_name="WBP_DialogueWindow",
    variable_name="NPCReference",
    variable_type="BP_DialogueNPC",
    is_exposed=False
)

# Reference to answer button class
add_blueprint_variable(
    blueprint_name="WBP_DialogueWindow",
    variable_name="AnswerButtonClass",
    variable_type="Class<WBP_AnswerButton>",
    is_exposed=True
)

# Dialogue data
add_blueprint_variable(
    blueprint_name="WBP_DialogueWindow",
    variable_name="CurrentDialogueData",
    variable_type="DialogueRow",
    is_exposed=False
)
```

### 3.5 Implement Widget Functions

**Using MCP Tool**: `create_custom_blueprint_function` and `node_tools`

```python
# Create Initialize Dialogue function
create_custom_blueprint_function(
    blueprint_name="WBP_DialogueWindow",
    function_name="InitializeDialogue",
    inputs=[
        {"name": "NPCRef", "type": "BP_DialogueNPC"},
        {"name": "DialogueData", "type": "DialogueRow"}
    ]
)

# Create On Answer Selected function
create_custom_blueprint_function(
    blueprint_name="WBP_DialogueWindow",
    function_name="OnAnswerSelected",
    inputs=[
        {"name": "AnswerIndex", "type": "Integer"}
    ]
)
```

**Function Logic** (to be implemented with node_tools during implementation):

**InitializeDialogue**:
1. Store NPCRef and DialogueData
2. Set TXT_NPCName text from NPCRef.NPCName
3. Set TXT_NPCDialogue text from DialogueData.NPCText
4. Clear ScrollBox_Answers
5. For Each PlayerAnswer: Create WBP_AnswerButton widget and add to ScrollBox
6. Set input mode to UI Only

**OnAnswerSelected**:
1. Call NPCReference.EndDialogue()
2. Remove widget from parent
3. Set input mode to Game Only

### 3.6 ⚠️ COMPILATION CHECKPOINT #2c

```python
# Compile WBP_DialogueWindow to verify all functions and widget bindings
compile_blueprint(blueprint_name="WBP_DialogueWindow")
```

**Verify**:
- [ ] Widget compiles without errors
- [ ] InitializeDialogue function compiles correctly
- [ ] OnAnswerSelected function compiles correctly
- [ ] All variable types are valid (NPCReference, AnswerButtonClass, CurrentDialogueData)
- [ ] Widget hierarchy is valid

**If any widget compilation fails**:
1. Document the specific error message
2. Check widget hierarchy parent-child relationships
3. Verify variable types match expected Blueprint/Struct types
4. Fix issues before proceeding to Phase 4

**DO NOT proceed to Phase 4 until ALL widget blueprints compile successfully.**

---

## Phase 4: Player Interaction Setup

### 4.1 Check Existing Enhanced Input Mappings

**Using MCP Tool**: `list_input_actions` and `list_input_mapping_contexts`

```python
# Check what Input Actions exist
list_input_actions(path="/Game")

# Check what Input Mapping Contexts exist
list_input_mapping_contexts(path="/Game")
```

### 4.2 Create Enhanced Input Action (if not exists)

**Using MCP Tool**: `create_enhanced_input_action`

```python
# Create Interact action
create_enhanced_input_action(
    action_name="IA_Interact",
    path="/Game/Input/Actions",
    value_type="Digital"
)
```

### 4.3 Create or Update Input Mapping Context

**Using MCP Tool**: `create_input_mapping_context` and `add_mapping_to_context`

```python
# Create mapping context (if doesn't exist)
create_input_mapping_context(
    context_name="IMC_Default",
    path="/Game/Input"
)

# Add E key mapping to the context
add_mapping_to_context(
    context_path="/Game/Input/IMC_Default",
    action_path="/Game/Input/Actions/IA_Interact",
    key="E"
)
```

### 4.4 Implement Interaction in Player Character

**Using MCP Tool**: `node_tools` (create nodes in player Blueprint)

**Location**: Find player character blueprint using `list_folder_contents` or search

**Event Graph Logic** (to be created with node_tools):

**Enhanced Input Action Event: IA_Interact (Triggered)**:
1. Get Actor Location (Self)
2. Sphere Overlap Actors (Radius: 350, Object Types: WorldDynamic)
3. For Each Overlapped Actor:
   - Check if actor has BP_DialogueNPC class
   - If yes: Call StartDialogue() on that actor
   - Break loop

**NOTE**: Detailed node creation commands will be provided during implementation phase.

### 4.4.1 ⚠️ COMPILATION CHECKPOINT #3

```python
# Compile the player character blueprint after adding interaction logic
compile_blueprint(blueprint_name="BP_ThirdPersonCharacter")  # or actual player BP name
```

**Verify**:
- [ ] Player blueprint compiles without errors
- [ ] IA_Interact Enhanced Input event is properly bound
- [ ] Sphere overlap and cast nodes are correctly connected
- [ ] Call to BP_DialogueNPC.StartDialogue() has correct pin connections

**If compilation fails**:
1. Check that IA_Interact Input Action exists and is valid
2. Verify cast target class matches BP_DialogueNPC exactly
3. Ensure all execution pins are connected in the flow

### 4.5 Connect Dialogue Events in BP_DialogueNPC

**Using MCP Tool**: `node_tools`

**Event Graph additions** (to be created with node_tools):

**Custom Event: OnDialogueStartedInternal**:
1. Create Widget (WBP_DialogueWindow)
2. Add to Viewport (Z-Order: 100)
3. Call InitializeDialogue(self, DialogueData)

**Function: StartDialogue** (call this event when player presses E):
1. Check if DialogueTable is valid
2. Get first row from DialogueTable
3. Trigger OnDialogueStartedInternal event with row data

### 4.6 ⚠️ COMPILATION CHECKPOINT #4 (FINAL)

**CRITICAL**: This is the final compilation checkpoint before testing. ALL blueprints must compile successfully.

```python
# Final compilation of BP_DialogueNPC with all dialogue event logic
compile_blueprint(blueprint_name="BP_DialogueNPC")

# Re-verify all widget blueprints still compile (in case of cross-references)
compile_blueprint(blueprint_name="WBP_InteractionIndicator")
compile_blueprint(blueprint_name="WBP_AnswerButton")
compile_blueprint(blueprint_name="WBP_DialogueWindow")

# Re-verify player blueprint
compile_blueprint(blueprint_name="BP_ThirdPersonCharacter")  # or actual player BP name
```

**Verify ALL of the following**:
- [ ] BP_DialogueNPC compiles without errors
- [ ] OnDialogueStartedInternal custom event works correctly
- [ ] Widget creation and AddToViewport nodes are valid
- [ ] InitializeDialogue call has correct parameters
- [ ] WBP_InteractionIndicator compiles without errors
- [ ] WBP_AnswerButton compiles without errors
- [ ] WBP_DialogueWindow compiles without errors
- [ ] Player character blueprint compiles without errors

**Common issues to check at this stage**:
1. **Type mismatches**: Variable types between blueprints don't match
2. **Missing connections**: Required execution or data pins not connected
3. **Invalid references**: Widget class references point to non-existent assets
4. **Circular dependencies**: Blueprints referencing each other incorrectly

**If ANY compilation fails**:
1. Document the specific error message and blueprint
2. Trace back to identify the root cause
3. Fix the issue in the appropriate blueprint
4. Re-compile ALL blueprints to ensure fix didn't break others
5. Repeat until all compilations succeed

**DO NOT proceed to Phase 5 (Testing) until ALL blueprints compile successfully with ZERO errors.**

---

## Phase 5: Testing Checklist

### 5.1 DataTable Test
- [ ] DialogueAnswer struct created via MCP tool
- [ ] DialogueRow struct created via MCP tool
- [ ] DataTable created with DialogueRow structure via MCP tool
- [ ] At least one dialogue entry with 2+ answer options
- [ ] Row data displays correctly in editor

### 5.2 Blueprint Test
- [ ] BP_DialogueNPC created via MCP tool
- [ ] All variables added via MCP tool (NPCName, DialogueTable, InteractionRadius, bDialogueActive)
- [ ] All components added via MCP tool (NPCMesh, InteractionSphere, InteractionWidget)
- [ ] BP_DialogueNPC spawnable in level
- [ ] Components visible and configured correctly

### 5.3 Widget Test
- [ ] WBP_InteractionIndicator created via MCP tool
- [ ] Shows "Press E to Talk" text
- [ ] WBP_DialogueWindow created via MCP tool
- [ ] WBP_AnswerButton created via MCP tool
- [ ] Widget hierarchies correct

### 5.4 Interaction Test
- [ ] Walk near NPC (within 3 meters) → Interaction indicator appears
- [ ] Walk away → Interaction indicator disappears
- [ ] Press E when in range → Dialogue widget opens
- [ ] Dialogue widget shows correct NPC name
- [ ] Dialogue widget shows NPC text from DataTable
- [ ] All answer options display in scrollable list
- [ ] Click answer → Dialogue closes
- [ ] After closing → Can walk normally, mouse hidden

### 5.5 Input Test
- [ ] Enhanced Input Action created via MCP tool
- [ ] Input Mapping Context created via MCP tool
- [ ] E key mapped correctly via MCP tool
- [ ] Pressing E outside range does nothing
- [ ] Pressing E with no NPC nearby does nothing
- [ ] Input mode switches correctly (game → UI → game)

### 5.6 MCP Tools Verification
- [ ] **ZERO manual file creation** - everything done via MCP tools
- [ ] **ZERO C++ files created manually**
- [ ] **ZERO Unreal Editor manual asset creation**
- [ ] All structs created via `create_struct`
- [ ] All blueprints created via `create_blueprint`
- [ ] All components added via `add_component_to_blueprint`
- [ ] All widgets created via UMG MCP tools
- [ ] All nodes created via `node_tools`

---

## Phase 6: Future Expansion Points

This simple implementation has clear extension points for future features:

### 7.1 Branching Dialogues
- Add `NextDialogueRowName` field to `FDialogueAnswer`
- Modify `GetDialogueData()` to accept row name parameter
- Track conversation state in DialogueComponent

### 7.2 Conditions
- Add `FGameplayTagContainer RequiredTags` to `FDialogueAnswer`
- Check player's gameplay tags before showing answer option
- Hide/gray out answers that don't meet conditions

### 7.3 Actions
- Add `TArray<UDialogueAction*>` to `FDialogueAnswer`
- Create `UDialogueAction` base class (give item, start quest, etc.)
- Execute actions when answer selected

### 7.4 Quest Integration
- Add `QuestID` and `QuestStage` to `FDialogueRow`
- Query quest system to determine which dialogue row to show
- Update quest progress when specific answers chosen

### 7.5 Voice Audio
- Add `USoundBase* VoiceAudio` to `FDialogueRow`
- Play audio when dialogue starts
- Add subtitle timing

### 7.6 Character Portraits
- Add `UTexture2D* Portrait` to DialogueComponent
- Display portrait in dialogue widget
- Support emotion variants (happy, sad, angry)

### 7.7 Multiple NPCs
- Support `TArray<UDialogueComponent*>` for group conversations
- Rotate speakers in dialogue widget
- Show multiple portraits

---

## Implementation Order Summary

**Recommended sequence for AI implementing this (100% MCP TOOLS ONLY)**:

⚠️ **CRITICAL**: Each phase must end with a compilation checkpoint. Do NOT proceed to the next phase until all blueprints compile successfully. This catches errors early when they are easier to diagnose and fix.

1. **Create Data Structures** (Phase 1)
   - Create DialogueAnswer struct via `create_struct`
   - Create DialogueRow struct via `create_struct`
   - Create DT_TestNPCDialogue DataTable via `create_datatable`
   - Populate sample dialogue rows (manually in editor or via datatable_tools)
   - *(No compilation checkpoint - structs/datatables don't compile)*

2. **Build NPC Blueprint** (Phase 2)
   - Create BP_DialogueNPC via `create_blueprint`
   - Add variables via `add_blueprint_variable`
   - Add components via `add_component_to_blueprint`
   - Configure component properties via `set_component_property`
   - Create custom functions via `create_custom_blueprint_function`
   - Implement event graph logic via `node_tools`
   - **Connect all nodes in event graph**
   - ⚠️ **CHECKPOINT #1**: `compile_blueprint(BP_DialogueNPC)` - MUST PASS before Phase 3

3. **Build Widget Blueprints** (Phase 3)
   - Create WBP_InteractionIndicator via UMG MCP tools
   - ⚠️ **CHECKPOINT #2a**: `compile_blueprint(WBP_InteractionIndicator)` - MUST PASS
   - Create WBP_AnswerButton via UMG MCP tools
   - Add button event graph logic via `node_tools`
   - **Connect all nodes**
   - ⚠️ **CHECKPOINT #2b**: `compile_blueprint(WBP_AnswerButton)` - MUST PASS
   - Create WBP_DialogueWindow via UMG MCP tools
   - Add widget variables via `add_blueprint_variable`
   - Create widget functions via `create_custom_blueprint_function`
   - Implement widget event graphs via `node_tools`
   - **Connect all nodes in functions**
   - ⚠️ **CHECKPOINT #2c**: `compile_blueprint(WBP_DialogueWindow)` - MUST PASS before Phase 4

4. **Setup Input System** (Phase 4)
   - Check existing inputs via `list_input_actions` and `list_input_mapping_contexts`
   - Create IA_Interact via `create_enhanced_input_action`
   - Create/update IMC_Default via `create_input_mapping_context`
   - Map E key via `add_mapping_to_context`
   - Add interaction logic to player BP via `node_tools`
   - **Connect all nodes**
   - ⚠️ **CHECKPOINT #3**: `compile_blueprint(PlayerCharacter)` - MUST PASS

5. **Connect Everything** (Phase 4 continued)
   - Add dialogue event handling to BP_DialogueNPC via `node_tools`
   - Wire up widget creation and initialization via `node_tools`
   - **Connect all nodes**
   - ⚠️ **CHECKPOINT #4 (FINAL)**: Compile ALL blueprints:
     - `compile_blueprint(BP_DialogueNPC)`
     - `compile_blueprint(WBP_InteractionIndicator)`
     - `compile_blueprint(WBP_AnswerButton)`
     - `compile_blueprint(WBP_DialogueWindow)`
     - `compile_blueprint(PlayerCharacter)`
   - **ALL must pass with ZERO errors before testing**

6. **Test & Debug** (Phase 5)
   - Place BP_DialogueNPC in test level
   - Verify all functionality via checklist
   - Document any MCP tool limitations discovered

**Why Compilation Checkpoints Matter**:
- Catches type mismatches between connected pins immediately
- Identifies missing required connections before they cascade
- Validates that AI-created node logic is syntactically correct
- Proves MCP tools are creating valid, compilable Blueprint graphs
- Makes debugging easier by isolating issues to specific phases

**Total Estimated Time**: ~3-4 hours (accounting for Blueprint node creation via MCP tools)

---

## MCP Tools Required

The implementing AI will use these MCP tools exclusively:

### Project & Structure Tools
1. **create_folder** - Create content folders
2. **create_struct** - Create DialogueAnswer and DialogueRow structs
3. **list_folder_contents** - Navigate project structure

### DataTable Tools
4. **create_datatable** - Create DT_TestNPCDialogue
5. **add_datatable_row** - Populate dialogue data (if available)

### Blueprint Tools
6. **create_blueprint** - Create BP_DialogueNPC
7. **add_blueprint_variable** - Add variables to blueprints
8. **add_component_to_blueprint** - Add mesh, sphere, widget components
9. **set_component_property** - Configure component properties
10. **set_static_mesh_properties** - Set mesh asset
11. **create_custom_blueprint_function** - Create StartDialogue, EndDialogue, etc.
12. **compile_blueprint** - Compile blueprints after changes

### UMG Widget Tools
13. **create_widget_blueprint** - Create WBP_* widgets
14. **add_widget_to_blueprint** - Build widget hierarchies
15. **set_widget_property** - Configure widget properties

### Blueprint Node Tools
16. **create_node** - Create Blueprint nodes (events, functions, variables, etc.)
17. **connect_nodes** - Wire nodes together
18. **set_node_property** - Configure node parameters

### Input Tools
19. **list_input_actions** - Check existing input actions
20. **list_input_mapping_contexts** - Check existing input contexts
21. **create_enhanced_input_action** - Create IA_Interact
22. **create_input_mapping_context** - Create IMC_Default
23. **add_mapping_to_context** - Map E key to IA_Interact

---

## Notes for Implementing AI

- ⚠️ **MCP TOOLS ONLY**: Every single asset, blueprint, widget, component, struct, and node MUST be created using MCP tools
- ⚠️ **NO MANUAL WORKAROUNDS**: If an MCP tool doesn't exist or fails, document it and stop - don't open Unreal Editor manually
- ⚠️ **NO C++ FILES**: Do NOT create DialogueComponent.h/.cpp or DialogueStructs.h manually - use `create_struct` and Blueprint-based logic
- ⚠️ **TEST THE PLUGIN**: This is about validating MCP tool coverage, not just building a dialogue system
- ⚠️ **COMPILATION CHECKPOINTS ARE MANDATORY**: After each phase where nodes are created and connected, you MUST compile the blueprint and verify it succeeds before proceeding. This is crucial for:
  - Catching type mismatches early
  - Identifying missing connections before they cascade into larger problems
  - Validating that MCP-created nodes form valid Blueprint logic
  - Proving the MCP plugin creates production-quality, compilable code
- **Keep it simple**: No fancy UI, no animations, minimal Blueprint node graphs
- **Test incrementally**: Compile after each blueprint using `compile_blueprint` MCP tool
- **Use placeholder assets**: `/Engine/BasicShapes/Cube.Cube` for NPC mesh, simple text for UI
- **Focus on functionality**: Get it working first, polish never (this is a test)
- **Blueprint-only logic**: All dialogue logic lives in BP_DialogueNPC, no C++ component needed
- **Document MCP gaps**: If any functionality can't be completed via MCP, report it as a plugin limitation
- **Node creation**: Use `node_tools` for all Blueprint graph logic - Event BeginPlay, Event Tick, custom functions, etc.
- **Connect nodes before compiling**: Always connect ALL nodes in a graph before running compilation checkpoint - unconnected required pins will cause compilation errors

## MCP Tool Limitations to Watch For

During implementation, document if any of these are missing or broken:

1. **Struct array support**: Can `create_struct` handle `DialogueAnswer[]` type?
2. **DataTable row editing**: Can we populate DataTable rows via MCP or only manually in editor?
3. **Widget hierarchy creation**: Are all UMG widget types supported in `add_widget_to_blueprint`?
4. **Blueprint node creation**: Can `node_tools` create all needed node types (events, branches, loops, casts)?
5. **Component property setting**: Can `set_component_property` handle all needed properties (SphereRadius, WidgetClass, etc.)?
6. **Widget event binding**: Can we bind button OnClicked events via MCP tools?

## Questions to Ask User If Issues Arise

1. "Which player character blueprint should I modify? (e.g., BP_ThirdPersonCharacter path)"
2. "If `node_tools` cannot create certain Blueprint nodes, should I document the gap and continue with what's possible?"
3. "If DataTable row population isn't available via MCP, is manual editor input acceptable for this test?"
4. "Should the interaction widget be billboard (always face camera) or world-space?"

---

## Success Criteria

✅ **Minimum Viable Product**:
- NPC with DialogueComponent in level
- Player walks near NPC → sees "Press E to Talk"
- Player presses E → dialogue window opens with NPC name, text, and 2+ answers
- Player clicks answer → dialogue closes
- Player can repeat interaction

✅ **Code Quality**:
- Compiles without errors
- No crashes during interaction
- Clean component-based architecture
- Easy to extend with new features

✅ **Testing Plugin**:
- Demonstrates MCP tools can create full gameplay systems
- Shows DataTable, Blueprint, Component, and Widget creation
- Validates Enhanced Input interaction
- Proves UMG widget functionality

✅ **MCP Tool Validation**:
- **100% of implementation done through MCP tools only**
- No manual file creation or Unreal Editor interaction
- Every asset created via Python MCP server commands
- Complete end-to-end AI-driven development workflow
- Validates that the Unreal MCP plugin enables true AI autonomy

---

## 🎯 Primary Goal Reminder

**This dialogue system exists for ONE purpose: To test if an AI assistant can build a complete Unreal Engine gameplay feature using ONLY the MCP tools provided by this plugin.**

**Success = Fully working dialogue system created 100% through MCP tool calls**
**Failure = Any manual intervention required**

This project IS the MCP plugin. This dialogue system is the validation test. The real product being tested is the MCP plugin's ability to enable AI-driven Unreal Engine development.

---

**End of Implementation Plan**

This plan is intentionally over-detailed to ensure another AI can implement it without ambiguity. The focus is on simplicity and extensibility, perfect for testing the Unreal MCP plugin's capabilities through 100% MCP-tool-driven development.