# Phase 1: Loot Window Skeleton

> **STATUS: 📋 PLANNED**

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Create visual loot widgets with **hardcoded test data** so user can verify layout before connecting real data.

**USER TEST at end of phase:** Add loot window to viewport via BeginPlay, verify:
- Window appears with correct styling
- Slots display hardcoded item icons
- Close button exists (click handler in Phase 4)

## Dependencies

- Existing S_InventorySlot struct (reuse for loot slots)
- Existing S_ItemDefinition struct (reuse for item data)

---

## Step 1.1: Create Base Folder

### MCP TOOL CALLS

```python
create_folder(folder_path="Content/Inventory/UI/Base")
create_folder(folder_path="Content/Loot")
create_folder(folder_path="Content/Loot/UI")
create_folder(folder_path="Content/Loot/Blueprints")
```

### VERIFICATION
- [ ] Folders created in Content Browser
- [ ] No errors

---

## Step 1.2: Create WBP_ItemSlotBase

### AIM
Base widget class for item slots. Both inventory and loot slots will extend this.

### COMPONENTS

| Component | Type | Size | Properties |
|-----------|------|------|------------|
| CanvasPanel | Canvas Panel | - | Root |
| SlotBackground | Border | 64x64 | Dark grey (0.1, 0.1, 0.1, 1.0) |
| ItemIcon | Image | 56x56 | Centered, displays item texture |
| StackCountText | TextBlock | - | Bottom-right, font size 12 |
| DragOverlay | Border | 64x64 | Grey (0.3, 0.3, 0.3, 0.6), **Hidden by default** |

### VARIABLES

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| SlotIndex | Integer | 0 | Position in container |
| CachedSlotData | S_InventorySlot | - | Cached slot data |
| ParentContainer | UserWidget | None | Reference to parent |
| IsDragSource | Boolean | false | True when being dragged |

### FUNCTIONS

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| SetSlotData | SlotData, ItemDef | - | Update display |
| SetDragOverlay | bVisible | - | Show/hide overlay |
| GetSlotData | - | SlotData | Return cached data |

### MCP TOOL CALLS

```python
# Create widget
create_umg_widget_blueprint(
    widget_name="WBP_ItemSlotBase",
    parent_class="UserWidget",
    path="/Game/Inventory/UI/Base"
)

# Add SlotBackground
add_widget_component_to_widget(
    widget_name="WBP_ItemSlotBase",
    component_name="SlotBackground",
    component_type="Border",
    position=[0, 0],
    size=[64, 64],
    kwargs={"background_color": [0.1, 0.1, 0.1, 1.0]}
)

# Add ItemIcon (inside SlotBackground, or on top)
add_widget_component_to_widget(
    widget_name="WBP_ItemSlotBase",
    component_name="ItemIcon",
    component_type="Image",
    position=[4, 4],
    size=[56, 56],
    kwargs={}
)

# Add StackCountText
add_widget_component_to_widget(
    widget_name="WBP_ItemSlotBase",
    component_name="StackCountText",
    component_type="TextBlock",
    position=[40, 46],
    size=[20, 16],
    kwargs={"text": "", "font_size": 12}
)

# Add DragOverlay (initially Hidden)
add_widget_component_to_widget(
    widget_name="WBP_ItemSlotBase",
    component_name="DragOverlay",
    component_type="Border",
    position=[0, 0],
    size=[64, 64],
    kwargs={"background_color": [0.3, 0.3, 0.3, 0.6]}
)

set_widget_component_property(
    widget_name="WBP_ItemSlotBase",
    component_name="DragOverlay",
    kwargs={"Visibility": "Hidden"}
)

# Add variables
add_blueprint_variable(
    blueprint_name="WBP_ItemSlotBase",
    variable_name="SlotIndex",
    variable_type="Integer",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_ItemSlotBase",
    variable_name="CachedSlotData",
    variable_type="/Game/Inventory/Data/S_InventorySlot",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_ItemSlotBase",
    variable_name="ParentContainer",
    variable_type="UserWidget",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_ItemSlotBase",
    variable_name="IsDragSource",
    variable_type="Boolean",
    is_exposed=False
)

# Create functions
create_custom_blueprint_function(
    blueprint_name="WBP_ItemSlotBase",
    function_name="SetSlotData",
    inputs=[
        {"name": "SlotData", "type": "S_InventorySlot"},
        {"name": "ItemDef", "type": "S_ItemDefinition"}
    ],
    outputs=[],
    is_pure=False,
    category="Slot"
)

create_custom_blueprint_function(
    blueprint_name="WBP_ItemSlotBase",
    function_name="SetDragOverlay",
    inputs=[{"name": "bVisible", "type": "Boolean"}],
    outputs=[],
    is_pure=False,
    category="Slot"
)

create_custom_blueprint_function(
    blueprint_name="WBP_ItemSlotBase",
    function_name="GetSlotData",
    inputs=[],
    outputs=[{"name": "SlotData", "type": "S_InventorySlot"}],
    is_pure=True,
    category="Slot"
)

compile_blueprint(blueprint_name="WBP_ItemSlotBase")
```

### FUNCTION IMPLEMENTATIONS

#### SetSlotData
```
1. Set CachedSlotData = SlotData
2. If SlotData.ItemRowName is not empty:
   - Set ItemIcon texture = ItemDef.Icon
   - If SlotData.StackCount > 1:
     - Set StackCountText = StackCount (as string)
     - Set StackCountText visibility = Visible
   - Else:
     - Set StackCountText visibility = Hidden
   - Set ItemIcon visibility = Visible
3. Else (empty slot):
   - Set ItemIcon visibility = Hidden
   - Set StackCountText visibility = Hidden
```

#### SetDragOverlay
```
1. If bVisible:
   - Set DragOverlay visibility = Visible
2. Else:
   - Set DragOverlay visibility = Hidden
3. Set IsDragSource = bVisible
```

#### GetSlotData
```
1. Return CachedSlotData
```

### VERIFICATION
- [ ] WBP_ItemSlotBase created at /Game/Inventory/UI/Base/
- [ ] All 5 components added
- [ ] DragOverlay initially Hidden
- [ ] 4 variables added
- [ ] 3 functions created
- [ ] Blueprint compiles

---

## Step 1.3: Create WBP_LootSlot

### AIM
Loot-specific slot widget extending base. For now, simple implementation.

### MCP TOOL CALLS

```python
create_umg_widget_blueprint(
    widget_name="WBP_LootSlot",
    parent_class="WBP_ItemSlotBase",  # Extend base
    path="/Game/Loot/UI"
)

# Add LootableRef variable
add_blueprint_variable(
    blueprint_name="WBP_LootSlot",
    variable_name="LootableRef",
    variable_type="Actor",  # Will be BP_LootableBase later
    is_exposed=False
)

compile_blueprint(blueprint_name="WBP_LootSlot")
```

### VERIFICATION
- [ ] WBP_LootSlot created at /Game/Loot/UI/
- [ ] Inherits from WBP_ItemSlotBase
- [ ] LootableRef variable added
- [ ] Blueprint compiles

---

## Step 1.4: Create WBP_LootWindow

### AIM
Container for loot slots with title and close button.

### COMPONENTS

| Component | Type | Size | Properties |
|-----------|------|------|------------|
| CanvasPanel | Canvas Panel | - | Root |
| WindowBackground | Border | 300x350 | Dark (0.05, 0.05, 0.05, 0.95) |
| HeaderBackground | Border | 280x35 | Slightly lighter (0.1, 0.1, 0.15, 1.0) |
| TitleText | TextBlock | - | "Loot", font size 16 |
| CloseButton | Button | 24x24 | Top-right corner |
| CloseButtonText | TextBlock | - | "X" |
| SlotsGrid | UniformGridPanel | 280x280 | 4 columns |

### VARIABLES

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| SlotWidgets | WBP_LootSlot[] | [] | Array of slot widgets |
| LootableRef | Actor | None | Reference to lootable |
| InventoryGridRef | UserWidget | None | Reference to inventory |
| GridColumns | Integer | 4 | Slots per row |
| MaxSlots | Integer | 8 | Maximum loot slots |

### MCP TOOL CALLS

```python
create_umg_widget_blueprint(
    widget_name="WBP_LootWindow",
    parent_class="UserWidget",
    path="/Game/Loot/UI"
)

# Add WindowBackground
add_widget_component_to_widget(
    widget_name="WBP_LootWindow",
    component_name="WindowBackground",
    component_type="Border",
    position=[0, 0],
    size=[300, 350],
    kwargs={"background_color": [0.05, 0.05, 0.05, 0.95]}
)

# Add HeaderBackground
add_widget_component_to_widget(
    widget_name="WBP_LootWindow",
    component_name="HeaderBackground",
    component_type="Border",
    position=[10, 10],
    size=[280, 35],
    kwargs={"background_color": [0.1, 0.1, 0.15, 1.0]}
)

# Add TitleText
add_widget_component_to_widget(
    widget_name="WBP_LootWindow",
    component_name="TitleText",
    component_type="TextBlock",
    position=[20, 15],
    size=[200, 25],
    kwargs={"text": "Loot", "font_size": 16}
)

# Add CloseButton
add_widget_component_to_widget(
    widget_name="WBP_LootWindow",
    component_name="CloseButton",
    component_type="Button",
    position=[262, 15],
    size=[24, 24],
    kwargs={"text": "X"}
)

# Add SlotsGrid
add_widget_component_to_widget(
    widget_name="WBP_LootWindow",
    component_name="SlotsGrid",
    component_type="UniformGridPanel",
    position=[10, 55],
    size=[280, 280],
    kwargs={}
)

# Add variables
add_blueprint_variable(
    blueprint_name="WBP_LootWindow",
    variable_name="SlotWidgets",
    variable_type="WBP_LootSlot[]",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_LootWindow",
    variable_name="LootableRef",
    variable_type="Actor",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_LootWindow",
    variable_name="InventoryGridRef",
    variable_type="UserWidget",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_LootWindow",
    variable_name="GridColumns",
    variable_type="Integer",
    is_exposed=False
)

set_blueprint_variable_value(
    blueprint_name="WBP_LootWindow",
    variable_name="GridColumns",
    value=4
)

add_blueprint_variable(
    blueprint_name="WBP_LootWindow",
    variable_name="MaxSlots",
    variable_type="Integer",
    is_exposed=False
)

set_blueprint_variable_value(
    blueprint_name="WBP_LootWindow",
    variable_name="MaxSlots",
    value=8
)

# Create functions (implementations in Phase 4)
create_custom_blueprint_function(
    blueprint_name="WBP_LootWindow",
    function_name="InitializeSlots",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Loot"
)

create_custom_blueprint_function(
    blueprint_name="WBP_LootWindow",
    function_name="RefreshAllSlots",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Loot"
)

compile_blueprint(blueprint_name="WBP_LootWindow")
```

### VERIFICATION
- [ ] WBP_LootWindow created at /Game/Loot/UI/
- [ ] All components visible
- [ ] Variables added with defaults
- [ ] Blueprint compiles

---

## Step 1.5: Test Widget Display

### AIM
Temporarily add loot window to viewport to verify visual appearance.

### APPROACH
Add to player's BeginPlay temporarily (or create test Blueprint).

```python
# Get existing BP_ThirdPersonCharacter
# In BeginPlay, add nodes to:
# 1. Create WBP_LootWindow widget
# 2. Add to viewport
# This is TEMPORARY for visual testing only
```

Alternatively, open widget in UMG Editor and use preview.

### USER TEST
1. Launch Unreal Editor
2. Open WBP_LootWindow in UMG Designer
3. Verify:
   - [ ] Window background appears (dark)
   - [ ] Header bar visible
   - [ ] "Loot" title text shows
   - [ ] Close button (X) in top-right
   - [ ] Grid area visible

---

## Phase 1 Completion Checklist

### Assets Created
- [ ] WBP_ItemSlotBase with components and functions
- [ ] WBP_LootSlot extending base
- [ ] WBP_LootWindow with all components

### Visual Verification (USER TEST)
- [ ] Loot window displays correctly in designer
- [ ] Slot base displays with dark background
- [ ] DragOverlay is hidden by default
- [ ] All widgets compile without errors

### Ready for Phase 2
- [ ] All widgets compile
- [ ] User has verified visual appearance
- [ ] Proceed to Phase 2: Drag Widget

---

## Next Phase

Continue to [Phase2_DragWidget.md](Phase2_DragWidget.md) to create the cursor-following drag visual.
