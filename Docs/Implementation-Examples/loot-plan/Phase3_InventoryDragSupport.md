# Phase 3: Inventory Drag Support

> **STATUS: 📋 PLANNED**

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Add drag functionality to existing inventory widgets:
- Grey overlay on source slot
- Left-click starts drag
- Drag widget spawns and follows cursor

**USER TEST at end of phase:** Click inventory slot → grey overlay appears, drag widget follows cursor.

## Dependencies

- Phase 2 completed (WBP_DraggedItem exists)
- Existing WBP_InventorySlot, WBP_InventoryGrid

---

## Step 3.1: Add DragOverlay to WBP_InventorySlot

### AIM
Add grey overlay component that shows during drag.

### MCP TOOL CALLS

```python
# Add DragOverlay component
add_widget_component_to_widget(
    widget_name="WBP_InventorySlot",
    component_name="DragOverlay",
    component_type="Border",
    position=[0, 0],
    size=[64, 64],
    kwargs={"background_color": [0.3, 0.3, 0.3, 0.6]}
)

# Set DragOverlay to Hidden by default
set_widget_component_property(
    widget_name="WBP_InventorySlot",
    component_name="DragOverlay",
    kwargs={"Visibility": "Hidden"}
)

# Add IsDragSource variable
add_blueprint_variable(
    blueprint_name="WBP_InventorySlot",
    variable_name="IsDragSource",
    variable_type="Boolean",
    is_exposed=False
)

compile_blueprint(blueprint_name="WBP_InventorySlot")
```

### VERIFICATION
- [ ] DragOverlay added to slot
- [ ] Overlay initially Hidden
- [ ] IsDragSource variable added
- [ ] Blueprint compiles

---

## Step 3.2: Create SetDragOverlay Function in WBP_InventorySlot

### AIM
Function to show/hide the drag overlay.

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventorySlot",
    function_name="SetDragOverlay",
    inputs=[{"name": "bVisible", "type": "Boolean"}],
    outputs=[],
    is_pure=False,
    category="Slot"
)
```

### LOGIC
```
Input: bVisible (bool)

1. If bVisible:
   - Set DragOverlay visibility = Visible
2. Else:
   - Set DragOverlay visibility = Hidden
3. Set IsDragSource = bVisible
```

### VERIFICATION
- [ ] SetDragOverlay function created
- [ ] Logic implemented
- [ ] Blueprint compiles

---

## Step 3.3: Add Drag Variables to WBP_InventoryGrid

### AIM
Add state tracking for active drag operations.

### MCP TOOL CALLS

```python
# Add drag state variables
add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="ActiveDragWidget",
    variable_type="UserWidget",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="DragSourceSlotIndex",
    variable_type="Integer",
    is_exposed=False
)

set_blueprint_variable_value(
    blueprint_name="WBP_InventoryGrid",
    variable_name="DragSourceSlotIndex",
    value=-1
)

add_blueprint_variable(
    blueprint_name="WBP_InventoryGrid",
    variable_name="DragSourceContainer",
    variable_type="UserWidget",
    is_exposed=False
)

compile_blueprint(blueprint_name="WBP_InventoryGrid")
```

### VERIFICATION
- [ ] Variables added
- [ ] DragSourceSlotIndex defaults to -1
- [ ] Blueprint compiles

---

## Step 3.4: Create StartDrag Function in WBP_InventoryGrid

### AIM
Begin drag operation: show overlay, create drag widget.

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="StartDrag",
    inputs=[{"name": "SlotIndex", "type": "Integer"}],
    outputs=[],
    is_pure=False,
    category="DragDrop"
)
```

### LOGIC
```
Input: SlotIndex (int)

1. Set DragSourceSlotIndex = SlotIndex
2. Set DragSourceContainer = Self
3. Get slot widget at SlotIndex from SlotWidgets array
4. Call SetDragOverlay(true) on slot widget
5. Get slot data (CachedSlotData) from slot widget
6. Look up ItemDef from DataTable using ItemRowName
7. Create WBP_DraggedItem widget
8. Call SetDragData on drag widget with slot data, self, index, itemdef
9. Set ActiveDragWidget = created widget
10. Add drag widget to viewport (Z-Order: 1000)
```

### VERIFICATION
- [ ] StartDrag function created
- [ ] Overlay shows on source slot
- [ ] Drag widget created and added to viewport
- [ ] Blueprint compiles

---

## Step 3.5: Create CancelDrag Function in WBP_InventoryGrid

### AIM
Cancel drag operation: hide overlay, remove drag widget.

### MCP TOOL CALLS

```python
create_custom_blueprint_function(
    blueprint_name="WBP_InventoryGrid",
    function_name="CancelDrag",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="DragDrop"
)
```

### LOGIC
```
1. If DragSourceSlotIndex >= 0:
   - Get slot widget at DragSourceSlotIndex
   - Call SetDragOverlay(false) on slot widget
2. If ActiveDragWidget is valid:
   - Remove ActiveDragWidget from parent
3. Set ActiveDragWidget = None
4. Set DragSourceSlotIndex = -1
5. Set DragSourceContainer = None
```

### VERIFICATION
- [ ] CancelDrag function created
- [ ] Overlay hides on cancel
- [ ] Drag widget removed
- [ ] State variables reset
- [ ] Blueprint compiles

---

## Step 3.6: Modify WBP_InventorySlot Left-Click Handler

### AIM
Left-click on slot with item starts drag.

### CURRENT BEHAVIOR
HandleMouseDown handles right-click for context menu.

### NEW BEHAVIOR
```
OnMouseButtonDown:
1. Get mouse button from event
2. If LEFT CLICK:
   a. Get CachedSlotData
   b. If ItemRowName is NOT empty (has item):
      - Call ParentGrid.StartDrag(SlotIndex)
      - Return Handled
   c. Else:
      - Hide context menu if open
      - Return Handled
3. If RIGHT CLICK:
   a. (Existing context menu logic)
4. Return Unhandled
```

### MCP TOOL CALLS

```python
# Modify existing HandleMouseDown or create OnMouseButtonDown override
create_widget_input_handler(
    widget_name="WBP_InventorySlot",
    input_type="MouseButton",
    input_event="LeftMouseButton",
    trigger="Pressed",
    handler_name="OnSlotLeftClick"
)
```

### VERIFICATION
- [ ] Left-click starts drag
- [ ] Only works on slots with items
- [ ] Right-click still shows context menu
- [ ] Blueprint compiles

---

## Step 3.7: Test Drag from Inventory

### USER TEST
1. Launch Unreal Editor
2. Play in Editor
3. Open inventory (I key or existing method)
4. Left-click on slot with item:
   - [ ] Grey overlay appears on clicked slot
   - [ ] Drag widget appears at cursor
   - [ ] Drag widget follows cursor
5. Press Escape or click empty area:
   - [ ] Drag widget disappears
   - [ ] Grey overlay disappears
   - [ ] Item stays in original slot
6. Right-click on slot:
   - [ ] Context menu still works

---

## Phase 3 Completion Checklist

### WBP_InventorySlot Updates
- [ ] DragOverlay component added
- [ ] IsDragSource variable added
- [ ] SetDragOverlay function implemented
- [ ] Left-click starts drag

### WBP_InventoryGrid Updates
- [ ] Drag state variables added
- [ ] StartDrag function implemented
- [ ] CancelDrag function implemented

### Functionality Verified (USER TEST)
- [ ] Left-click on item shows grey overlay
- [ ] Drag widget follows cursor
- [ ] Cancel restores original state
- [ ] Right-click context menu still works

### Ready for Phase 4
- [ ] All blueprints compile
- [ ] Drag start/cancel working
- [ ] Proceed to Phase 4: Lootable Actor + Data

---

## Next Phase

Continue to [Phase4_LootableActorAndData.md](Phase4_LootableActorAndData.md) to create the lootable actor and connect real data.
