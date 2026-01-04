# Phase 5: E-Key Integration

> **STATUS: 📋 PLANNED**

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Complete the system integration:
1. Modify player E-key handler to use BPI_Interactable
2. Add interface implementation to BP_DialogueNPC
3. Add OpenLootWindow/CloseLootWindow functions to player
4. Implement side-by-side layout

**USER TEST at end of phase:** E near NPC = dialogue, E near chest = loot window.

## Dependencies

- Phase 4 completed (loot system working)
- Existing BP_DialogueNPC, IA_Interact

---

## Step 5.1: Add Player Variables

### AIM
Add variables to track loot window state.

### MCP TOOL CALLS

```python
add_blueprint_variable(
    blueprint_name="BP_ThirdPersonCharacter",
    variable_name="ActiveLootWindow",
    variable_type="UserWidget",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="BP_ThirdPersonCharacter",
    variable_name="IsLootOpen",
    variable_type="Boolean",
    is_exposed=False
)

compile_blueprint(blueprint_name="BP_ThirdPersonCharacter")
```

### VERIFICATION
- [ ] Variables added
- [ ] Blueprint compiles

---

## Step 5.2: Create OpenLootWindow Function

### AIM
Function to open loot UI with inventory side-by-side.

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="OpenLootWindow",
    inputs=[{"name": "Lootable", "type": "Actor"}],  # Will be cast to BP_LootableBase
    outputs=[],
    is_pure=False,
    category="Loot"
)
```

### LOGIC
```
Input: Lootable (Actor)

1. If IsLootOpen:
   - Return (already open)

2. Cast Lootable to BP_LootableBase
3. If cast fails:
   - Return

4. Create WBP_LootWindow widget
5. Set ActiveLootWindow = created widget

6. If NOT IsInventoryOpen:
   - Create WBP_InventoryGrid widget
   - Call InitializeGrid(InventoryManagerRef)
   - Set InventoryWidget = created widget
   - Set IsInventoryOpen = true

7. Position windows side-by-side:
   - Loot window: X = 200, Y = 200 (left side)
   - Inventory: X = 560, Y = 200 (right side)

8. Call OpenLoot(Lootable, InventoryWidget) on ActiveLootWindow
9. Add ActiveLootWindow to viewport (Z-Order: 50)
10. Add InventoryWidget to viewport (Z-Order: 50) if newly created

11. Set IsLootOpen = true
12. Set Input Mode Game and UI
13. Set Show Mouse Cursor = true
```

### VERIFICATION
- [ ] OpenLootWindow function created
- [ ] Logic implemented
- [ ] Blueprint compiles

---

## Step 5.3: Create CloseLootWindow Function

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="BP_ThirdPersonCharacter",
    function_name="CloseLootWindow",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Loot"
)
```

### LOGIC
```
1. If NOT IsLootOpen:
   - Return

2. If ActiveLootWindow is valid:
   - Remove ActiveLootWindow from parent
   - Set ActiveLootWindow = None

3. Set IsLootOpen = false

4. If IsInventoryOpen:
   - Reposition inventory to center: X = 400, Y = 200
   - (Keep inventory open after closing loot)
5. Else:
   - Set Input Mode Game Only
   - Set Show Mouse Cursor = false
```

### VERIFICATION
- [ ] CloseLootWindow function created
- [ ] Logic implemented
- [ ] Blueprint compiles

---

## Step 5.4: Add BPI_Interactable to BP_DialogueNPC

### AIM
Update existing NPC to use interface.

### MCP TOOL CALLS

```python
add_interface_to_blueprint(
    blueprint_name="BP_DialogueNPC",
    interface_name="/Game/Blueprints/Interfaces/BPI_Interactable"
)

# Create CanInteract implementation
create_custom_blueprint_function(
    blueprint_name="BP_DialogueNPC",
    function_name="CanInteract",
    inputs=[{"name": "Interactor", "type": "Actor"}],
    outputs=[{"name": "CanInteract", "type": "Boolean"}],
    is_pure=True,
    category="Interaction"
)

# Create GetInteractionPrompt implementation
create_custom_blueprint_function(
    blueprint_name="BP_DialogueNPC",
    function_name="GetInteractionPrompt",
    inputs=[],
    outputs=[{"name": "Prompt", "type": "String"}],
    is_pure=True,
    category="Interaction"
)

# Create OnInteract implementation
create_custom_blueprint_function(
    blueprint_name="BP_DialogueNPC",
    function_name="OnInteract",
    inputs=[{"name": "Interactor", "type": "Actor"}],
    outputs=[],
    is_pure=False,
    category="Interaction"
)

compile_blueprint(blueprint_name="BP_DialogueNPC")
```

### FUNCTION IMPLEMENTATIONS

#### CanInteract
```
Input: Interactor (Actor)
Output: CanInteract (bool)

1. Return NOT bDialogueActive
   (Can interact if not already in dialogue)
```

#### GetInteractionPrompt
```
Output: Prompt (String)

1. Return "Press E to Talk"
```

#### OnInteract
```
Input: Interactor (Actor)

1. Call StartDialogue()
   (Existing function that opens dialogue window)
```

### VERIFICATION
- [ ] Interface added to BP_DialogueNPC
- [ ] CanInteract returns based on dialogue state
- [ ] GetInteractionPrompt returns text
- [ ] OnInteract calls StartDialogue
- [ ] Blueprint compiles

---

## Step 5.5: Modify E-Key Handler

### AIM
Update IA_Interact handler to use interface.

### CURRENT IMPLEMENTATION
```
IA_Interact (Triggered)
    ↓
Sphere Overlap Actors (radius ~350cm)
    ↓
For Each: Cast to BP_DialogueNPC
    ↓
If cast succeeds: Call StartDialogue()
```

### NEW IMPLEMENTATION
```
IA_Interact (Triggered)
    ↓
Get Actor Location (Self)
    ↓
Sphere Overlap Actors (radius 350cm)
    ↓
For Each Actor:
    ↓
Does Implement Interface (BPI_Interactable)?
    ↓
YES: Call CanInteract(Self) on actor
    ↓
    Returns TRUE: Call OnInteract(Self) on actor
    ↓
    Break (only interact with first valid actor)
```

### MCP TOOL CALLS

Modify existing IA_Interact event in EventGraph:

```python
# This requires modifying existing event graph nodes
# Replace Cast to BP_DialogueNPC with Does Implement Interface
# Replace direct StartDialogue call with interface calls
```

### VERIFICATION
- [ ] E-key uses interface check
- [ ] Works with NPCs (dialogue)
- [ ] Works with lootables (loot window)
- [ ] Blueprint compiles

---

## Step 5.6: Wire Close Button

### AIM
Connect loot window close button.

### MCP TOOL CALLS

```python
bind_widget_component_event(
    widget_name="WBP_LootWindow",
    widget_component_name="CloseButton",
    event_name="OnClicked",
    function_name="OnCloseButtonClicked"
)
```

### LOGIC: OnCloseButtonClicked
```
1. Get owning player
2. Cast to BP_ThirdPersonCharacter
3. Call CloseLootWindow() on character
```

### VERIFICATION
- [ ] Close button bound
- [ ] Clicking closes window
- [ ] Blueprint compiles

---

## Step 5.7: Test Complete Flow

### TEST SCENARIO 1: Basic Loot Flow
```
1. Walk to test loot chest
2. Press E
   → Expected: Loot window opens on left, inventory on right
3. Left-click on HealthPotion in loot
   → Expected: Grey overlay appears, drag widget follows cursor
4. Release over inventory grid
   → Expected: Potion appears in inventory, disappears from loot
5. Click X on loot window
   → Expected: Loot closes, inventory recenters
```

### TEST SCENARIO 2: Dialogue Still Works
```
1. Walk to NPC
2. Press E
   → Expected: Dialogue window opens (not loot)
3. Complete dialogue
4. Walk to loot chest
5. Press E
   → Expected: Loot window opens
```

### TEST SCENARIO 3: Cancel Drag
```
1. Open loot
2. Start dragging item
3. Release over empty area (not on container)
   → Expected: Item returns to original slot, overlay clears
```

### TEST SCENARIO 4: Context Awareness
```
1. Stand between NPC and chest
2. Face NPC, press E → Dialogue
3. Face chest, press E → Loot
   (Based on which is closer/in front)
```

---

## Phase 5 Completion Checklist

### Player Functions
- [ ] OpenLootWindow positions both windows
- [ ] CloseLootWindow cleans up properly
- [ ] Variables track loot window state

### Interface Integration
- [ ] BP_DialogueNPC implements BPI_Interactable
- [ ] BP_LootableBase implements BPI_Interactable
- [ ] E-key uses interface to determine action

### Side-by-Side Layout
- [ ] Loot on left (X=200)
- [ ] Inventory on right (X=560)
- [ ] No overlapping
- [ ] Inventory recenters when loot closes

### Close Button
- [ ] Button wired to event
- [ ] Closes loot window properly

### Full Flow Works (USER TEST)
- [ ] E-key near loot → loot opens
- [ ] E-key near NPC → dialogue opens
- [ ] Drag from loot to inventory works
- [ ] Close button works
- [ ] Cancel drag works
- [ ] Mouse cursor shows/hides correctly
- [ ] Input mode switches correctly

---

## Final System Verification

### All Success Criteria Met
- [ ] E key opens dialogue for NPCs, loot window for lootables
- [ ] Inventory and loot windows appear side-by-side
- [ ] Left-click and drag from loot slot shows grey overlay
- [ ] Dragged item visual follows cursor
- [ ] Drop on inventory transfers full stack
- [ ] Source slot clears, target slot shows item
- [ ] Both containers refresh after transfer
- [ ] Close button on loot window works
- [ ] Empty loot containers work correctly

### All Done Via MCP Tools
- [ ] `create_blueprint_interface` used
- [ ] `add_interface_to_blueprint` used
- [ ] `add_blueprint_variable` used
- [ ] `create_custom_blueprint_function` used
- [ ] `create_node_by_action_name` used
- [ ] `connect_blueprint_nodes` used
- [ ] `spawn_actor` used (with Blueprint path)
- [ ] All blueprints compile

---

## Return to Overview

See [00_Overview.md](00_Overview.md) for complete system documentation.
