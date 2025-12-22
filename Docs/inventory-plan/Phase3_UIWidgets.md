# Phase 3: UI Widget Layer

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Create the visual representation of the inventory system using three widget blueprints:
1. **WBP_InventorySlot** - Individual slot showing item icon and stack count
2. **WBP_ItemContextMenu** - Right-click popup with Use/Drop buttons
3. **WBP_InventoryGrid** - Main container with 4x4 grid of slots

## Dependencies

- Phase 2 completed (BP_InventoryManager)
- Required assets:
  - BP_InventoryManager component
  - S_InventorySlot struct
  - S_ItemDefinition struct

---

## Section 3.1: WBP_InventorySlot

### Step 3.1.1: Create Slot Widget Blueprint

#### AIM
Create the widget that represents a single inventory slot. Each slot displays an item icon and stack count.

#### USAGE EXAMPLES
```python
# Created 16 times by WBP_InventoryGrid
slot = CreateWidget(WBP_InventorySlot)
slot.InitializeSlot(5, InventoryManager)
```

#### WORKFLOW
1. Create UserWidget blueprint
2. Widget will be instantiated by grid

#### MCP TOOL CALL

```python
create_umg_widget_blueprint(
    widget_name="WBP_InventorySlot",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

#### VERIFICATION
- [ ] WBP_InventorySlot created at /Game/Inventory/UI/WBP_InventorySlot

---

### Step 3.1.2: Add Slot Widget Components

#### AIM
Create the visual hierarchy: Border (background) > Image (item icon) + TextBlock (stack count) + Border (selection highlight)

#### USAGE EXAMPLES
```python
# Visual layout:
# ┌──────────────────┐
# │ [SlotBackground] │ 64x64 Border
# │ ┌──────────────┐ │
# │ │ [ItemIcon]   │ │ Image inside border
# │ │              │ │
# │ │           99 │ │ StackCountText at bottom-right
# │ └──────────────┘ │
# └──────────────────┘
```

#### WORKFLOW
1. Create Border as slot background (clickable area)
2. Add Image inside for item icon
3. Add TextBlock for stack count
4. Add selection highlight border (hidden by default)

#### MCP TOOL CALLS

```python
# Create Border with Image inside
create_parent_and_child_widget_components(
    widget_name="WBP_InventorySlot",
    parent_component_name="SlotBackground",
    child_component_name="ItemIcon",
    parent_component_type="Border",
    child_component_type="Image",
    parent_position=[0.0, 0.0],
    parent_size=[64.0, 64.0],
    child_attributes={
        "size_x": 56.0,
        "size_y": 56.0,
        "brush_color": [1.0, 1.0, 1.0, 1.0]
    }
)

# Set border background color
set_widget_component_property(
    widget_name="WBP_InventorySlot",
    component_name="SlotBackground",
    background_color=[0.2, 0.2, 0.2, 0.8],
    padding=[4.0, 4.0, 4.0, 4.0]
)

# Add stack count text (bottom-right)
add_widget_component_to_widget(
    widget_name="WBP_InventorySlot",
    component_name="StackCountText",
    component_type="TextBlock",
    position=[44.0, 44.0],
    size=[20.0, 20.0],
    text="1",
    font_size=12,
    justification="Right"
)

# Add selection highlight (hidden by default)
add_widget_component_to_widget(
    widget_name="WBP_InventorySlot",
    component_name="SelectionHighlight",
    component_type="Border",
    position=[0.0, 0.0],
    size=[64.0, 64.0],
    background_color=[1.0, 0.8, 0.0, 0.0]  # Gold, invisible (alpha=0)
)
```

#### COMPONENT LAYOUT

| Component | Type | Size | Purpose |
|-----------|------|------|---------|
| SlotBackground | Border | 64x64 | Clickable area, dark background |
| ItemIcon | Image | 56x56 | Displays item texture |
| StackCountText | TextBlock | 20x20 | Shows stack count (bottom-right) |
| SelectionHighlight | Border | 64x64 | Gold highlight when selected |

#### VERIFICATION
- [ ] Border background visible
- [ ] Image centered in border
- [ ] Text in bottom-right corner
- [ ] Selection highlight invisible by default

---

### Step 3.1.3: Add Slot Widget Variables

#### AIM
Store slot state and references needed for functionality.

#### MCP TOOL CALLS

```python
# Slot index in grid (0-15)
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="SlotIndex",
    variable_type="Integer",
    is_exposed=True
)

# Reference to inventory manager
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="InventoryManager",
    variable_type="/Game/Inventory/Blueprints/BP_InventoryManager",
    is_exposed=True
)

# Is this slot currently selected
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="IsSelected",
    variable_type="Boolean",
    is_exposed=False
)

# Cached slot data for quick access
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="CachedSlotData",
    variable_type="/Game/Inventory/Data/S_InventorySlot",
    is_exposed=False
)

# Reference to parent grid (for context menu)
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="ParentGrid",
    variable_type="UserWidget",
    is_exposed=True
)
```

#### VERIFICATION
- [ ] 5 variables created
- [ ] SlotIndex and InventoryManager are exposed

---

### Step 3.1.4: Create Slot Widget Functions

#### Function: InitializeSlot

##### AIM
Set up the slot with its index and manager reference. Called by grid during initialization.

##### USAGE EXAMPLES
```python
slot.InitializeSlot(5, InventoryManager, GridWidget)
```

##### WORKFLOW
1. Store SlotIndex parameter
2. Store InventoryManager reference
3. Store ParentGrid reference
4. Call RefreshSlot to update display

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="InitializeSlot",
    inputs=[
        {"name": "Index", "type": "Integer"},
        {"name": "Manager", "type": "/Game/Inventory/Blueprints/BP_InventoryManager"},
        {"name": "Grid", "type": "UserWidget"}
    ],
    outputs=[],
    is_pure=False,
    category="Slot"
)
```

---

#### Function: RefreshSlot

##### AIM
Update the visual display from inventory data. Called on initialization and when inventory changes.

##### USAGE EXAMPLES
```python
# After inventory change
slot.RefreshSlot()
```

##### WORKFLOW
1. Call InventoryManager.GetSlotData(SlotIndex)
2. Store result in CachedSlotData
3. If IsEmpty:
   - Set ItemIcon visibility to Hidden/Collapsed
   - Set StackCountText visibility to Hidden
4. Else:
   - Set ItemIcon texture to ItemDef.Icon
   - Set ItemIcon visibility to Visible
   - If StackCount > 1: Show StackCountText with count
   - Else: Hide StackCountText

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="RefreshSlot",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Slot"
)
```

---

#### Function: HandleMouseDown

##### AIM
Handle mouse button presses on the slot. Right-click shows context menu.

##### USAGE EXAMPLES
```python
# Bound to SlotBackground OnMouseButtonDown
# Right-click triggers context menu
```

##### WORKFLOW
1. Check which mouse button was pressed
2. If right mouse button:
   - Get mouse screen position
   - Call ParentGrid.ShowContextMenuAtSlot(SlotIndex, Position)
3. Return handled

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="HandleMouseDown",
    inputs=[
        {"name": "MyGeometry", "type": "Geometry"},
        {"name": "MouseEvent", "type": "PointerEvent"}
    ],
    outputs=[
        {"name": "ReturnValue", "type": "EventReply"}
    ],
    is_pure=False,
    category="Slot"
)
```

---

### Step 3.1.5: Bind Mouse Events

#### AIM
Connect the SlotBackground's mouse event to HandleMouseDown function.

#### MCP TOOL CALL

```python
bind_widget_component_event(
    widget_name="WBP_InventorySlot",
    widget_component_name="SlotBackground",
    event_name="OnMouseButtonDown",
    function_name="HandleMouseDown"
)
```

#### VERIFICATION
- [ ] OnMouseButtonDown bound to HandleMouseDown
- [ ] Right-click will trigger context menu

---

### Step 3.1.6: Compile Slot Widget

```python
compile_blueprint(blueprint_name="WBP_InventorySlot")
```

---

## Section 3.2: WBP_ItemContextMenu

### Step 3.2.1: Create Context Menu Widget

#### AIM
Create the right-click popup menu with Use and Drop buttons.

#### MCP TOOL CALL

```python
create_umg_widget_blueprint(
    widget_name="WBP_ItemContextMenu",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

---

### Step 3.2.2: Add Context Menu Components

#### AIM
Create the menu layout with background and two buttons.

#### USAGE EXAMPLES
```python
# Visual layout:
# ┌────────────────┐
# │ [MenuBG]       │ Border with dark background
# │ ┌────────────┐ │
# │ │ [Use]      │ │ Button (only if item IsUsable)
# │ ├────────────┤ │
# │ │ [Drop]     │ │ Button (only if item IsDroppable)
# │ └────────────┘ │
# └────────────────┘
```

#### MCP TOOL CALLS

```python
# Create Border with VerticalBox inside
create_parent_and_child_widget_components(
    widget_name="WBP_ItemContextMenu",
    parent_component_name="MenuBackground",
    child_component_name="ButtonContainer",
    parent_component_type="Border",
    child_component_type="VerticalBox",
    parent_position=[0.0, 0.0],
    parent_size=[100.0, 80.0],
    child_attributes={}
)

# Style the background
set_widget_component_property(
    widget_name="WBP_ItemContextMenu",
    component_name="MenuBackground",
    background_color=[0.15, 0.15, 0.15, 0.95],
    padding=[5.0, 5.0, 5.0, 5.0]
)

# Add Use button
add_widget_component_to_widget(
    widget_name="WBP_ItemContextMenu",
    component_name="UseButton",
    component_type="Button",
    position=[0.0, 0.0],
    size=[90.0, 30.0]
)

# Add text to Use button
add_child_widget_component_to_parent(
    widget_name="WBP_ItemContextMenu",
    parent_component_name="UseButton",
    child_component_name="UseButtonText",
    child_component_type="TextBlock",
    child_attributes={
        "text": "Use",
        "justification": "Center"
    }
)

# Add Drop button
add_widget_component_to_widget(
    widget_name="WBP_ItemContextMenu",
    component_name="DropButton",
    component_type="Button",
    position=[0.0, 35.0],
    size=[90.0, 30.0]
)

# Add text to Drop button
add_child_widget_component_to_parent(
    widget_name="WBP_ItemContextMenu",
    parent_component_name="DropButton",
    child_component_name="DropButtonText",
    child_component_type="TextBlock",
    child_attributes={
        "text": "Drop",
        "justification": "Center"
    }
)
```

#### VERIFICATION
- [ ] Menu has dark background
- [ ] Use button visible
- [ ] Drop button visible

---

### Step 3.2.3: Add Context Menu Variables

```python
# Slot this menu is acting on
add_blueprint_variable(
    blueprint_name="WBP_ItemContextMenu",
    variable_name="TargetSlotIndex",
    variable_type="Integer",
    is_exposed=True
)

# Reference to inventory manager
add_blueprint_variable(
    blueprint_name="WBP_ItemContextMenu",
    variable_name="InventoryManager",
    variable_type="/Game/Inventory/Blueprints/BP_InventoryManager",
    is_exposed=True
)

# Reference to parent grid (to close menu)
add_blueprint_variable(
    blueprint_name="WBP_ItemContextMenu",
    variable_name="ParentGrid",
    variable_type="UserWidget",
    is_exposed=True
)
```

---

### Step 3.2.4: Create Context Menu Functions

#### Function: InitializeMenu

##### AIM
Configure the menu for a specific slot, showing/hiding buttons based on item properties.

##### USAGE EXAMPLES
```python
menu.InitializeMenu(5, InventoryManager, GridWidget)
# Shows Use button only if item.IsUsable
# Shows Drop button only if item.IsDroppable
```

##### WORKFLOW
1. Store references
2. Get slot data from InventoryManager
3. If slot empty, hide both buttons
4. Else:
   - Set UseButton visibility based on ItemDef.IsUsable
   - Set DropButton visibility based on ItemDef.IsDroppable

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_ItemContextMenu",
    function_name="InitializeMenu",
    inputs=[
        {"name": "SlotIndex", "type": "Integer"},
        {"name": "Manager", "type": "/Game/Inventory/Blueprints/BP_InventoryManager"},
        {"name": "Grid", "type": "UserWidget"}
    ],
    outputs=[],
    is_pure=False,
    category="Menu"
)
```

---

#### Function: OnUseClicked

##### AIM
Handle Use button click. Calls inventory UseItem and closes menu.

##### WORKFLOW
1. Call InventoryManager.UseItem(TargetSlotIndex)
2. Call ParentGrid.HideContextMenu()

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_ItemContextMenu",
    function_name="OnUseClicked",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Menu"
)
```

---

#### Function: OnDropClicked

##### AIM
Handle Drop button click. Calls inventory DropItem and closes menu.

##### WORKFLOW
1. Call InventoryManager.DropItem(TargetSlotIndex, 1)
2. Call ParentGrid.HideContextMenu()

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_ItemContextMenu",
    function_name="OnDropClicked",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Menu"
)
```

---

### Step 3.2.5: Bind Button Events

```python
bind_widget_component_event(
    widget_name="WBP_ItemContextMenu",
    widget_component_name="UseButton",
    event_name="OnClicked",
    function_name="OnUseClicked"
)

bind_widget_component_event(
    widget_name="WBP_ItemContextMenu",
    widget_component_name="DropButton",
    event_name="OnClicked",
    function_name="OnDropClicked"
)
```

---

### Step 3.2.6: Compile Context Menu Widget

```python
compile_blueprint(blueprint_name="WBP_ItemContextMenu")
```

---

## Section 3.3: WBP_InventoryGrid

### Step 3.3.1: Create Grid Widget Blueprint

#### AIM
Create the main inventory container that holds the 4x4 grid of slots.

#### MCP TOOL CALL

```python
create_umg_widget_blueprint(
    widget_name="WBP_InventoryGrid",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

---

### Step 3.3.2: Add Grid Widget Components

#### AIM
Create the grid layout with title and slot container.

#### USAGE EXAMPLES
```python
# Visual layout:
# ┌─────────────────────────────┐
# │ [GridBackground]            │ Border 300x340
# │ ┌─────────────────────────┐ │
# │ │ Inventory               │ │ Title text
# │ ├─────────────────────────┤ │
# │ │ ┌──┬──┬──┬──┐          │ │ UniformGridPanel
# │ │ │  │  │  │  │          │ │ 4 columns
# │ │ ├──┼──┼──┼──┤          │ │
# │ │ │  │  │  │  │          │ │ Slots added dynamically
# │ │ ├──┼──┼──┼──┤          │ │
# │ │ │  │  │  │  │          │ │
# │ │ ├──┼──┼──┼──┤          │ │
# │ │ │  │  │  │  │          │ │
# │ │ └──┴──┴──┴──┘          │ │
# │ └─────────────────────────┘ │
# └─────────────────────────────┘
```

#### MCP TOOL CALLS

```python
# Background border
add_widget_component_to_widget(
    widget_name="WBP_InventoryGrid",
    component_name="GridBackground",
    component_type="Border",
    position=[0.0, 0.0],
    size=[300.0, 340.0],
    background_color=[0.1, 0.1, 0.1, 0.9]
)

# Title text
add_widget_component_to_widget(
    widget_name="WBP_InventoryGrid",
    component_name="TitleText",
    component_type="TextBlock",
    position=[10.0, 10.0],
    size=[280.0, 30.0],
    text="Inventory",
    font_size=18
)

# Grid panel for slots
add_widget_component_to_widget(
    widget_name="WBP_InventoryGrid",
    component_name="SlotsGrid",
    component_type="UniformGridPanel",
    position=[10.0, 50.0],
    size=[280.0, 280.0]
)

# Configure grid to have 4 columns
set_widget_component_property(
    widget_name="WBP_InventoryGrid",
    component_name="SlotsGrid",
    slot_padding=[2.0, 2.0, 2.0, 2.0],
    min_desired_slot_width=64.0,
    min_desired_slot_height=64.0
)
```

#### VERIFICATION
- [ ] Dark background visible
- [ ] Title "Inventory" at top
- [ ] Grid panel sized for 4x4 slots

---

### Step 3.3.3: Add Grid Widget Variables

```python
# Reference to inventory manager
add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="InventoryManager",
    variable_type="/Game/Inventory/Blueprints/BP_InventoryManager",
    is_exposed=True
)

# Class for slot widget instantiation
add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="SlotWidgetClass",
    variable_type="Class<UserWidget>",
    is_exposed=True
)

# Class for context menu instantiation
add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="ContextMenuClass",
    variable_type="Class<UserWidget>",
    is_exposed=True
)

# Array of created slot widgets
add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="SlotWidgets",
    variable_type="UserWidget[]",
    is_exposed=False
)

# Currently active context menu
add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="ActiveContextMenu",
    variable_type="UserWidget",
    is_exposed=False
)
```

---

### Step 3.3.4: Create Grid Widget Functions

#### Function: InitializeGrid

##### AIM
Create 16 slot widgets, add them to the grid, and set up event bindings.

##### USAGE EXAMPLES
```python
grid.InitializeGrid(InventoryManager)
# Creates 16 WBP_InventorySlot widgets
# Adds them to UniformGridPanel
# Binds to OnInventoryChanged
```

##### WORKFLOW
1. Store InventoryManager reference
2. Clear SlotWidgets array
3. For index 0 to 15:
   - Create WBP_InventorySlot widget
   - Call InitializeSlot(index, InventoryManager, self)
   - Add to SlotsGrid (UniformGridPanel)
   - Add to SlotWidgets array
4. Bind to InventoryManager.OnInventoryChanged -> RefreshAllSlots

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="InitializeGrid",
    inputs=[
        {"name": "Manager", "type": "/Game/Inventory/Blueprints/BP_InventoryManager"}
    ],
    outputs=[],
    is_pure=False,
    category="Grid"
)
```

---

#### Function: RefreshAllSlots

##### AIM
Update all slot widgets to reflect current inventory state.

##### USAGE EXAMPLES
```python
# Bound to OnInventoryChanged
# Called whenever inventory modified
```

##### WORKFLOW
1. For each widget in SlotWidgets:
   - Cast to WBP_InventorySlot
   - Call RefreshSlot()

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="RefreshAllSlots",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Grid"
)
```

---

#### Function: ShowContextMenuAtSlot

##### AIM
Display the context menu at a specific screen position for a slot.

##### USAGE EXAMPLES
```python
# Called from slot's HandleMouseDown
grid.ShowContextMenuAtSlot(5, [400, 300])
```

##### WORKFLOW
1. Call HideContextMenu() to close any existing menu
2. Get slot data to check if empty
3. If empty, return (no menu for empty slots)
4. Create WBP_ItemContextMenu widget
5. Call InitializeMenu(SlotIndex, InventoryManager, self)
6. Add to viewport at screen position
7. Store in ActiveContextMenu

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="ShowContextMenuAtSlot",
    inputs=[
        {"name": "SlotIndex", "type": "Integer"},
        {"name": "ScreenPosition", "type": "Vector2D"}
    ],
    outputs=[],
    is_pure=False,
    category="Grid"
)
```

---

#### Function: HideContextMenu

##### AIM
Remove the active context menu from viewport.

##### WORKFLOW
1. If ActiveContextMenu is valid:
   - Remove from parent
   - Set ActiveContextMenu to null

##### MCP TOOL CALL

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="HideContextMenu",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Grid"
)
```

---

### Step 3.3.5: Compile Grid Widget

```python
compile_blueprint(blueprint_name="WBP_InventoryGrid")
```

---

## Phase 3 Completion Checklist

### WBP_InventorySlot
- [ ] Widget created
- [ ] Components: SlotBackground, ItemIcon, StackCountText, SelectionHighlight
- [ ] Variables: SlotIndex, InventoryManager, IsSelected, CachedSlotData, ParentGrid
- [ ] Functions: InitializeSlot, RefreshSlot, HandleMouseDown
- [ ] OnMouseButtonDown event bound
- [ ] Compiles successfully

### WBP_ItemContextMenu
- [ ] Widget created
- [ ] Components: MenuBackground, ButtonContainer, UseButton, DropButton
- [ ] Variables: TargetSlotIndex, InventoryManager, ParentGrid
- [ ] Functions: InitializeMenu, OnUseClicked, OnDropClicked
- [ ] Button OnClicked events bound
- [ ] Compiles successfully

### WBP_InventoryGrid
- [ ] Widget created
- [ ] Components: GridBackground, TitleText, SlotsGrid
- [ ] Variables: InventoryManager, SlotWidgetClass, ContextMenuClass, SlotWidgets, ActiveContextMenu
- [ ] Functions: InitializeGrid, RefreshAllSlots, ShowContextMenuAtSlot, HideContextMenu
- [ ] Compiles successfully

### All Done Via MCP Tools
- [ ] `create_umg_widget_blueprint` used for all 3 widgets
- [ ] `add_widget_component_to_widget` used for components
- [ ] `create_parent_and_child_widget_components` used for nested layouts
- [ ] `set_widget_component_property` used for styling
- [ ] `add_blueprint_variable` used for all variables
- [ ] `create_custom_blueprint_function` used for all functions
- [ ] `bind_widget_component_event` used for event binding
- [ ] `compile_blueprint` successful for all widgets

### Ready for Phase 4
- [ ] All widgets complete and compiling
- [ ] Proceed to Phase 4: Integration Layer

---

## Next Phase

Continue to [Phase4_Integration.md](Phase4_Integration.md) to connect the inventory to the player character and input system.
