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

### Material ScalarParameter DefaultValue Not Applied
- **Affected**: `add_material_expression` with `expression_type="ScalarParameter"` and `properties={"DefaultValue": X}`
- **Problem**: The `DefaultValue` property for ScalarParameter expressions is not being applied when specified in the properties dict.
- **Example**: Creating ScalarParameter with `{"ParameterName": "Intensity", "DefaultValue": 5}` results in DefaultValue showing 0.0 in the editor.
- **Workaround**: After creating the expression, use `set_material_expression_property` to set the DefaultValue separately.
- **Root Cause**: Property name case sensitivity - the C++ code checks for `default_value` (lowercase) but Python sends `DefaultValue` (camelCase).
- **Status**: FIXED in MaterialExpressionService.cpp - now accepts both camelCase and lowercase property names.

---

## Resolved Issues

- **Material Expression Connections Return Success But Don't Persist** - Fixed in MaterialExpressionService.cpp
  - Issue: `connect_material_expressions` returned `success: true` but connections were lost after reopening the material.
  - Root Cause: Code was using graph-level connections (`MakeLinkTo()`, `TryCreateConnection()`) and syncing in wrong direction (`LinkMaterialExpressionsFromGraph()`). MaterialGraph is transient - rebuilt from expressions on load.
  - Fix: Use UE5's `ConnectExpression()` method and sync graph FROM expressions:
    ```cpp
    SourceExpr->ConnectExpression(TargetInput, Params.SourceOutputIndex);
    Material->MaterialGraph->LinkGraphNodesFromMaterial();  // Expressions → Graph
    ```
  - Key insight: `LinkGraphNodesFromMaterial()` = Expressions → Graph (CORRECT). `LinkMaterialExpressionsFromGraph()` = Graph → Expressions (UNRELIABLE for persistence).

## Notes

- When working on MCP improvements, check this file first
- Add new issues as they are discovered during development
