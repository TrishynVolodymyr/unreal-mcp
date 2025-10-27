# Unreal MCP DataTable Tools

This document provides detailed information about the DataTable tools available in the Unreal MCP integration.

## Overview

DataTable tools allow you to create and manipulate DataTable assets in Unreal Engine, including creating DataTables with custom struct types, managing rows and data, performing CRUD operations, and handling structured game data.

## DataTable Tools

### create_datatable

Create a new DataTable asset.

**Parameters:**
- `datatable_name` (string) - Name of the DataTable to create
- `row_struct_name` (string) - Name or path of the struct to use for rows. Can be:
  - Full path (recommended): "/Game/Path/To/MyStruct"
  - Simple name: "MyStruct" (will search in multiple locations)
- `path` (string, optional) - Path where to create the DataTable, defaults to "/Game/Data"
- `description` (string, optional) - Optional description for the DataTable

**Returns:**
- Dict containing success status and created asset info

**Example:**
```json
{
  "command": "create_datatable",
  "params": {
    "datatable_name": "ItemTable",
    "row_struct_name": "/Game/Data/ItemStruct",
    "path": "/Game/Data",
    "description": "Table containing all game items"
  }
}
```

### get_datatable_rows

Get rows from a DataTable.

**Parameters:**
- `datatable_name` (string) - Name of the target DataTable
- `row_names` (array, optional) - Optional list of specific row names to retrieve

**Returns:**
- Dict containing the requested rows

**Example:**
```json
{
  "command": "get_datatable_rows",
  "params": {
    "datatable_name": "ItemTable",
    "row_names": ["Sword", "Shield", "Potion"]
  }
}
```

### delete_datatable_row

Delete a row from a DataTable.

**Parameters:**
- `datatable_name` (string) - Name of the target DataTable
- `row_name` (string) - Name of the row to delete

**Returns:**
- Dict containing success status and updated DataTable info

**Example:**
```json
{
  "command": "delete_datatable_row",
  "params": {
    "datatable_name": "ItemTable",
    "row_name": "OldSword"
  }
}
```

### get_datatable_row_names

Get all row names and struct field names from a DataTable.

**Parameters:**
- `datatable_name` (string) **REQUIRED** - Name of the target DataTable

**Returns:**
- Dict containing:
  - `row_names` (array) - List of row names (string)
  - `field_names` (array) - List of struct field names with GUID suffixes (string)

**⚠️ CRITICAL: GUID-based Field Names**
Field names returned include auto-generated GUID suffixes that MUST be used for add/update operations:
- Original field: "ItemName" 
- Actual field: "ItemName_2_6E790E8C43602AE89B031BA4C77F112E"

**Example:**
```json
{
  "command": "get_datatable_row_names",
  "params": {
    "datatable_name": "ItemTable"
  }
}

// Response example:
{
  "result": {
    "row_names": ["Sword", "Potion"],
    "field_names": [
      "ItemName_2_6E790E8C43602AE89B031BA4C77F112E",
      "ItemType_4_B4EA87F9426CC9C18DDC389BB5346008",
      "Value_6_2B59520A4965FAC496E07197E22FB659"
    ]
  }
}
```

### add_rows_to_datatable

Add multiple rows to an existing DataTable.

**Parameters:**
- `datatable_name` (string) **REQUIRED** - Name of the target DataTable. Can be:
  - Full path (recommended): "/Game/Data/MyTable"
  - Simple name: "MyTable" (will search in multiple locations)
- `rows` (array) **REQUIRED** - List of dicts, each with:
  - `row_name` (string) - Unique name for the row
  - `row_data` (object) - Dict of property values using GUID-based property names from get_datatable_row_names

**Returns:**
- Dict containing success status and list of added row names

**⚠️ CRITICAL WORKFLOW:**
1. **ALWAYS call get_datatable_row_names FIRST** to get correct field names with GUIDs
2. Use the exact field names returned (with GUID suffixes) in row_data
3. Field names are case-sensitive and must match exactly

**Example (Real workflow from testing):**
```json
// Step 1: Get field names with GUIDs
{
  "command": "get_datatable_row_names",
  "params": {
    "datatable_name": "ItemsDataTable"
  }
}

// Response shows actual field names:
// "ItemName_2_6E790E8C43602AE89B031BA4C77F112E"
// "ItemType_4_B4EA87F9426CC9C18DDC389BB5346008"
// "Value_6_2B59520A4965FAC496E07197E22FB659"
// etc.

// Step 2: Use exact field names in add operation
{
  "command": "add_rows_to_datatable",
  "params": {
    "datatable_name": "ItemsDataTable",
    "rows": [
      {
        "row_name": "Sword",
        "row_data": {
          "ItemName_2_6E790E8C43602AE89B031BA4C77F112E": "Iron Sword",
          "ItemType_4_B4EA87F9426CC9C18DDC389BB5346008": "Weapon",
          "Value_6_2B59520A4965FAC496E07197E22FB659": 150,
          "Weight_8_E1CD64B54B8F6B4D51677CACB3C96B13": 2.5,
          "IsStackable_10_1483BFC542416DBC2E57269B9B5520B6": false,
          "MaxStackSize_12_1AA05FCC4E410B0F6F0D5283F011F132": 1,
          "Description_14_34CA8ACC453C90C90665BB9C963A5E9E": "A sturdy iron sword for combat"
        }
      }
    ]
  }
}
```

### update_rows_in_datatable

Update multiple rows in an existing DataTable.

**Parameters:**
- `datatable_name` (string) - Name of the target DataTable. Can be:
  - Full path (recommended): "/Game/Data/MyTable"
  - Simple name: "MyTable" (will search in multiple locations)
- `rows` (array) - List of dicts, each with:
  - `row_name` (string) - Name of the row to update
  - `row_data` (object) - Dict of property values using the internal GUID-based property names

**Returns:**
- Dict containing success status and list of updated/failed row names

**Example:**
```json
{
  "command": "update_rows_in_datatable",
  "params": {
    "datatable_name": "/Game/Data/ItemTable",
    "rows": [
      {
        "row_name": "MagicSword",
        "row_data": {
          "ItemName_12345678": "Enhanced Magic Sword",
          "Price_87654321": 399.99,
          "Quantity_11111111": 2,
          "IsAvailable_22222222": true,
          "Tags_33333333": ["weapon", "magic", "enhanced"]
        }
      }
    ]
  }
}
```

### delete_datatable_rows

Delete multiple rows from a DataTable.

**Parameters:**
- `datatable_name` (string) - Name of the target DataTable
- `row_names` (array) - List of row names to delete

**Returns:**
- Dict containing success status and updated DataTable info

**Example:**
```json
{
  "command": "delete_datatable_rows",
  "params": {
    "datatable_name": "ItemTable",
    "row_names": ["OldSword", "BrokenShield", "ExpiredPotion"]
  }
}
```

## Common Usage Patterns

### DataTable Creation Workflow

1. **Create Struct**: First create a struct definition using Project Tools
2. **Create DataTable**: Use `create_datatable` with the struct path
3. **Get Field Names**: Use `get_datatable_row_names` to get GUID-based field names
4. **Add Data**: Use `add_rows_to_datatable` with the correct field names
5. **Manage Data**: Use update and delete operations as needed

### Working with GUID-based Field Names

DataTable field names include auto-generated GUIDs that must be used for data operations:

```json
// Step 1: Get the correct field names
{
  "command": "get_datatable_row_names",
  "params": {
    "datatable_name": "ItemTable"
  }
}

// Response includes field_names like:
// ["ItemName_12345678", "Price_87654321", "Quantity_11111111", ...]

// Step 2: Use these exact field names in add/update operations
{
  "command": "add_rows_to_datatable",
  "params": {
    "datatable_name": "ItemTable",
    "rows": [
      {
        "row_name": "NewItem",
        "row_data": {
          "ItemName_12345678": "New Item Name",
          "Price_87654321": 99.99
        }
      }
    ]
  }
}
```

### Struct Path Resolution

The system searches for structs in the following order:
1. Direct path if starting with "/Game/" or "/Script/"
2. Engine and core paths
3. Game content paths:
   - /Game/Blueprints/
   - /Game/Data/
   - /Game/ (root)

**Recommended**: Always use full paths for predictable behavior:
```json
{
  "row_struct_name": "/Game/Data/Structs/ItemStruct"
}
```

### Data Type Support

DataTables support various data types in struct fields:
- **Basic Types**: Boolean, Integer, Float, String
- **Unreal Types**: Vector, Rotator, Transform, Color
- **Arrays**: Any type can be made into an array
- **Objects**: References to other assets
- **Nested Structs**: Structs within structs

### Batch Operations

For efficiency, use batch operations when possible:
```json
// Add multiple rows at once
{
  "command": "add_rows_to_datatable",
  "params": {
    "datatable_name": "ItemTable",
    "rows": [
      {"row_name": "Item1", "row_data": {...}},
      {"row_name": "Item2", "row_data": {...}},
      {"row_name": "Item3", "row_data": {...}}
    ]
  }
}

// Delete multiple rows at once
{
  "command": "delete_datatable_rows",
  "params": {
    "datatable_name": "ItemTable",
    "row_names": ["Item1", "Item2", "Item3"]
  }
}
```

### Error Handling

Always check return values for success status:
```json
// Typical successful response
{
  "success": true,
  "message": "Operation completed successfully",
  "added_rows": ["Item1", "Item2"],
  "failed_rows": []
}

// Typical error response
{
  "success": false,
  "message": "DataTable not found: ItemTable",
  "error": "Asset path resolution failed"
}
```

### Integration with Blueprint Tools

DataTables work well with Blueprint systems:
1. **Blueprint Variables**: Set DataTable references in Blueprint variables
2. **Blueprint Functions**: Use "Get Data Table Row" nodes in Blueprints
3. **Data Binding**: Bind DataTable data to UMG widgets
4. **Runtime Access**: Read DataTable data during gameplay

### Performance Considerations

- **Batch Operations**: Use add_rows_to_datatable with multiple rows instead of single-row operations
- **Selective Retrieval**: Use row_names parameter in get_datatable_rows to fetch only needed data
- **Field Name Caching**: Cache field names from get_datatable_row_names to avoid repeated calls
- **Asset References**: Use full paths to avoid search overhead 
##
 Troubleshooting and Error Handling

### Common Errors and Solutions

**Error: "Invalid parameters for command 'get_datatable_rows'"**
- **Cause**: DataTable name not found or incorrect path
- **Solution**: Use full path like "/Game/Data/ItemsDataTable" or verify DataTable exists

**Error: "DataTable not found"**
- **Cause**: Incorrect DataTable name or path
- **Solution**: Check exact spelling and use full asset path

**Error: "Field not found in struct"**
- **Cause**: Using original field names instead of GUID-based names
- **Solution**: Always call get_datatable_row_names first and use the returned field names exactly

**Error: "Row already exists"**
- **Cause**: Trying to add a row with a name that already exists
- **Solution**: Use update_rows_in_datatable instead, or choose a different row_name

**Error: "Struct type mismatch"**
- **Cause**: DataTable struct doesn't match the provided data structure
- **Solution**: Verify struct definition and ensure all required fields are provided

### Best Practices

1. **Always get field names first**: Call get_datatable_row_names before any add/update operations
2. **Use full paths**: Specify complete asset paths to avoid resolution issues
3. **Batch operations**: Group multiple row operations together for better performance
4. **Validate data types**: Ensure data types match the struct definition
5. **Handle errors gracefully**: Check success status in responses and handle failures appropriately

### Debugging Workflow

1. **Verify DataTable exists**: Check asset browser or use project tools to list assets
2. **Get field structure**: Use get_datatable_row_names to see current structure
3. **Test with single row**: Add one row first to verify field names and data types
4. **Check response messages**: Read error messages carefully for specific issues
5. **Use full paths**: Always specify complete asset paths for reliability

### Field Name Management

The GUID-based field naming system ensures uniqueness but requires careful handling:

```json
// WRONG: Using original field names
{
  "row_data": {
    "ItemName": "Sword",
    "Value": 100
  }
}

// CORRECT: Using GUID-based field names from get_datatable_row_names
{
  "row_data": {
    "ItemName_2_6E790E8C43602AE89B031BA4C77F112E": "Sword",
    "Value_6_2B59520A4965FAC496E07197E22FB659": 100
  }
}
```

**Pro Tip**: Store the field names mapping in your workflow to avoid repeated get_datatable_row_names calls:

```json
// Get field names once
const fieldNames = await get_datatable_row_names("ItemsDataTable");
const nameField = fieldNames.field_names[0]; // "ItemName_2_6E790E8C43602AE89B031BA4C77F112E"
const valueField = fieldNames.field_names[2]; // "Value_6_2B59520A4965FAC496E07197E22FB659"

// Use stored field names for multiple operations
const rowData = {
  [nameField]: "New Item",
  [valueField]: 150
};
```