# Phase 2: Drag Widget

> **STATUS: 📋 PLANNED**

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Create WBP_DraggedItem widget that follows the mouse cursor.

**USER TEST at end of phase:** Spawn drag widget and verify it follows cursor smoothly.

## Dependencies

- Phase 1 completed (base widgets exist)

---

## Step 2.1: Create WBP_DraggedItem Widget

### AIM
Widget that displays dragged item and follows cursor.

### COMPONENTS

| Component | Type | Size | Properties |
|-----------|------|------|------------|
| CanvasPanel | Canvas Panel | 64x64 | Root, slight transparency |
| ItemIcon | Image | 56x56 | Centered item texture |
| StackCountText | TextBlock | - | Bottom-right stack count, font size 12 |

### VARIABLES

| Variable | Type | Purpose |
|----------|------|---------|
| DraggedSlotData | S_InventorySlot | Data being dragged |
| SourceContainer | UserWidget | Container item came from |
| SourceSlotIndex | Integer | Original slot index |
| ItemDefRef | S_ItemDefinition | Cached item definition |

### FUNCTIONS

| Function | Inputs | Outputs | Purpose |
|----------|--------|---------|---------|
| SetDragData | SlotData, Container, SlotIndex, ItemDef | - | Initialize with data |
| UpdatePosition | - | - | Move to mouse position |

### MCP TOOL CALLS

```python
create_umg_widget_blueprint(
    widget_name="WBP_DraggedItem",
    parent_class="UserWidget",
    path="/Game/Inventory/UI"
)

# Add ItemIcon
add_widget_component_to_widget(
    widget_name="WBP_DraggedItem",
    component_name="ItemIcon",
    component_type="Image",
    position=[4, 4],
    size=[56, 56],
    kwargs={}
)

# Add StackCountText
add_widget_component_to_widget(
    widget_name="WBP_DraggedItem",
    component_name="StackCountText",
    component_type="TextBlock",
    position=[40, 46],
    size=[20, 16],
    kwargs={"text": "", "font_size": 12}
)

# Add variables
add_blueprint_variable(
    blueprint_name="WBP_DraggedItem",
    variable_name="DraggedSlotData",
    variable_type="/Game/Inventory/Data/S_InventorySlot",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_DraggedItem",
    variable_name="SourceContainer",
    variable_type="UserWidget",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_DraggedItem",
    variable_name="SourceSlotIndex",
    variable_type="Integer",
    is_exposed=False
)

add_blueprint_variable(
    blueprint_name="WBP_DraggedItem",
    variable_name="ItemDefRef",
    variable_type="/Game/Inventory/Data/S_ItemDefinition",
    is_exposed=False
)

# Create SetDragData function
create_custom_blueprint_function(
    blueprint_name="WBP_DraggedItem",
    function_name="SetDragData",
    inputs=[
        {"name": "SlotData", "type": "S_InventorySlot"},
        {"name": "Container", "type": "UserWidget"},
        {"name": "SlotIndex", "type": "Integer"},
        {"name": "ItemDef", "type": "S_ItemDefinition"}
    ],
    outputs=[],
    is_pure=False,
    category="Drag"
)

# Create UpdatePosition function
create_custom_blueprint_function(
    blueprint_name="WBP_DraggedItem",
    function_name="UpdatePosition",
    inputs=[],
    outputs=[],
    is_pure=False,
    category="Drag"
)

compile_blueprint(blueprint_name="WBP_DraggedItem")
```

### VERIFICATION
- [ ] WBP_DraggedItem created at /Game/Inventory/UI/
- [ ] Components added
- [ ] Variables added
- [ ] Functions created
- [ ] Blueprint compiles

---

## Step 2.2: Implement SetDragData Function

### AIM
Initialize the drag widget with item data.

### LOGIC
```
Input: SlotData, Container, SlotIndex, ItemDef

1. Set DraggedSlotData = SlotData
2. Set SourceContainer = Container
3. Set SourceSlotIndex = SlotIndex
4. Set ItemDefRef = ItemDef
5. Set ItemIcon texture = ItemDef.Icon
6. If SlotData.StackCount > 1:
   - Set StackCountText = SlotData.StackCount (as string)
   - Set StackCountText visibility = Visible
7. Else:
   - Set StackCountText visibility = Hidden
```

### MCP TOOL CALLS

```python
# Implement SetDragData function graph
# Node sequence:
# FunctionEntry → Set DraggedSlotData → Set SourceContainer → Set SourceSlotIndex
# → Set ItemDefRef → Break ItemDef → Set ItemIcon Brush → Branch (StackCount > 1)
# → True: Set StackCountText, Set Visibility Visible
# → False: Set Visibility Hidden
# → Return
```

### VERIFICATION
- [ ] SetDragData function implemented
- [ ] Icon displays correctly
- [ ] Stack count shows when > 1
- [ ] Stack count hidden when = 1
- [ ] Blueprint compiles

---

## Step 2.3: Implement UpdatePosition Function

### AIM
Move widget to follow mouse cursor.

### LOGIC
```
1. Get Player Controller → Get Mouse Position
2. Get widget slot (if using Canvas Panel slot)
3. Set position = Mouse Position - (32, 32)  // Center on cursor
```

### MCP TOOL CALLS

```python
# Implement UpdatePosition function graph
# Node sequence:
# FunctionEntry → Get Player Controller (0) → Get Mouse Position
# → Subtract Vector2D (32, 32) → Set Position in Viewport
```

### VERIFICATION
- [ ] UpdatePosition implemented
- [ ] Widget moves to mouse position
- [ ] Blueprint compiles

---

## Step 2.4: Add Tick Event

### AIM
Call UpdatePosition every frame.

### LOGIC
```
Event Tick → Call UpdatePosition()
```

### MCP TOOL CALLS

```python
# In EventGraph:
# Event Tick → UpdatePosition call
```

### VERIFICATION
- [ ] Tick event connected to UpdatePosition
- [ ] Blueprint compiles

---

## Step 2.5: Test Cursor Following

### AIM
Verify drag widget follows cursor smoothly.

### TEST APPROACH
Temporarily add test logic to player BeginPlay:

```python
# Temporary test code (remove after verification):
# BeginPlay → Create WBP_DraggedItem → Add to Viewport (Z-Order 1000)
```

### USER TEST
1. Launch Unreal Editor
2. Play in Editor
3. Verify:
   - [ ] Drag widget appears on screen
   - [ ] Widget follows mouse cursor smoothly
   - [ ] No visual lag or jitter
   - [ ] Widget stays centered on cursor

### CLEANUP
After verification, remove test code from player BeginPlay.

---

## Phase 2 Completion Checklist

### Assets Created
- [ ] WBP_DraggedItem with icon and stack count

### Functions Implemented
- [ ] SetDragData populates display
- [ ] UpdatePosition follows mouse
- [ ] Tick calls UpdatePosition

### Visual Verification (USER TEST)
- [ ] Drag widget follows cursor smoothly
- [ ] Icon displays correctly
- [ ] Stack count displays when applicable

### Ready for Phase 3
- [ ] Widget compiles
- [ ] Cursor following verified
- [ ] Proceed to Phase 3: Inventory Drag Support

---

## Next Phase

Continue to [Phase3_InventoryDragSupport.md](Phase3_InventoryDragSupport.md) to add drag functionality to existing inventory.
