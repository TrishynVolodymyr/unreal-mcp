# Phase 1: Data Layer

> **REMINDER**: ALL functionality MUST be implemented using MCP tools.
> NO manual Unreal Editor work is permitted.

## Objective

Establish the data foundation for the inventory system by creating:
- Folder structure
- E_ItemType enum (item categories)
- S_ItemDefinition struct (item properties)
- S_InventorySlot struct (slot data)
- DT_Items DataTable (item database)

## Dependencies

None - this is the foundation phase.

---

## Step 1.1: Create Folder Structure

### AIM
Create organized Content folders for inventory assets.

### USAGE EXAMPLES
```python
create_folder(folder_path="Content/Inventory")
create_folder(folder_path="Content/Inventory/Data")
```

### WORKFLOW
1. Create main Inventory folder
2. Create Data subfolder for enums, structs, DataTable
3. Create Blueprints subfolder for BP_InventoryManager
4. Create UI subfolder for widget blueprints

### MCP TOOL CALLS

```python
# Main folder
create_folder(folder_path="Content/Inventory")

# Subfolders
create_folder(folder_path="Content/Inventory/Data")
create_folder(folder_path="Content/Inventory/Blueprints")
create_folder(folder_path="Content/Inventory/UI")
```

### VERIFICATION
- [ ] /Game/Inventory/ folder exists in Content Browser
- [ ] /Game/Inventory/Data/ subfolder exists
- [ ] /Game/Inventory/Blueprints/ subfolder exists
- [ ] /Game/Inventory/UI/ subfolder exists

---

## Step 1.2: Create E_ItemType Enum

### AIM
Define the 5 categories of items the inventory can hold. This enum is used for filtering, display, and future module integration.

### USAGE EXAMPLES
```python
# In S_ItemDefinition struct
{"name": "ItemType", "type": "E_ItemType", "description": "Category of the item"}

# In UI filtering
# Filter inventory to show only Consumables
```

### WORKFLOW
1. Create enum with 5 values
2. Each value has a name and description
3. Enum will be referenced by S_ItemDefinition struct

### MCP TOOL CALL

```python
create_enum(
    enum_name="E_ItemType",
    values=[
        {"name": "Weapon", "description": "Melee or ranged weapons for combat"},
        {"name": "Armor", "description": "Protective equipment worn by player"},
        {"name": "Consumable", "description": "Items that can be used/consumed (potions, food)"},
        {"name": "Material", "description": "Crafting materials and resources"},
        {"name": "QuestItem", "description": "Special items for quests (cannot be dropped)"}
    ],
    path="/Game/Inventory/Data",
    description="Categories for inventory items"
)
```

### VERIFICATION
- [ ] E_ItemType asset created at /Game/Inventory/Data/E_ItemType
- [ ] Enum has exactly 5 values
- [ ] Values are: Weapon, Armor, Consumable, Material, QuestItem

---

## Step 1.3: Create S_ItemDefinition Struct

### AIM
Define the schema for item definitions stored in the DataTable. This struct contains all properties needed to describe an item.

### USAGE EXAMPLES
```python
# Used as row type for DT_Items DataTable
create_datatable(
    row_struct_name="/Game/Inventory/Data/S_ItemDefinition",
    ...
)

# Accessed in Blueprint
GetItemDefinition("HealthPotion") -> S_ItemDefinition
```

### WORKFLOW
1. Create struct with all item properties
2. Properties support display, stacking, and interaction
3. BaseValue included for future Trading module

### MCP TOOL CALL

```python
create_struct(
    struct_name="S_ItemDefinition",
    properties=[
        {"name": "ItemID", "type": "Name", "description": "Unique identifier for the item (matches DataTable row name)"},
        {"name": "DisplayName", "type": "String", "description": "Human-readable item name shown in UI"},
        {"name": "Description", "type": "String", "description": "Item description for tooltips"},
        {"name": "ItemType", "type": "E_ItemType", "description": "Category of the item (Weapon, Armor, etc.)"},
        {"name": "Icon", "type": "Texture2D", "description": "Item icon texture for UI display"},
        {"name": "MaxStackSize", "type": "Integer", "description": "Maximum items per stack (1 = non-stackable)"},
        {"name": "IsDroppable", "type": "Boolean", "description": "Whether item can be dropped into world"},
        {"name": "IsUsable", "type": "Boolean", "description": "Whether item can be used directly (consumables)"},
        {"name": "BaseValue", "type": "Integer", "description": "Base gold value for trading (future module)"}
    ],
    path="/Game/Inventory/Data",
    description="Defines all properties for an inventory item - used as DataTable row type"
)
```

### PROPERTY DETAILS

| Property | Type | Purpose |
|----------|------|---------|
| ItemID | Name | Unique key, matches DataTable row name |
| DisplayName | String | Shown in inventory slot tooltip |
| Description | String | Full description in item details |
| ItemType | E_ItemType | Categorization for filtering |
| Icon | Texture2D | 64x64 icon displayed in slot |
| MaxStackSize | Integer | 1=unique, 10=consumables, 99=materials |
| IsDroppable | Boolean | false for quest items |
| IsUsable | Boolean | true for consumables |
| BaseValue | Integer | Gold worth for trading module |

### VERIFICATION
- [ ] S_ItemDefinition asset created at /Game/Inventory/Data/S_ItemDefinition
- [ ] Struct has exactly 9 properties
- [ ] ItemType property references E_ItemType enum

---

## Step 1.4: Create S_InventorySlot Struct

### AIM
Define how inventory slots store item references and quantities. This struct is used by BP_InventoryManager to track inventory contents.

### USAGE EXAMPLES
```python
# BP_InventoryManager variable
add_blueprint_variable(
    variable_name="InventorySlots",
    variable_type="S_InventorySlot[]",
    ...
)

# Checking if slot is empty
if slot.ItemRowName == None:
    # Slot is empty
```

### WORKFLOW
1. Create struct with 3 properties
2. ItemRowName references DT_Items rows (empty = empty slot)
3. StackCount tracks quantity
4. SlotIndex tracks grid position (0-15)

### MCP TOOL CALL

```python
create_struct(
    struct_name="S_InventorySlot",
    properties=[
        {"name": "ItemRowName", "type": "Name", "description": "Row name from DT_Items DataTable (empty/None = empty slot)"},
        {"name": "StackCount", "type": "Integer", "description": "Number of items in this stack (0 for empty slots)"},
        {"name": "SlotIndex", "type": "Integer", "description": "Position in inventory grid (0-15 for 4x4)"}
    ],
    path="/Game/Inventory/Data",
    description="Represents a single slot in the inventory grid"
)
```

### VERIFICATION
- [ ] S_InventorySlot asset created at /Game/Inventory/Data/S_InventorySlot
- [ ] Struct has exactly 3 properties
- [ ] ItemRowName is type Name (for DataTable row reference)

---

## Step 1.5: Create DT_Items DataTable

### AIM
Create the item database that stores all available item definitions. This is the central data source for the inventory system.

### USAGE EXAMPLES
```python
# Look up item by row name
Get Data Table Row (DT_Items, "HealthPotion") -> S_ItemDefinition

# Iterate all items
Get Data Table Row Names (DT_Items) -> ["IronSword", "LeatherArmor", ...]
```

### WORKFLOW
1. Create DataTable with S_ItemDefinition as row struct
2. DataTable will be populated in Step 1.7

### MCP TOOL CALL

```python
create_datatable(
    datatable_name="DT_Items",
    row_struct_name="/Game/Inventory/Data/S_ItemDefinition",
    path="/Game/Inventory/Data",
    description="Database of all item definitions - the source of truth for item properties"
)
```

### VERIFICATION
- [ ] DT_Items asset created at /Game/Inventory/Data/DT_Items
- [ ] DataTable uses S_ItemDefinition as row structure
- [ ] DataTable is empty (will be populated in 1.7)

---

## Step 1.6: Get DataTable Field Names (CRITICAL)

### AIM
Retrieve the GUID-based field names required for adding rows. This step is MANDATORY before adding any rows to the DataTable.

### USAGE EXAMPLES
```python
# Response contains field_names like:
# "ItemID_0_ABC123...", "DisplayName_2_DEF456...", etc.
```

### WORKFLOW
1. Call get_datatable_row_names on empty DataTable
2. Response includes field_names array
3. Store these names for use in Step 1.7

### MCP TOOL CALL

```python
get_datatable_row_names(datatable_path="/Game/Inventory/Data/DT_Items")
```

### EXPECTED RESPONSE FORMAT
```json
{
    "success": true,
    "result": {
        "row_names": [],
        "field_names": [
            "ItemID_0_<GUID>",
            "DisplayName_2_<GUID>",
            "Description_4_<GUID>",
            "ItemType_6_<GUID>",
            "Icon_8_<GUID>",
            "MaxStackSize_10_<GUID>",
            "IsDroppable_12_<GUID>",
            "IsUsable_14_<GUID>",
            "BaseValue_16_<GUID>"
        ]
    }
}
```

### CRITICAL NOTE
The exact field names will include auto-generated GUIDs. You MUST use the actual returned field names in Step 1.7, not the simplified names shown in this document.

### VERIFICATION
- [ ] Field names retrieved successfully
- [ ] 9 field names returned (matching S_ItemDefinition properties)
- [ ] Field names stored for Step 1.7

---

## Step 1.7: Populate Sample Items

### AIM
Add 5 sample items to the DataTable, one for each item type, to enable testing of all inventory functionality.

### USAGE EXAMPLES
```python
# Items for testing:
# - IronSword: Weapon, non-stackable, droppable
# - LeatherArmor: Armor, non-stackable, droppable
# - HealthPotion: Consumable, stackable (10), usable, droppable
# - IronOre: Material, highly stackable (99), droppable
# - AncientKey: QuestItem, non-stackable, NOT droppable
```

### WORKFLOW
1. Use field names from Step 1.6
2. Create row data for each item
3. Add all rows in single call

### MCP TOOL CALL

```python
# IMPORTANT: Replace <GUID> with actual field names from Step 1.6
add_rows_to_datatable(
    datatable_path="/Game/Inventory/Data/DT_Items",
    rows=[
        {
            "row_name": "IronSword",
            "row_data": {
                "ItemID_0_<GUID>": "IronSword",
                "DisplayName_2_<GUID>": "Iron Sword",
                "Description_4_<GUID>": "A sturdy iron blade, reliable in combat.",
                "ItemType_6_<GUID>": "Weapon",
                "Icon_8_<GUID>": "",
                "MaxStackSize_10_<GUID>": 1,
                "IsDroppable_12_<GUID>": True,
                "IsUsable_14_<GUID>": False,
                "BaseValue_16_<GUID>": 100
            }
        },
        {
            "row_name": "LeatherArmor",
            "row_data": {
                "ItemID_0_<GUID>": "LeatherArmor",
                "DisplayName_2_<GUID>": "Leather Armor",
                "Description_4_<GUID>": "Light armor offering basic protection without hindering movement.",
                "ItemType_6_<GUID>": "Armor",
                "Icon_8_<GUID>": "",
                "MaxStackSize_10_<GUID>": 1,
                "IsDroppable_12_<GUID>": True,
                "IsUsable_14_<GUID>": False,
                "BaseValue_16_<GUID>": 75
            }
        },
        {
            "row_name": "HealthPotion",
            "row_data": {
                "ItemID_0_<GUID>": "HealthPotion",
                "DisplayName_2_<GUID>": "Health Potion",
                "Description_4_<GUID>": "A red potion that restores 50 health when consumed.",
                "ItemType_6_<GUID>": "Consumable",
                "Icon_8_<GUID>": "",
                "MaxStackSize_10_<GUID>": 10,
                "IsDroppable_12_<GUID>": True,
                "IsUsable_14_<GUID>": True,
                "BaseValue_16_<GUID>": 25
            }
        },
        {
            "row_name": "IronOre",
            "row_data": {
                "ItemID_0_<GUID>": "IronOre",
                "DisplayName_2_<GUID>": "Iron Ore",
                "Description_4_<GUID>": "Raw iron ore that can be smelted into iron ingots.",
                "ItemType_6_<GUID>": "Material",
                "Icon_8_<GUID>": "",
                "MaxStackSize_10_<GUID>": 99,
                "IsDroppable_12_<GUID>": True,
                "IsUsable_14_<GUID>": False,
                "BaseValue_16_<GUID>": 10
            }
        },
        {
            "row_name": "AncientKey",
            "row_data": {
                "ItemID_0_<GUID>": "AncientKey",
                "DisplayName_2_<GUID>": "Ancient Key",
                "Description_4_<GUID>": "A mysterious key that opens the door to the ancient ruins. Do not lose it.",
                "ItemType_6_<GUID>": "QuestItem",
                "Icon_8_<GUID>": "",
                "MaxStackSize_10_<GUID>": 1,
                "IsDroppable_12_<GUID>": False,
                "IsUsable_14_<GUID>": False,
                "BaseValue_16_<GUID>": 0
            }
        }
    ]
)
```

### SAMPLE ITEMS SUMMARY

| Row Name | Type | Stack | Droppable | Usable | Value |
|----------|------|-------|-----------|--------|-------|
| IronSword | Weapon | 1 | Yes | No | 100 |
| LeatherArmor | Armor | 1 | Yes | No | 75 |
| HealthPotion | Consumable | 10 | Yes | Yes | 25 |
| IronOre | Material | 99 | Yes | No | 10 |
| AncientKey | QuestItem | 1 | No | No | 0 |

### VERIFICATION
- [ ] 5 rows added to DT_Items
- [ ] All item types represented
- [ ] HealthPotion has IsUsable=true
- [ ] AncientKey has IsDroppable=false
- [ ] Stack sizes vary appropriately

---

## Phase 1 Completion Checklist

### Assets Created
- [ ] /Game/Inventory/ folder structure
- [ ] E_ItemType enum (5 values)
- [ ] S_ItemDefinition struct (9 properties)
- [ ] S_InventorySlot struct (3 properties)
- [ ] DT_Items DataTable (5 rows)

### All Done Via MCP Tools
- [ ] `create_folder` used for folder structure
- [ ] `create_enum` used for E_ItemType
- [ ] `create_struct` used for both structs
- [ ] `create_datatable` used for DT_Items
- [ ] `get_datatable_row_names` used to get field names
- [ ] `add_rows_to_datatable` used to populate items

### Ready for Phase 2
- [ ] Data layer complete and verified
- [ ] Proceed to Phase 2: Blueprint Logic Layer

---

## Next Phase

Continue to [Phase2_BlueprintLogic.md](Phase2_BlueprintLogic.md) to create the BP_InventoryManager component with all inventory functions.
