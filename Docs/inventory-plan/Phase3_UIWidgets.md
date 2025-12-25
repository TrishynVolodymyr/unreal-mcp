# Phase 3: UI Widget Layer

> **STATUS: ✅ COMPLETE**
> All 4 widget blueprints successfully implemented via MCP tools.

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Create the visual representation of the inventory system using four widget blueprints:
1. **WBP_InventorySlot** - Individual slot showing item icon and stack count
2. **WBP_ItemContextMenu** - Right-click popup with Use/Drop buttons
3. **WBP_InventoryGrid** - Main container with 4x4 grid of slots
4. **WBP_ItemTooltip** - Hover tooltip showing item details

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

#### MCP TOOL CALL

```python
create_umg_widget_blueprint(
    widget_name="WBP_InventorySlot",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

#### VERIFICATION
- [x] WBP_InventorySlot created at /Game/Inventory/UI/WBP_InventorySlot

---

### Step 3.1.2: Add Slot Widget Components

#### ACTUAL IMPLEMENTATION

The slot uses the following component hierarchy:

| Component | Type | Purpose |
|-----------|------|---------|
| CanvasPanel | CanvasPanel | Root container (auto-created) |
| SlotBackground | Border | Clickable area, dark background |
| ItemIcon | Image | Displays item texture |
| StackCountText | TextBlock | Shows stack count (bottom-right) |
| SelectionHighlight | Border | Gold highlight when selected (hidden by default) |
| ContextMenuAnchor | SizeBox | Anchor point for context menu positioning |

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
    parent_size=[64.0, 64.0]
)

# Add stack count text (bottom-right)
add_widget_component_to_widget(
    widget_name="WBP_InventorySlot",
    component_name="StackCountText",
    component_type="TextBlock",
    position=[44.0, 44.0],
    size=[20.0, 20.0],
    text="1",
    font_size=12
)

# Add selection highlight (hidden by default)
add_widget_component_to_widget(
    widget_name="WBP_InventorySlot",
    component_name="SelectionHighlight",
    component_type="Border",
    position=[0.0, 0.0],
    size=[64.0, 64.0]
)

# Add context menu anchor for positioning
add_widget_component_to_widget(
    widget_name="WBP_InventorySlot",
    component_name="ContextMenuAnchor",
    component_type="SizeBox",
    position=[64.0, 64.0],
    size=[1.0, 1.0]
)
```

#### VERIFICATION
- [x] SlotBackground Border visible
- [x] ItemIcon Image centered in border
- [x] StackCountText in bottom-right corner
- [x] SelectionHighlight invisible by default
- [x] ContextMenuAnchor for menu positioning

---

### Step 3.1.3: Add Slot Widget Variables

#### ACTUAL IMPLEMENTATION (7 variables)

| Variable | Type | Purpose |
|----------|------|---------|
| SlotIndex | Integer | Position in grid (0-15) |
| InventoryManager | BP_InventoryManager_C | Reference to manager component |
| IsSelected | Boolean | Current selection state |
| CachedSlotData | S_InventorySlot | Cached slot data for quick access |
| ParentGrid | UserWidget | Reference to parent grid widget |
| TooltipClass | Class | Widget class for tooltip instantiation |
| ItemTooltipRef | UserWidget | Active tooltip instance |

#### MCP TOOL CALLS

```python
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="SlotIndex",
    variable_type="Integer",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="InventoryManager",
    variable_type="/Game/Inventory/Blueprints/BP_InventoryManager",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="IsSelected",
    variable_type="Boolean",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="CachedSlotData",
    variable_type="/Game/Inventory/Data/S_InventorySlot",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="ParentGrid",
    variable_type="UserWidget",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="TooltipClass",
    variable_type="Class",
    is_exposed=True
)

add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="ItemTooltipRef",
    variable_type="UserWidget",
    is_exposed=True
)
```

---

### Step 3.1.4: Create Slot Widget Functions

#### ACTUAL IMPLEMENTATION (7 functions)

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| InitializeSlot | Index (int32), Manager (BP_InventoryManager_C*), Grid (UserWidget*) | - | Setup slot with references |
| RefreshSlot | - | - | Update display from data |
| HandleMouseDown | MyGeometry (FGeometry), MouseEvent (FPointerEvent) | ReturnValue (FEventReply) | Process mouse clicks |
| OnMouseButtonDown | MyGeometry (FGeometry) | MouseEvent (FPointerEvent), ReturnValue (FEventReply) | Event override |
| ShowTooltip | - | - | Display item tooltip |
| HideTooltip | - | - | Remove tooltip |
| OnFocusReceived | MyGeometry (FGeometry), InFocusEvent (FFocusEvent) | ReturnValue (FEventReply) | Focus handler |

#### MCP TOOL CALLS

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
    is_pure=False
)

create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="RefreshSlot",
    inputs=[],
    outputs=[],
    is_pure=False
)

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
    is_pure=False
)

create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="ShowTooltip",
    inputs=[],
    outputs=[],
    is_pure=False
)

create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="HideTooltip",
    inputs=[],
    outputs=[],
    is_pure=False
)
```

---

## Section 3.2: WBP_ItemContextMenu

### Step 3.2.1: Create Context Menu Widget

```python
create_umg_widget_blueprint(
    widget_name="WBP_ItemContextMenu",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

---

### Step 3.2.2: Add Context Menu Components

#### ACTUAL IMPLEMENTATION

| Component | Type | Purpose |
|-----------|------|---------|
| CanvasPanel | CanvasPanel | Root container (auto-created) |
| MenuBackground | Border | Dark background container |
| ButtonContainer | VerticalBox | Vertical layout for buttons |
| UseButton | Button | Use item button |
| DropButton | Button | Drop item button |
| UseButtonText | TextBlock | "Use" label (SelfHitTestInvisible) |
| DropButtonText | TextBlock | "Drop" label (SelfHitTestInvisible) |

**IMPORTANT**: Button text components must have `Visibility: SelfHitTestInvisible` to prevent them from blocking button hover events.

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
    parent_size=[100.0, 80.0]
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
add_widget_component_to_widget(
    widget_name="WBP_ItemContextMenu",
    component_name="UseButtonText",
    component_type="TextBlock",
    text="Use"
)

# CRITICAL: Set text to SelfHitTestInvisible so it doesn't block button hover
set_widget_component_property(
    widget_name="WBP_ItemContextMenu",
    component_name="UseButtonText",
    kwargs={"Visibility": "SelfHitTestInvisible"}
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
add_widget_component_to_widget(
    widget_name="WBP_ItemContextMenu",
    component_name="DropButtonText",
    component_type="TextBlock",
    text="Drop"
)

# CRITICAL: Set text to SelfHitTestInvisible
set_widget_component_property(
    widget_name="WBP_ItemContextMenu",
    component_name="DropButtonText",
    kwargs={"Visibility": "SelfHitTestInvisible"}
)
```

---

### Step 3.2.3: Add Context Menu Variables

#### ACTUAL IMPLEMENTATION (3 variables)

| Variable | Type | Purpose |
|----------|------|---------|
| TargetSlotIndex | Integer | Slot this menu is acting on |
| InventoryManager | BP_InventoryManager_C | Reference to manager |
| ParentGrid | UserWidget | Reference to grid (for HideContextMenu) |

---

### Step 3.2.4: Create Context Menu Functions

#### ACTUAL IMPLEMENTATION (3 functions)

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| InitializeMenu | SlotIndex (int32), Manager (BP_InventoryManager_C*), Grid (UserWidget*) | - | Setup menu for slot |
| OnUseClicked | - | - | Handle Use button click |
| OnDropClicked | - | - | Handle Drop button click |

---

### Step 3.2.5: Bind Button Events and Self-Dismissal

The context menu handles its own lifecycle:
- OnMouseLeave hides the menu (self-dismissing)
- Button clicks execute action then hide menu

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

## Section 3.3: WBP_InventoryGrid

### Step 3.3.1: Create Grid Widget Blueprint

```python
create_umg_widget_blueprint(
    widget_name="WBP_InventoryGrid",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

---

### Step 3.3.2: Add Grid Widget Components

#### ACTUAL IMPLEMENTATION

| Component | Type | Purpose |
|-----------|------|---------|
| CanvasPanel | CanvasPanel | Root container (auto-created) |
| GridBackground | Border | Dark background for entire grid |
| TitleText | TextBlock | "Inventory" title |
| SlotsScrollBox | ScrollBox | Scrollable container (for future expansion) |
| SlotsContainer | VerticalBox | Container for rows |
| SlotsGrid | UniformGridPanel | 4x4 grid of slots |

**NOTE**: We use `UniformGridPanel` (not GridPanel) for automatic uniform slot sizing.

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

# UniformGridPanel for slots (NOT GridPanel)
add_widget_component_to_widget(
    widget_name="WBP_InventoryGrid",
    component_name="SlotsGrid",
    component_type="UniformGridPanel",
    position=[10.0, 50.0],
    size=[280.0, 280.0]
)
```

---

### Step 3.3.3: Add Grid Widget Variables

#### ACTUAL IMPLEMENTATION (3 variables)

| Variable | Type | Purpose |
|----------|------|---------|
| SlotWidgets | WBP_InventorySlot_C[] | Array of created slot widgets |
| ActiveContextMenu | WBP_ItemContextMenu_C | Currently shown context menu |
| InventoryManager | Object | Reference to inventory manager |

**NOTE**: We don't need separate SlotWidgetClass/ContextMenuClass variables - we use direct class references in Create Widget nodes.

---

### Step 3.3.4: Create Grid Widget Functions

#### ACTUAL IMPLEMENTATION (5 functions)

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| InitializeGrid | Manager (Object*) | - | Create 16 slots, setup grid |
| RefreshAllSlots | - | - | Update all slot displays |
| ShowContextMenuAtSlot | SlotIndex (int32) | - | Show menu (deprecated, use ShowContextMenuAtPosition) |
| ShowContextMenuAtPosition | SlotIndex (int32), Position (Vector2D) | - | Show menu at specific screen position |
| HideContextMenu | - | - | Remove active context menu |

#### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="InitializeGrid",
    inputs=[
        {"name": "Manager", "type": "Object"}
    ],
    outputs=[],
    is_pure=False
)

create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="RefreshAllSlots",
    inputs=[],
    outputs=[],
    is_pure=False
)

create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="ShowContextMenuAtPosition",
    inputs=[
        {"name": "SlotIndex", "type": "Integer"},
        {"name": "Position", "type": "Vector2D"}
    ],
    outputs=[],
    is_pure=False
)

create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="HideContextMenu",
    inputs=[],
    outputs=[],
    is_pure=False
)
```

---

## Section 3.4: WBP_ItemTooltip

### Step 3.4.1: Create Tooltip Widget Blueprint

```python
create_umg_widget_blueprint(
    widget_name="WBP_ItemTooltip",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)
```

---

### Step 3.4.2: Add Tooltip Components

#### ACTUAL IMPLEMENTATION

| Component | Type | Purpose |
|-----------|------|---------|
| CanvasPanel | CanvasPanel | Root container (auto-created) |
| TooltipBackground | Border | Dark background |
| ContentBox | VerticalBox | Vertical layout for text |
| ItemNameText | TextBlock | Item display name (bold) |
| ItemTypeText | TextBlock | Item type category |
| ItemDescriptionText | TextBlock | Item description |

---

### Step 3.4.3: Create Tooltip Functions

#### ACTUAL IMPLEMENTATION (1 function)

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| SetItemData | ItemDef (S_ItemDefinition) | - | Populate tooltip with item data |

---

## Phase 3 Completion Checklist

### WBP_InventorySlot
- [x] Widget created
- [x] Components: SlotBackground, ItemIcon, StackCountText, SelectionHighlight, ContextMenuAnchor
- [x] Variables: SlotIndex, InventoryManager, IsSelected, CachedSlotData, ParentGrid, TooltipClass, ItemTooltipRef (7 total)
- [x] Functions: InitializeSlot, RefreshSlot, HandleMouseDown, OnMouseButtonDown, ShowTooltip, HideTooltip, OnFocusReceived (7 total)
- [x] Compiles successfully

### WBP_ItemContextMenu
- [x] Widget created
- [x] Components: MenuBackground, ButtonContainer, UseButton, DropButton, UseButtonText, DropButtonText
- [x] Variables: TargetSlotIndex, InventoryManager, ParentGrid (3 total)
- [x] Functions: InitializeMenu, OnUseClicked, OnDropClicked (3 total)
- [x] Button OnClicked events bound
- [x] OnMouseLeave self-dismisses menu
- [x] Button text set to SelfHitTestInvisible (fixes hover)
- [x] Compiles successfully

### WBP_InventoryGrid
- [x] Widget created
- [x] Components: GridBackground, TitleText, SlotsScrollBox, SlotsContainer, SlotsGrid (UniformGridPanel)
- [x] Variables: SlotWidgets, ActiveContextMenu, InventoryManager (3 total)
- [x] Functions: InitializeGrid, RefreshAllSlots, ShowContextMenuAtSlot, ShowContextMenuAtPosition, HideContextMenu (5 total)
- [x] Compiles successfully

### WBP_ItemTooltip
- [x] Widget created
- [x] Components: TooltipBackground, ContentBox, ItemNameText, ItemTypeText, ItemDescriptionText
- [x] Functions: SetItemData (1 total)
- [x] Compiles successfully

### All Done Via MCP Tools
- [x] `create_umg_widget_blueprint` used for all 4 widgets
- [x] `add_widget_component_to_widget` used for components
- [x] `create_parent_and_child_widget_components` used for nested layouts
- [x] `set_widget_component_property` used for styling and SelfHitTestInvisible
- [x] `add_blueprint_variable` used for all variables
- [x] `create_custom_blueprint_function` used for all functions
- [x] `bind_widget_component_event` used for event binding
- [x] `compile_blueprint` successful for all widgets

---

## Key Implementation Notes

### 1. UniformGridPanel vs GridPanel
Use `UniformGridPanel` for the slot grid - it automatically sizes children uniformly. Regular `GridPanel` requires manual column/row configuration.

### 2. SelfHitTestInvisible for Button Text
Button text blocks MUST have `Visibility: SelfHitTestInvisible` to prevent them from blocking mouse events on the parent button. This is set via:
```python
set_widget_component_property(
    widget_name="WBP_ItemContextMenu",
    component_name="UseButtonText",
    kwargs={"Visibility": "SelfHitTestInvisible"}
)
```

### 3. Context Menu Self-Management
The context menu handles its own lifecycle:
- OnMouseLeave on the menu itself calls HideContextMenu
- Opening a new context menu auto-closes any existing one
- Left-click on slots (not right-click) also hides the menu

### 4. Context Menu Positioning
Use `ShowContextMenuAtPosition` with the slot's bottom-right corner position, not mouse position. This ensures the menu is always reachable from the slot.

---

## Next Phase

Continue to [Phase4_Integration.md](Phase4_Integration.md) to connect the inventory to the player character and input system.
