# Inventory System - Overview

> **STATUS: ✅ PHASES 1-4 COMPLETE | 🔄 PHASE 5 IN PROGRESS**

> **REMINDER**: ALL functionality in this inventory system MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted. This serves as a comprehensive test of AI-driven Blueprint development.

## Implementation Status

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 1: Data Layer | ✅ COMPLETE | Enums, Structs, DataTable created |
| Phase 2: Blueprint Logic | ✅ COMPLETE | BP_InventoryManager with 11 functions |
| Phase 3: UI Widgets | ✅ COMPLETE | All 4 widgets + tooltip widget |
| Phase 4: Integration | ✅ COMPLETE | Player integration, I key toggle |
| Phase 5: Testing | 🔄 IN PROGRESS | UX refinements ongoing |

---

## Purpose

This document series provides a step-by-step plan for building a complete inventory system in Unreal Engine 5.7 using only MCP (Model Context Protocol) tools. The inventory system is designed to be modular, extensible, and ready for future integration with Equipment and Trading modules.

## System Requirements

- **Grid Size**: 4x4 (16 slots)
- **Item Types**: Weapon, Armor, Consumable, Material, QuestItem
- **Data Storage**: DataTable-driven item definitions
- **Stacking**: Enabled with configurable max stack size per item
- **UI Components**:
  - Grid-based inventory container (UniformGridPanel)
  - Individual item slots with icon and stack count
  - Right-click context menu (Use/Drop options)
  - Item tooltip on hover

## Architecture

### Data Flow

```
DT_Items (DataTable)
    │ Item definitions with properties
    ▼
BP_InventoryManager (Actor Component)
    │ Core logic: AddItem, RemoveItem, UseItem, DropItem
    │ Stores: Array<S_InventorySlot>
    ▼
WBP_InventoryGrid (Widget Blueprint)
    │ 4x4 UniformGridPanel of slot widgets
    │ Handles context menu display
    ▼
WBP_InventorySlot (Widget Blueprint)
    │ Displays item icon and stack count
    │ Handles mouse events (hover, right-click)
    ▼
WBP_ItemContextMenu (Widget Blueprint)
    │ Use/Drop buttons (self-dismissing on mouse leave)
    │ Calls back to InventoryManager
    ▼
WBP_ItemTooltip (Widget Blueprint)
    │ Shows item name, description, type on hover
```

### Implemented Asset Structure

```
/Game/Inventory/
├── Data/
│   ├── E_ItemType.uasset          (Enum - 5 item categories) ✓
│   ├── S_ItemDefinition.uasset    (Struct - 9 properties) ✓
│   ├── S_InventorySlot.uasset     (Struct - 3 properties) ✓
│   └── DT_Items.uasset            (DataTable - 5 sample items) ✓
├── Blueprints/
│   └── BP_InventoryManager.uasset (Actor Component - 11 functions, 7 variables) ✓
└── UI/
    ├── WBP_InventoryGrid.uasset   (Main container - 5 functions, 3 variables) ✓
    ├── WBP_InventorySlot.uasset   (Slot widget - 7 functions, 7 variables) ✓
    ├── WBP_ItemContextMenu.uasset (Context menu - 3 functions, 3 variables) ✓
    └── WBP_ItemTooltip.uasset     (Tooltip widget - 1 function) ✓
```

---

## Implemented Components Detail

### BP_InventoryManager (Actor Component)

**Variables (7):**
| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| InventorySlots | S_InventorySlot[] | [] | Array of slot data |
| ItemsDataTable | DataTable | DT_Items | Reference to item definitions |
| GridWidth | Integer | 4 | Grid columns |
| GridHeight | Integer | 4 | Grid rows |
| TempCount | Integer | 0 | Temp variable for operations |
| TempSlotIndex | Integer | 0 | Temp variable for operations |
| TempRemainingQty | Integer | 0 | Temp variable for operations |

**Functions (11):**
| Function | Type | Inputs | Outputs |
|----------|------|--------|---------|
| InitializeInventory | Impure | - | Success (bool) |
| GetItemDefinition | Pure | ItemRowName (Name) | ItemDef (S_ItemDefinition), Found (bool) |
| FindSlotWithItem | Pure | ItemRowName (Name), RequireSpace (bool) | SlotIndex (int32), Found (bool) |
| FindEmptySlot | Pure | - | SlotIndex (int32), Found (bool) |
| AddItem | Impure | ItemRowName (Name), Quantity (int32) | AmountAdded (int32), Success (bool) |
| RemoveItem | Impure | ItemRowName (Name), Quantity (int32) | AmountRemoved (int32), Success (bool) |
| RemoveItemFromSlot | Impure | SlotIndex (int32), Quantity (int32) | ItemRowName (Name), AmountRemoved (int32), Success (bool) |
| UseItem | Impure | SlotIndex (int32) | ItemRowName (Name), Success (bool) |
| DropItem | Impure | SlotIndex (int32), Quantity (int32) | ItemRowName (Name), AmountDropped (int32), Success (bool) |
| GetSlotData | Pure | SlotIndex (int32) | SlotData (S_InventorySlot), ItemDef (S_ItemDefinition), IsEmpty (bool) |
| GetTotalItemCount | Pure | ItemRowName (Name) | TotalCount (int32) |

---

### WBP_InventoryGrid (Widget Blueprint)

**Components:**
- CanvasPanel (root)
- GridBackground (Border)
- TitleText (TextBlock)
- SlotsScrollBox (ScrollBox)
- SlotsContainer (VerticalBox)
- SlotsGrid (UniformGridPanel) - **NOTE: Uses UniformGridPanel, not GridPanel**

**Variables (3):**
- SlotWidgets (WBP_InventorySlot_C[]) - Array of slot widgets
- ActiveContextMenu (WBP_ItemContextMenu_C) - Currently shown context menu
- InventoryManager (Object) - Reference to manager

**Functions (5):**
- InitializeGrid(Manager) - Create 16 slots, add to UniformGridPanel
- RefreshAllSlots() - Update all slot displays
- ShowContextMenuAtSlot(SlotIndex) - Show context menu (deprecated)
- ShowContextMenuAtPosition(SlotIndex, Position) - Show at specific screen position
- HideContextMenu() - Hide active context menu

---

### WBP_InventorySlot (Widget Blueprint)

**Components:**
- CanvasPanel (root)
- SlotBackground (Border)
- ItemIcon (Image)
- StackCountText (TextBlock)
- SelectionHighlight (Border)
- ContextMenuAnchor (SizeBox) - Anchor for context menu positioning

**Variables (7):**
- SlotIndex (Integer) - Position in grid (0-15)
- InventoryManager (BP_InventoryManager_C) - Manager reference
- IsSelected (Boolean) - Selection state
- CachedSlotData (S_InventorySlot) - Cached data
- ParentGrid (UserWidget) - Grid reference
- TooltipClass (Class) - Tooltip widget class
- ItemTooltipRef (UserWidget) - Active tooltip

**Functions (7):**
- InitializeSlot(Index, Manager, Grid) - Setup slot
- RefreshSlot() - Update display from data
- HandleMouseDown(Geometry, MouseEvent) - Mouse handler
- OnMouseButtonDown() - Event override
- ShowTooltip() - Show item tooltip on hover
- HideTooltip() - Hide tooltip
- OnFocusReceived() - Focus handler

**Recent UX Improvements:**
- Context menu appears at slot's bottom-right corner (not mouse position)
- Left-click anywhere hides context menu
- Mouse leave from slot doesn't hide context menu (menu handles its own lifecycle)

---

### WBP_ItemContextMenu (Widget Blueprint)

**Components:**
- CanvasPanel (root)
- MenuBackground (Border)
- ButtonContainer (VerticalBox)
- UseButton (Button)
- DropButton (Button)
- UseButtonText (TextBlock) - **Visibility: SelfHitTestInvisible**
- DropButtonText (TextBlock) - **Visibility: SelfHitTestInvisible**

**Variables (3):**
- TargetSlotIndex (Integer) - Slot being acted on
- InventoryManager (BP_InventoryManager_C) - Manager reference
- ParentGrid (UserWidget) - Grid reference

**Functions (3):**
- InitializeMenu(SlotIndex, Manager, Grid) - Setup menu
- OnUseClicked() - Handle Use button
- OnDropClicked() - Handle Drop button

**Key UX Features:**
- Button text set to SelfHitTestInvisible (fixes hover detection)
- OnMouseLeave hides menu (self-dismissing)
- Opening context menu on another slot auto-hides previous

---

### WBP_ItemTooltip (Widget Blueprint)

**Components:**
- CanvasPanel (root)
- TooltipBackground (Border)
- ContentBox (VerticalBox)
- ItemNameText (TextBlock)
- ItemTypeText (TextBlock)
- ItemDescriptionText (TextBlock)

**Functions (1):**
- SetItemData(ItemDef) - Populate tooltip with S_ItemDefinition data

---

## Development Phases

| Phase | Document | Focus | Status |
|-------|----------|-------|--------|
| 1 | [Phase1_DataLayer.md](Phase1_DataLayer.md) | Enums, Structs, DataTable | ✅ Complete |
| 2 | [Phase2_BlueprintLogic.md](Phase2_BlueprintLogic.md) | BP_InventoryManager functions | ✅ Complete |
| 3 | [Phase3_UIWidgets.md](Phase3_UIWidgets.md) | Widget Blueprints (4 widgets) | ✅ Complete |
| 4 | [Phase4_Integration.md](Phase4_Integration.md) | Player integration, input | ✅ Complete |
| 5 | [Phase5_Testing.md](Phase5_Testing.md) | Verification checklist | 🔄 In Progress |

---

## MCP Tools Reference

### Project Tools (`project_tools`)
| Tool | Purpose |
|------|---------|
| `create_folder` | Create Content folder structure |
| `create_enum` | Create E_ItemType enum |
| `create_struct` | Create S_ItemDefinition, S_InventorySlot |

### DataTable Tools (`datatable_tools`)
| Tool | Purpose |
|------|---------|
| `create_datatable` | Create DT_Items DataTable |
| `get_datatable_row_names` | Get GUID-based field names (CRITICAL) |
| `add_rows_to_datatable` | Populate sample items |

### Blueprint Tools (`blueprint_tools`)
| Tool | Purpose |
|------|---------|
| `create_blueprint` | Create BP_InventoryManager |
| `add_blueprint_variable` | Add variables to blueprints |
| `create_custom_blueprint_function` | Create modular functions |
| `compile_blueprint` | Compile blueprints |

### Node Tools (`blueprint_action_tools`, `node_tools`)
| Tool | Purpose |
|------|---------|
| `search_blueprint_actions` | Discover available nodes |
| `create_node_by_action_name` | Create Blueprint nodes |
| `connect_blueprint_nodes` | Connect node pins |
| `auto_arrange_nodes` | Auto-layout nodes |
| `delete_orphaned_nodes` | Clean disconnected nodes |

### UMG Widget Tools (`umg_tools`)
| Tool | Purpose |
|------|---------|
| `create_umg_widget_blueprint` | Create widget blueprints |
| `add_widget_component_to_widget` | Add UI components |
| `add_child_widget_component_to_parent` | Create nested widgets |
| `set_widget_component_property` | Configure widget properties (including Visibility) |
| `bind_widget_component_event` | Bind click/mouse events |

---

## Design Principles

### 1. Modular Functions
All inventory logic is encapsulated in custom Blueprint functions. The main Event Graph should only contain initialization and event dispatching - NO heavy logic.

### 2. DataTable-Driven
Item definitions are stored in a DataTable, making it easy to add new items without code changes. The inventory stores references (row names) rather than copying all item data.

### 3. Direct Function Calls (Not Event Dispatchers)
UI updates happen via direct function calls (RefreshAllSlots) rather than event dispatchers. This simplifies the implementation.

### 4. Self-Managing UI
Widgets handle their own lifecycle where possible:
- Context menu dismisses itself on mouse leave
- Tooltips show/hide based on slot hover
- Slots refresh themselves when inventory changes

### 5. Future-Ready
The architecture supports future modules:
- **Equipment Module**: EquipItem/UnequipItem functions, equipment slots
- **Trading Module**: TransferItem between inventories, value calculations

---

## Critical Implementation Notes

### DataTable Field Names (GUID Pattern)
When adding rows to DataTables, you MUST first call `get_datatable_row_names()` to retrieve the GUID-based field names. Unreal stores properties with auto-generated GUIDs like:
```
ItemName_2_6E790E8C43602AE89B031BA4C77F112E
```
These exact names must be used when adding or updating rows.

### UniformGridPanel vs GridPanel
Use `UniformGridPanel` for the inventory grid - it automatically sizes children uniformly without manual column/row configuration.

### SelfHitTestInvisible for Button Text
Button text labels MUST have `Visibility: SelfHitTestInvisible` to prevent them from blocking hover events on the parent button:
```python
set_widget_component_property(
    widget_name="WBP_ItemContextMenu",
    component_name="UseButtonText",
    kwargs={"Visibility": "SelfHitTestInvisible"}
)
```

### Struct References
When referencing structs in Blueprint variables, use the full path:
```
/Game/Inventory/Data/S_InventorySlot
```

---

## Success Criteria

At the end of implementation:
- [x] Zero manual file creation in Unreal Editor
- [x] All assets created via MCP tools
- [x] Inventory opens/closes with I key
- [x] Items display with icons and stack counts
- [x] Right-click context menu works
- [x] Context menu positioned at slot corner
- [x] Use/Drop functionality operational
- [x] Quest items cannot be dropped
- [x] Tooltip shows on hover
- [x] Context menu hides on click outside
- [x] Button hover works correctly (text doesn't interfere)

---

## Known Issues & Fixes

### Resolved Issues
| Issue | Solution | MCP Tool Used |
|-------|----------|---------------|
| Context menu at mouse position | Changed to slot's bottom-right corner | ShowContextMenuAtPosition |
| Button text blocking hover | Set Visibility to SelfHitTestInvisible | set_widget_component_property |
| Context menu not hiding on click | Added left-click handler in HandleMouseDown | create_node_by_action_name |
| Menu hiding when moving to it | Context menu self-manages via OnMouseLeave | delete_orphaned_nodes |

### Future Enhancements
- Drag and drop between slots
- Item splitting (shift+click)
- Quick-use hotbar
- Sort/filter inventory
- Search functionality
- Equipment slot integration
