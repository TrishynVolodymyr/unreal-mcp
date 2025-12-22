# Inventory System - Overview

> **REMINDER**: ALL functionality in this inventory system MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted. This serves as a comprehensive test of AI-driven Blueprint development.

## Purpose

This document series provides a step-by-step plan for building a complete inventory system in Unreal Engine 5.7 using only MCP (Model Context Protocol) tools. The inventory system is designed to be modular, extensible, and ready for future integration with Equipment and Trading modules.

## System Requirements

- **Grid Size**: 4x4 (16 slots)
- **Item Types**: Weapon, Armor, Consumable, Material, QuestItem
- **Data Storage**: DataTable-driven item definitions
- **Stacking**: Enabled with configurable max stack size per item
- **UI Components**:
  - Grid-based inventory container
  - Individual item slots with icon and stack count
  - Right-click context menu (Use/Drop options)

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
    │ 4x4 grid of slot widgets
    │ Handles context menu display
    ▼
WBP_InventorySlot (Widget Blueprint)
    │ Displays item icon and stack count
    │ Handles mouse events
    ▼
WBP_ItemContextMenu (Widget Blueprint)
    │ Use/Drop buttons
    │ Calls back to InventoryManager
```

### Asset Structure

```
/Game/Inventory/
├── Data/
│   ├── E_ItemType.uasset          (Enum - 5 item categories)
│   ├── S_ItemDefinition.uasset    (Struct - item properties)
│   ├── S_InventorySlot.uasset     (Struct - slot data)
│   └── DT_Items.uasset            (DataTable - item database)
├── Blueprints/
│   └── BP_InventoryManager.uasset (Actor Component - core logic)
└── UI/
    ├── WBP_InventoryGrid.uasset   (Main container widget)
    ├── WBP_InventorySlot.uasset   (Individual slot widget)
    └── WBP_ItemContextMenu.uasset (Right-click menu widget)
```

## Development Phases

| Phase | Document | Focus |
|-------|----------|-------|
| 1 | [Phase1_DataLayer.md](Phase1_DataLayer.md) | Enums, Structs, DataTable |
| 2 | [Phase2_BlueprintLogic.md](Phase2_BlueprintLogic.md) | BP_InventoryManager functions |
| 3 | [Phase3_UIWidgets.md](Phase3_UIWidgets.md) | Widget Blueprints |
| 4 | [Phase4_Integration.md](Phase4_Integration.md) | Player integration, input |
| 5 | [Phase5_Testing.md](Phase5_Testing.md) | Verification checklist |

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

### UMG Widget Tools (`umg_tools`)
| Tool | Purpose |
|------|---------|
| `create_umg_widget_blueprint` | Create widget blueprints |
| `add_widget_component_to_widget` | Add UI components |
| `create_parent_and_child_widget_components` | Create nested widgets |
| `set_widget_component_property` | Configure widget properties |
| `bind_widget_component_event` | Bind click/mouse events |

## Design Principles

### 1. Modular Functions
All inventory logic is encapsulated in custom Blueprint functions. The main Event Graph should only contain initialization and event dispatching - NO heavy logic.

### 2. DataTable-Driven
Item definitions are stored in a DataTable, making it easy to add new items without code changes. The inventory stores references (row names) rather than copying all item data.

### 3. Event-Driven Updates
The InventoryManager broadcasts events (OnInventoryChanged, OnItemUsed, OnItemDropped) that UI and game systems can subscribe to.

### 4. Future-Ready
The architecture supports future modules:
- **Equipment Module**: EquipItem/UnequipItem functions, equipment slots
- **Trading Module**: TransferItem between inventories, value calculations

## Critical Implementation Notes

### DataTable Field Names (GUID Pattern)
When adding rows to DataTables, you MUST first call `get_datatable_row_names()` to retrieve the GUID-based field names. Unreal stores properties with auto-generated GUIDs like:
```
ItemName_2_6E790E8C43602AE89B031BA4C77F112E
```
These exact names must be used when adding or updating rows.

### Struct References
When referencing structs in Blueprint variables, use the full path:
```
/Game/Inventory/Data/S_InventorySlot
```

### Widget Class References
For variables holding widget classes:
```
Class<WBP_InventorySlot>
```

## Success Criteria

At the end of implementation:
- [ ] Zero manual file creation in Unreal Editor
- [ ] All assets created via MCP tools
- [ ] Inventory opens/closes with I key
- [ ] Items display with icons and stack counts
- [ ] Right-click context menu works
- [ ] Use/Drop functionality operational
- [ ] Quest items cannot be dropped
