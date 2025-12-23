# Known Issues and Future Improvements

This file tracks MCP tool limitations discovered during development. When encountering issues, document them here.

## Active Issues

### Struct Field GUID Middle Numbers Can Change When Structs Are Modified
- **Affected**: `get_struct_pin_names`, `get_datatable_row_names`, DataTable operations
- **Problem**: User-defined structs use GUID-based internal field names (e.g., `TestField1_2_1EAE0B8A4B971533B2AD21BF45BA9220`). The hex suffix (GUID) remains stable, but the middle number can change when the struct is modified (type changes, field additions/removals).
- **Example**: After modifying `S_TestGUID` struct:
  - Before: `TestField1_2_1EAE0B8A4B971533B2AD21BF45BA9220`
  - After:  `TestField1_5_1EAE0B8A4B971533B2AD21BF45BA9220`
  - Note: `_2_` changed to `_5_`, but hex GUID suffix stayed same
- **Impact**: Cached field names may become stale after struct modifications
- **Workaround**: Always fetch fresh pin names using `get_struct_pin_names` or `get_datatable_row_names` before any struct field operations. Never hardcode GUID-based field names.

### Pure Functions Cannot Use Loop Accumulation Pattern
- **Affected**: Blueprint functions marked as `is_pure=True` that need to accumulate values in loops
- **Problem**: Pure functions in Blueprint cannot have execution flow (no exec pins), but For Each Loop macro requires execution pins for the loop body. This makes accumulating values (like summing StackCount across matching slots) impossible in a pure function.
- **Example**: `GetTotalItemCount` was designed as pure but needs to sum values across loop iterations
- **Workaround**: Make such functions impure (`is_pure=False`) instead. The function will still work correctly, just won't be callable from pure contexts.

---

## Resolved Issues

Previously tracked issues have been resolved:

- **No MCP Tool to Modify Function Properties** - Fixed in ModifyBlueprintFunctionPropertiesCommand.cpp
  - Issue: Once a function was created with `create_custom_blueprint_function`, there was no way to change its properties (pure/impure, const, access specifier, etc.)
  - Fix: Added new `modify_blueprint_function_properties` tool that can change is_pure, is_const, access_specifier, and category on existing functions.

- **Blueprint Variable Default Values Not Exposed** - Fixed in GetBlueprintMetadataCommand.cpp
  - Issue: When querying Blueprint metadata, variable default values were not included.
  - Fix: Added CDO (Class Default Object) access to extract default values for all variable types.

- **Struct Field Pin Names Hard to Discover** - Fixed in GetStructPinNamesCommand.cpp
  - Issue: User-defined structs use GUID-based internal field names (e.g., `ItemName_F2A4BC92`) that were difficult to discover.
  - Fix: Added `get_struct_pin_names` tool that returns both GUID-based pin names and friendly display names.

- **Poor Error Messages When Function Not Found** - Fixed in BlueprintActionDatabaseNodeCreator.cpp
  - Issue: When `create_node_by_action_name` couldn't find a function, it returned a generic "not found" error.
  - Fix: Now searches for similar function names and includes up to 5 suggestions in the error message.

- **Custom Struct Output Types in Blueprint Functions** - Fixed in BlueprintPropertyService.cpp
  - Issue: When creating a Blueprint function with a custom struct as an output type (e.g., `/Game/Inventory/Data/S_ItemDefinition`), the output pin was created as `Float` instead of the specified struct type.
  - Fix: Added support for loading `UUserDefinedStruct` from full asset paths in `ResolveVariableType()` function.

- **Array Length Helper** - Fixed in BlueprintActionDatabaseNodeCreator.cpp
- **TextBlock Font Size** - Fixed in UMGService.cpp (use `font_size` kwarg)
- **Orphaned Nodes Detection** - Implemented in metadata commands (use `fields=["orphaned_nodes"]`)
- **Node Filtering in Metadata** - Fixed in GetBlueprintMetadataCommand.cpp
  - Issue: The `graph_nodes` field in `get_blueprint_metadata` lacked filtering capabilities that `find_blueprint_nodes` had.
  - Fix: Added optional `node_type`, `event_type`, and `graph_name` filter parameters to `get_blueprint_metadata`.
  - Usage: `get_blueprint_metadata(blueprint_name="BP_MyActor", fields=["graph_nodes"], node_type="Event", event_type="BeginPlay")`
  - Note: The `find_blueprint_nodes` tool has been removed. Use `get_blueprint_metadata` with filters instead.

## Notes

- When working on MCP improvements, check this file first
- Add new issues as they are discovered during development
