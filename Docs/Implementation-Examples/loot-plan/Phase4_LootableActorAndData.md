# Phase 4: Lootable Actor and Data Connection

> **STATUS: 📋 PLANNED**

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Create lootable actor with real item data and connect to loot window:
1. Create BPI_Interactable interface
2. Create BP_LootableBase actor with loot slot array
3. Connect loot window to real data
4. Implement item transfer (loot → inventory)

**USER TEST at end of phase:** Place chest in level, open loot window, drag item to inventory.

## Dependencies

- Phase 3 completed (inventory drag working)
- Existing S_InventorySlot, S_ItemDefinition, DT_Items

---

## Step 4.1: Create BPI_Interactable Interface

### AIM
Create interface for shared E-key interaction (used by NPCs and lootables).

### INTERFACE FUNCTIONS

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| CanInteract | Interactor (Actor) | CanInteract (bool) | Check if interaction possible |
| GetInteractionPrompt | - | Prompt (String) | Get display text |
| OnInteract | Interactor (Actor) | - | Execute interaction |

### MCP TOOL CALLS

```python
# Create folder for interfaces
create_folder(folder_path="Content/Blueprints/Interfaces")

# Create the interface
create_blueprint_interface(
    name="BPI_Interactable",
    folder_path="/Game/Blueprints/Interfaces"
)

# Note: Interface functions are typically added via UE Editor
# MCP may need add_interface_function tool or similar
```

### VERIFICATION
- [ ] BPI_Interactable created
- [ ] CanInteract function exists (Actor input, bool output)
- [ ] GetInteractionPrompt function exists (String output)
- [ ] OnInteract function exists (Actor input)

---

## Step 4.2: Create BP_LootableBase Actor

### AIM
Base actor for lootable objects (chests, crates, etc.).

### COMPONENTS

| Component | Type | Properties |
|-----------|------|------------|
| DefaultSceneRoot | Scene | Root |
| StaticMesh | StaticMesh | Visual mesh |
| InteractionSphere | SphereCollision | Radius 200cm |
| InteractionPrompt | WidgetComponent | "Press E to Loot" |

### VARIABLES

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| LootSlots | S_InventorySlot[] | [] | Items in container |
| ItemsDataTable | DataTable | DT_Items | Reference to item defs |
| MaxSlots | Integer | 8 | Maximum capacity |
| bIsLooted | Boolean | false | Already emptied |

### FUNCTIONS

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| InitializeLoot | ItemNames[], Quantities[] | Success | Set initial items |
| GetLootSlotData | SlotIndex | SlotData, ItemDef, IsEmpty | Get slot info |
| RemoveLootItem | SlotIndex | Removed, Success | Remove item |
| HasLoot | - | HasItems (bool) | Check if has items |

### MCP TOOL CALLS

```python
# Create actor blueprint
create_blueprint(
    name="BP_LootableBase",
    parent_class="Actor",
    folder_path="/Game/Loot/Blueprints"
)

# Add interface
add_interface_to_blueprint(
    blueprint_name="BP_LootableBase",
    interface_name="/Game/Blueprints/Interfaces/BPI_Interactable"
)

# Add components
add_component_to_blueprint(
    blueprint_name="BP_LootableBase",
    component_type="StaticMeshComponent",
    component_name="LootMesh"
)

add_component_to_blueprint(
    blueprint_name="BP_LootableBase",
    component_type="SphereComponent",
    component_name="InteractionSphere"
)

# Configure InteractionSphere radius (200cm)

add_component_to_blueprint(
    blueprint_name="BP_LootableBase",
    component_type="WidgetComponent",
    component_name="InteractionPrompt"
)

# Add variables
add_blueprint_variable(
    blueprint_name="BP_LootableBase",
    variable_name="LootSlots",
    variable_type="/Game/Inventory/Data/S_InventorySlot[]",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="BP_LootableBase",
    variable_name="ItemsDataTable",
    variable_type="DataTable",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="BP_LootableBase",
    variable_name="MaxSlots",
    variable_type="Integer",
    is_exposed=True
)

set_blueprint_variable_value(
    blueprint_name="BP_LootableBase",
    variable_name="MaxSlots",
    value=8
)

add_blueprint_variable(
    blueprint_name="BP_LootableBase",
    variable_name="bIsLooted",
    variable_type="Boolean",
    is_exposed=False
)

# Create functions
create_custom_blueprint_function(
    blueprint_name="BP_LootableBase",
    function_name="InitializeLoot",
    inputs=[
        {"name": "ItemNames", "type": "Name[]"},
        {"name": "Quantities", "type": "Integer[]"}
    ],
    outputs=[{"name": "Success", "type": "Boolean"}],
    is_pure=False,
    category="Loot"
)

create_custom_blueprint_function(
    blueprint_name="BP_LootableBase",
    function_name="GetLootSlotData",
    inputs=[{"name": "SlotIndex", "type": "Integer"}],
    outputs=[
        {"name": "SlotData", "type": "S_InventorySlot"},
        {"name": "ItemDef", "type": "S_ItemDefinition"},
        {"name": "IsEmpty", "type": "Boolean"}
    ],
    is_pure=True,
    category="Loot"
)

create_custom_blueprint_function(
    blueprint_name="BP_LootableBase",
    function_name="RemoveLootItem",
    inputs=[{"name": "SlotIndex", "type": "Integer"}],
    outputs=[
        {"name": "Removed", "type": "Boolean"},
        {"name": "Success", "type": "Boolean"}
    ],
    is_pure=False,
    category="Loot"
)

create_custom_blueprint_function(
    blueprint_name="BP_LootableBase",
    function_name="HasLoot",
    inputs=[],
    outputs=[{"name": "HasItems", "type": "Boolean"}],
    is_pure=True,
    category="Loot"
)

compile_blueprint(blueprint_name="BP_LootableBase")
```

### INTERFACE IMPLEMENTATIONS

#### CanInteract
```
Input: Interactor (Actor)
Output: CanInteract (bool)

1. Return HasLoot() AND NOT bIsLooted
```

#### GetInteractionPrompt
```
Output: Prompt (String)

1. Return "Press E to Loot"
```

#### OnInteract
```
Input: Interactor (Actor)

1. Cast Interactor to BP_ThirdPersonCharacter
2. Call OpenLootWindow(Self) on character
```

### VERIFICATION
- [ ] BP_LootableBase created
- [ ] Implements BPI_Interactable
- [ ] Components added
- [ ] Variables and functions created
- [ ] Interface functions implemented
- [ ] Blueprint compiles

---

## Step 4.3: Implement Loot Functions

### InitializeLoot
```
Input: ItemNames (Name[]), Quantities (int[])
Output: Success (bool)

1. Clear LootSlots array
2. For i = 0 to min(ItemNames.Length, MaxSlots):
   a. Create S_InventorySlot struct
   b. Set ItemRowName = ItemNames[i]
   c. Set StackCount = Quantities[i]
   d. Add to LootSlots
3. Return true
```

### GetLootSlotData
```
Input: SlotIndex (int)
Output: SlotData, ItemDef, IsEmpty

1. If SlotIndex < 0 OR SlotIndex >= LootSlots.Length:
   - Return empty, empty, true
2. Get slot from LootSlots[SlotIndex]
3. If slot.ItemRowName is empty:
   - Return slot, empty, true
4. Look up ItemDef from ItemsDataTable
5. Return slot, ItemDef, false
```

### RemoveLootItem
```
Input: SlotIndex (int)
Output: Removed, Success

1. If SlotIndex < 0 OR SlotIndex >= LootSlots.Length:
   - Return false, false
2. Get slot from LootSlots[SlotIndex]
3. If slot.ItemRowName is empty:
   - Return false, false
4. Clear slot (set ItemRowName = None, StackCount = 0)
5. Set LootSlots[SlotIndex] = cleared slot
6. Return true, true
```

### HasLoot
```
Output: HasItems (bool)

1. For each slot in LootSlots:
   - If slot.ItemRowName is NOT empty:
     - Return true
2. Return false
```

### VERIFICATION
- [ ] All functions implemented
- [ ] InitializeLoot populates array
- [ ] GetLootSlotData retrieves correct data
- [ ] RemoveLootItem clears slot
- [ ] HasLoot detects items
- [ ] Blueprint compiles

---

## Step 4.4: Connect WBP_LootWindow to Real Data

### AIM
Update loot window to display items from BP_LootableBase.

### NEW FUNCTION: OpenLoot

```python
create_custom_blueprint_function(
    blueprint_name="WBP_LootWindow",
    function_name="OpenLoot",
    inputs=[
        {"name": "Lootable", "type": "BP_LootableBase"},
        {"name": "InventoryGrid", "type": "UserWidget"}
    ],
    outputs=[],
    is_pure=False,
    category="Loot"
)
```

### LOGIC: OpenLoot
```
Input: Lootable, InventoryGrid

1. Set LootableRef = Lootable
2. Set InventoryGridRef = InventoryGrid
3. Call InitializeSlots()
4. Call RefreshAllSlots()
```

### UPDATE: InitializeSlots
```
1. Clear SlotWidgets array
2. For i = 0 to MaxSlots:
   a. Create WBP_LootSlot widget
   b. Set SlotIndex = i
   c. Set ParentContainer = Self
   d. Set LootableRef = LootableRef
   e. Add to SlotsGrid at row i/4, column i%4
   f. Add to SlotWidgets array
```

### UPDATE: RefreshAllSlots
```
1. For each slot in SlotWidgets:
   a. Get SlotIndex
   b. Call LootableRef.GetLootSlotData(SlotIndex)
   c. Call slot.SetSlotData(SlotData, ItemDef)
```

### VERIFICATION
- [ ] OpenLoot function created
- [ ] InitializeSlots creates slot widgets
- [ ] RefreshAllSlots displays items
- [ ] Blueprint compiles

---

## Step 4.5: Implement EndDrag for Item Transfer

### AIM
Complete drag by transferring item from loot to inventory.

### ADD TO WBP_InventoryGrid: EndDrag Function

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="EndDrag",
    inputs=[{"name": "TargetContainer", "type": "UserWidget"}],
    outputs=[],
    is_pure=False,
    category="DragDrop"
)
```

### LOGIC: EndDrag
```
Input: TargetContainer (UserWidget)

1. If DragSourceSlotIndex < 0:
   - Return (no active drag)

2. Get slot widget at DragSourceSlotIndex
3. Call SetDragOverlay(false) on slot widget

4. If TargetContainer is valid AND TargetContainer != DragSourceContainer:
   a. Get ActiveDragWidget's DraggedSlotData
   b. Call HandleItemDrop on TargetContainer with slot data
   c. If HandleItemDrop returns true:
      - Remove item from source container
      - Refresh source container

5. If ActiveDragWidget is valid:
   - Remove ActiveDragWidget from parent

6. Set ActiveDragWidget = None
7. Set DragSourceSlotIndex = -1
8. Set DragSourceContainer = None
```

### ADD: HandleItemDrop Function

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="HandleItemDrop",
    inputs=[{"name": "DroppedSlotData", "type": "S_InventorySlot"}],
    outputs=[{"name": "Success", "type": "Boolean"}],
    is_pure=False,
    category="DragDrop"
)
```

### LOGIC: HandleItemDrop
```
Input: DroppedSlotData (S_InventorySlot)
Output: Success (bool)

1. Get ItemRowName from DroppedSlotData
2. Get StackCount from DroppedSlotData
3. Call InventoryManager.AddItem(ItemRowName, StackCount)
4. If AmountAdded > 0:
   - Call RefreshAllSlots()
   - Return true
5. Return false
```

### VERIFICATION
- [ ] EndDrag function created
- [ ] HandleItemDrop function created
- [ ] Item transfers on drop
- [ ] Source slot clears
- [ ] Both containers refresh
- [ ] Blueprint compiles

---

## Step 4.6: Add StartDrag to WBP_LootWindow

### AIM
Enable dragging from loot slots.

```python
create_custom_blueprint_function(
    blueprint_name="WBP_LootWindow",
    function_name="StartDrag",
    inputs=[{"name": "SlotIndex", "type": "Integer"}],
    outputs=[],
    is_pure=False,
    category="DragDrop"
)
```

### LOGIC: (Same as WBP_InventoryGrid.StartDrag)

### ADD: Left-click handler to WBP_LootSlot

```python
create_widget_input_handler(
    widget_name="WBP_LootSlot",
    input_type="MouseButton",
    input_event="LeftMouseButton",
    trigger="Pressed",
    handler_name="OnSlotLeftClick"
)
```

### VERIFICATION
- [ ] StartDrag function on WBP_LootWindow
- [ ] Left-click on loot slot starts drag
- [ ] Blueprint compiles

---

## Step 4.7: Create Test Lootable

### AIM
Place a test chest in level with items.

### MCP TOOL CALLS

```python
# Place test lootable in level
spawn_actor(
    name="TestLootChest",
    type="/Game/Loot/Blueprints/BP_LootableBase",
    location=[500, 0, 100]
)
```

### INITIALIZE TEST DATA
In BP_LootableBase BeginPlay:
```
1. Call InitializeLoot with:
   - ItemNames: ["HealthPotion", "IronOre", "IronSword"]
   - Quantities: [5, 10, 1]
```

### VERIFICATION
- [ ] Test chest placed in level
- [ ] BeginPlay initializes items
- [ ] Blueprint compiles

---

## Step 4.8: Test Full Drag-Drop Flow

### USER TEST
1. Launch Unreal Editor
2. Play in Editor
3. Walk to test chest
4. Open loot window (manual test - E-key in Phase 5)
5. Verify:
   - [ ] Loot window shows items
   - [ ] Left-click loot slot shows grey overlay
   - [ ] Drag widget follows cursor
   - [ ] Drop on inventory transfers item
   - [ ] Item disappears from loot
   - [ ] Item appears in inventory
6. Test cancel:
   - [ ] Drag item but release elsewhere
   - [ ] Item stays in loot
   - [ ] Grey overlay disappears

---

## Phase 4 Completion Checklist

### Assets Created
- [ ] BPI_Interactable interface
- [ ] BP_LootableBase actor with loot storage

### Data Flow
- [ ] Loot window reads from actor
- [ ] Item transfer updates both containers
- [ ] DataTable lookup works

### Drag-Drop Complete
- [ ] Drag from loot shows overlay
- [ ] Drop on inventory transfers
- [ ] Cancel restores state
- [ ] Both directions work (if enabled)

### USER TEST Passed
- [ ] Full drag-drop flow working
- [ ] Items visually update
- [ ] No errors

### Ready for Phase 5
- [ ] All blueprints compile
- [ ] Data connection verified
- [ ] Proceed to Phase 5: E-Key Integration

---

## Next Phase

Continue to [Phase5_EKeyIntegration.md](Phase5_EKeyIntegration.md) to integrate with E-key and complete the system.
