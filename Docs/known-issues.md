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

### Graph Nodes Metadata Response Size
- **Affected**: `get_blueprint_metadata` with `fields=["graph_nodes"]`
- **Status**: Mitigated with ultra-compact format
- **New format**:
  ```json
  {
    "id": "ABC123",
    "title": "Branch",
    "pins": {
      "Condition": ["DEF456|Get IsInventoryOpen|ReturnValue"],
      "True": ["GHI789|Remove from Parent|execute"],
      "False": ["JKL012|Create Widget|execute"],
      "InputPin": "default_value_here"
    }
  }
  ```
- **Format details**:
  - Each node has: `id`, `title`, `pins` (object)
  - Connected pins: `"pinName": ["nodeId|nodeTitle|targetPin", ...]`
  - Unconnected input pins with defaults: `"pinName": "defaultValue"`
  - Unconnected pins without defaults: omitted entirely
  - Removed: `type`, `x`, `y`, `dir`, pin type info
- **Orphaned nodes format**: `id`, `title`, `graph` only (no type, x, y)
- **Best practices**:
  - Design smaller, focused functions (15-20 nodes max)
  - Use `node_type` filter to reduce response size
  - Always specify `graph_name` parameter to limit scope

### No MCP Tool to Remove Widget Components
- **Affected**: `mcp__umgMCP` tools
- **Problem**: While we can add components to UMG Widget Blueprints using `add_widget_component_to_widget`, there is no corresponding tool to remove/delete widget components.
- **Example**: After refactoring WBP_InventoryGrid from ScrollBox+VerticalBox to UniformGridPanel, the old components (SlotsScrollBox, SlotsContainer) remain in the widget.
- **Impact**: Old unused components clutter the widget hierarchy but don't affect functionality if not referenced in logic.
- **Workaround**: Leave unused components in place (they won't affect runtime if not connected), or manually remove them in Unreal Editor.
- **Future Fix**: Implement `remove_widget_component` tool in umgMCP.

### Cast Nodes Require Execution Pin Connections (CRITICAL - RECURRING ISSUE)
- **Affected**: `K2Node_DynamicCast` nodes created via `create_node_by_action_name`
- **Problem**: Cast nodes (e.g., "Cast To PlayerController", "Cast To Widget") have BOTH execution pins AND data pins. Simply connecting data pins (Object input, cast output) is NOT sufficient - the execution pins MUST also be connected for the cast to actually execute.
- **Root Cause of Recurring Mistakes**: Unlike most programming languages where casts are pure expressions, Unreal Blueprint Cast nodes are **IMPURE** - they require being in the execution chain to run.
- **Symptoms**:
  - Blueprint compiles with warnings: "Cast To X was pruned because its Exec pin is not connected"
  - Cast output pin returns null at runtime even though input is valid
  - Downstream nodes that depend on cast result fail silently
- **MANDATORY WORKFLOW when using Cast nodes**:
  1. Create the Cast node
  2. Connect BOTH execution pins (execute input AND then output) into the execution chain
  3. THEN connect data pins (Object input, cast result output)
  4. If you need the cast result but don't want execution flow to go through it - YOU CANNOT USE A CAST NODE. Consider alternatives:
     - Store the already-typed reference in a variable
     - Use interface calls instead of casting
     - Restructure logic so cast is in the main execution path
- **Example**:
  ```
  WRONG:  GetController --[Return Value]--> Cast To PlayerController --[As PlayerController]--> SetInputMode
  RIGHT:  GetController --[exec]--> Cast To PlayerController --[exec]--> SetInputMode
                        --[Return Value]-->                  --[As PlayerController]-->
  ```
- **Workaround**: Always connect BOTH execution flow AND data flow through cast nodes. The cast node must be in the execution chain, not just receiving data.

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

- **Node ID Returns All-Zeros for Certain Node Types** - Fixed in GraphUtils.cpp
  - Issue: Multiple node types returned `node_id: "00000000000000000000000000000000"` in metadata, including K2Node_FunctionEntry, K2Node_FunctionResult, and K2Node_DynamicCast. This made it impossible to reliably target these nodes for deletion/disconnection.
  - Root Cause: Some Unreal node types have uninitialized `NodeGuid` properties where all GUID components are zero. `FGuid::IsValid()` returns false for these.
  - Fix: Created `FGraphUtils::GetReliableNodeId()` function that checks if `NodeGuid.IsValid()` and falls back to generating a unique ID from the object's `GetUniqueID()` when the GUID is invalid. Updated 11 files across the codebase to use this function.
  - Generated IDs use format: `OBJID_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX` (32 hex chars derived from object's unique ID)

## Notes

- When working on MCP improvements, check this file first
- Add new issues as they are discovered during development
