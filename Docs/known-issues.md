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

---

### NiagaraMCP add_renderer Creates Duplicate When Template Has Inherited Renderer
- **Affected**: `add_renderer` in niagaraMCP when emitter uses a template
- **Problem**: When creating emitters from templates (which most do), the template already includes an inherited renderer (shown with lock icon in UI). Calling `add_renderer` then adds a SECOND renderer, causing duplicate rendering.
- **Symptoms**:
  - Each emitter shows 2 renderers: one inherited (locked), one added (unlocked)
  - Visual artifacts - solid masses instead of proper particles
  - One renderer may use correct material, other uses default/none
  - Double particle counts, performance impact
- **Example**: Creating fire emitter from template, then calling `add_renderer("Sprite")` results in:
  - `Sprite Renderer` (locked, inherited from template) - uses template's default material
  - `SpriteRenderer` (unlocked, MCP-added) - uses material we specified
- **Root Cause**: MCP workflow doesn't check for existing inherited renderers before adding new ones
- **Mitigation (Partial)**: `add_emitter_to_system` now returns `existing_renderers` array with info about inherited renderers, plus a `note` field advising to use `set_renderer_property` instead of `add_renderer`. Example response:
  ```json
  {
    "success": true,
    "emitter_handle_id": "...",
    "emitter_name": "FireCore",
    "existing_renderers": [{"name": "Renderer", "type": "Sprite", "enabled": true}],
    "note": "Emitter has 1 existing renderer(s). Use set_renderer_property to configure them instead of adding new ones."
  }
  ```
- **AI Workflow**: After `add_emitter_to_system`, check `existing_renderers`. If non-empty, use `set_renderer_property` on the inherited renderer (usually named "Renderer") instead of calling `add_renderer`.
- **Workaround for existing systems with duplicates**:
  1. Set material on the inherited "Renderer" using `set_renderer_property`
  2. Manually delete the duplicate renderer in Unreal Editor (MCP cannot delete inherited renderers)

---

### NiagaraMCP Module Search Returns Empty Results (PARTIALLY RESOLVED)
- **Affected**: `search_niagara_modules` in niagaraMCP
- **Problem**: Searching for some Niagara modules returns 0 results
- **Update (2026-01-06)**: Search DOES work for some queries:
  - `search_niagara_modules("Drag")` → 4 results ✅
  - `search_niagara_modules("Scale")` → 28 results ✅
- **Still failing**:
  - `search_niagara_modules("Scale Sprite")` → 0 results (but "Scale" alone works)
  - `search_niagara_modules("Curl Noise")` → 0 results
- **Workaround**: Use single-word searches, browse results to find exact module name

---

### NiagaraMCP LinearColor Type Cannot Be Set via set_module_input
- **Affected**: `set_module_input` in niagaraMCP when `input_type` is `LinearColor`
- **Date Discovered**: 2026-01-06
- **Problem**: Setting `LinearColor` type parameters returns success but value doesn't change
- **Symptoms**: Tool returns `{"success": true}` but parameter value remains unchanged
- **Tested Scenarios**:

  **Attempt 1 - Color module (Update stage)**:
  ```json
  {
    "system_path": "/Game/VFX/NS_RealisticFire",
    "emitter_name": "FireEmbers",
    "module_name": "Color",
    "stage": "Update",
    "input_name": "Color",
    "value": "(R=1.0, G=0.7, B=0.2, A=1.0)"
  }
  ```
  - **Response**: `{"success": true, "message": "Module input set successfully"}`
  - **Verification**: Value remained `(R=0.0000, G=0.0000, B=0.0000, A=0.0000)`

  **Attempt 2 - InitializeParticle module (Spawn stage)**:
  ```json
  {
    "system_path": "/Game/VFX/NS_RealisticFire",
    "emitter_name": "FireEmbers",
    "module_name": "InitializeParticle",
    "stage": "Spawn",
    "input_name": "Color",
    "value": "(R=1.0, G=0.7, B=0.2, A=1.0)"
  }
  ```
  - **Response**: `{"success": true, "message": "Module input set successfully"}`
  - **Verification**: Value remained `(R=0.0000, G=0.0000, B=0.0000, A=0.0000)`

- **Types That DO Work** (same session, same emitter):
  | Input | Type | Value | Result |
  |-------|------|-------|--------|
  | Lifetime Min | NiagaraFloat | 2.0 | ✅ Updated |
  | Lifetime Max | NiagaraFloat | 4.0 | ✅ Updated |
  | Velocity | Vector3f | (0,0,200) | ✅ Updated |
  | Cone Angle | NiagaraFloat | 70 | ✅ Updated |
  | Gravity | Vector3f | (0,0,-20) | ✅ Updated |
  | Noise Frequency | NiagaraFloat | 0.7 | ✅ Updated |
  | Scale Color | Vector3f | (4,3,1) | ✅ Updated |
  | Scale Alpha | NiagaraFloat | 1.0 | ✅ Updated |
  | Drag | NiagaraFloat | 0.8 | ✅ Updated |

- **Root Cause Hypothesis**:
  1. LinearColor parsing not implemented in C++ `set_module_input` handler
  2. Format mismatch - may need different format than `(R=x, G=y, B=z, A=w)`
  3. Niagara Color values may be bound to curves/dynamic inputs that override direct values
- **Workaround**: Set LinearColor values manually in Unreal Editor
- **Future Fix**: Investigate NiagaraService C++ code to add LinearColor type handling

---


### NiagaraMCP Cannot Set Curve/Gradient Values Over Particle Lifetime (MOSTLY RESOLVED)
- **Affected**: `set_module_input` in niagaraMCP
- **Date Discovered**: 2026-01-08
- **Status**: `set_module_curve_input` and `set_module_color_curve_input` commands implemented and working
- **Problem**: Cannot set values that change over particle lifetime - the most fundamental feature of particle effects
- **Impact**: Cannot create realistic VFX because particles cannot:
  - Fade out (alpha curve)
  - Change color (yellow → orange → red as ember cools)
  - Shrink over life (size curve)
  - Any property that varies with NormalizedAge
- **Technical Details**: Niagara module inputs often expect "Dynamic Inputs" such as:
  - `Curve over Normalized Age` - requires FRichCurve data
  - `Scale Color by Curve` - binds to curve asset or inline curve
  - `Float from Curve` - samples curve based on particle age
- **Current Limitation**: `set_module_input` can only set static scalar/vector values, not curve bindings or dynamic inputs
- **Implementation Progress**:
  1. ✅ Added `set_module_curve_input` command with keyframe data - WORKING
  2. ✅ Added `set_module_color_curve_input` command with RGBA keyframes - FIXED (2026-01-09)
     - Now uses Dynamic Inputs (e.g., "Scale Linear Color by Curve") for LinearColor inputs
     - Direct ColorCurve DI assignment still works for explicit curve inputs
- **Priority**: LOW - Core functionality now works

---

### NiagaraMCP Cannot Set Random Range Values
- **Affected**: `set_module_input` in niagaraMCP
- **Date Discovered**: 2026-01-08
- **Problem**: Cannot set randomized inputs - particles all have identical values
- **Impact**:
  - All particles same size (should vary)
  - All particles same lifetime (should vary)
  - All particles same velocity (should vary)
  - Results in artificial, uniform-looking effects
- **Technical Details**: Niagara expects dynamic inputs like:
  - `Uniform Random Float(Min, Max)`
  - `Uniform Random Vector(Min, Max)`
  - `Random Float from Distribution`
- **Current Limitation**: Can only set single static value, not random range
- **Required Implementation**:
  1. Add `set_module_random_input` command
  2. Accept min/max values: `{"min": 1, "max": 5}`
  3. Create appropriate Niagara dynamic input binding
- **Priority**: HIGH - Essential for natural-looking variation

---

### NiagaraMCP Module Input Type Discovery Missing
- **Affected**: `set_module_input` workflow
- **Date Discovered**: 2026-01-08
- **Problem**: Cannot determine what TYPE an input expects before setting it
- **Impact**:
  - Don't know if input expects float, vector, curve, or dynamic input
  - Trial and error required to find correct format
  - Errors like "Could not parse value '3' for input type 'Vector2f'"
- **Current Behavior**: Only see input name, not type information
- **Required Implementation**:
  1. Add `get_module_input_types` command
  2. Return: `{"input_name": "SpawnRate", "type": "NiagaraFloat", "accepts_curve": true, "accepts_random": true}`
- **Priority**: MEDIUM - Would improve workflow significantly

---

### NiagaraMCP add_emitter_to_system create_if_missing Returns Unknown Error
- **Affected**: `add_emitter_to_system` with `create_if_missing=True`
- **Date Discovered**: 2026-01-08
- **Problem**: When adding a non-existent emitter to a newly created system with `create_if_missing=True`, the emitter creation step fails with "Unknown error"
- **Example Request**:
  ```json
  {
    "system": "/Game/Testing/NS_CurveTest",
    "emitter": "NE_CurveTestEmitter",
    "create_if_missing": true,
    "emitter_folder": "/Game/Testing",
    "emitter_type": "Sprite"
  }
  ```
- **Response**:
  ```json
  {
    "success": false,
    "error": "Failed to create emitter: Unknown error",
    "original_add_error": "emitter not found: ne_curvetestemitter"
  }
  ```
- **Root Cause**: Python-side `create_if_missing` logic calls `create_niagara_emitter` which returns "Unknown error" - the C++ error message is not being properly propagated
- **Workaround**: Create emitter separately first with `create_niagara_emitter`, then use `add_emitter_to_system`
- **Required Fix**: Improve error handling in both:
  1. C++ `create_niagara_emitter` - return specific error messages
  2. Python `add_emitter_to_system` - better error propagation from create step
- **Priority**: MEDIUM - Workaround exists

---

### MaterialMCP RadialGradientExponential Expression Not Supported
- **Affected**: `add_material_expression` in materialMCP
- **Date Discovered**: 2026-01-08
- **Problem**: Expression type "RadialGradientExponential" not recognized
- **Error**: `{"status": "error", "error": "Unknown expression type: RadialGradientExponential"}`
- **Impact**: Cannot create sharp center-to-edge falloff for particle materials
- **Current Workaround**: Use Distance + OneMinus + Power chain (less convenient)
- **Required Implementation**: Add `UMaterialExpressionRadialGradientExponential` to expression type map
- **Include Path**: `Materials/MaterialExpressionRadialGradientExponential.h`
- **Priority**: LOW - Workaround exists

---

### NiagaraMCP compile_niagara_asset Returns Warnings as Errors
- **Affected**: `compile_niagara_asset` in niagaraMCP
- **Date Discovered**: 2026-01-06
- **Problem**: Compile returns `status: "error"` for non-fatal warnings
- **Example Request**:
  ```json
  {"asset_path": "/Game/VFX/NS_RealisticFire"}
  ```
- **Response**:
  ```json
  {
    "status": "error",
    "error": "[FireEmbers] Spawn Script: Default found for InitializeParticle.Material Random, but not found in ParameterMap traversal - Node: Map Get - ..."
  }
  ```
- **Actual Behavior**: System compiles and works fine - these are warnings about default values
- **Impact**: AI incorrectly interprets successful compiles as failures
- **Suggested Fix**: Distinguish warnings from errors:
  ```json
  {
    "status": "success",
    "warnings": ["..."],
    "errors": []
  }
  ```

---

## Resolved Issues

- **NiagaraMCP set_module_color_curve_input Cannot Set Curves on LinearColor Inputs** - Fixed 2026-01-09
  - Issue: ColorCurve data interfaces cannot be directly assigned to LinearColor inputs (type incompatibility). This caused crashes when viewing the module in Niagara Editor.
  - Root Cause: LinearColor inputs expect LinearColor values, not ColorCurve DIs. The ColorCurve DI's `SampleColorCurve(float)` function outputs LinearColor, but requires a Dynamic Input wrapper to bind properly.
  - Fix: Implemented two-approach system in `NiagaraCurveInputService::SetModuleColorCurveInput()`:
    1. **For LinearColor inputs**: Uses `FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput()` to attach a Dynamic Input script (e.g., "Scale Linear Color by Curve") that wraps the curve sampling
    2. **For explicit curve inputs**: Direct ColorCurve DI assignment (original logic)
  - Added helper functions: `FindDynamicInputScriptForType()` to find suitable Dynamic Input scripts, and `FindAndConfigureColorCurveOnDynamicInput()` to configure the curve on nested inputs
  - Now properly creates color gradients on Color module inputs, displaying as "Scale Linear Color by Curve" in the editor

- **NiagaraMCP set_emitter_enabled Command Not Implemented** - Fixed 2026-01-08
  - Issue: Python wrapper existed but C++ command handler was missing
  - Error: `{"status": "error", "error": "Unknown command: set_emitter_enabled"}`
  - Fix: Implemented `FSetEmitterEnabledCommand` and `FNiagaraService::SetEmitterEnabled()` using `FNiagaraEmitterHandle::SetIsEnabled()`
  - Now supports enabling/disabling emitters within Niagara systems

- **NiagaraMCP remove_emitter_from_system Command Missing** - Fixed 2026-01-08
  - Issue: No command existed to remove emitters from a Niagara system
  - Error: `{"status": "error", "error": "Unknown command: remove_emitter_from_system"}`
  - Fix: Implemented `FRemoveEmitterFromSystemCommand` and `FNiagaraService::RemoveEmitterFromSystem()` using `UNiagaraSystem::RemoveEmitterHandle()`
  - Now supports removing emitters from systems programmatically

- **NiagaraMCP set_renderer_property Enum Properties Not Supported** - Fixed in NiagaraService.cpp
  - Issue: Enum properties like `Alignment`, `FacingMode`, `SortMode` on sprite renderers couldn't be set
  - Fix: Added `FEnumProperty` and `FByteProperty` handling to `SetRendererProperty()`
  - Now supports all enum properties with helpful error messages listing valid values
  - Example: `set_renderer_property(..., "Alignment", "VelocityAligned")` now works

- **MaterialMCP Missing Expression Types for Particle/VFX** - Fixed in MaterialExpressionService.cpp
  - Added: `ParticleColor`, `VertexColor`, `SphereMask`, `Dot`/`DotProduct`, `Distance`, `Normalize`, `Saturate`, `Sqrt`/`SquareRoot`, `TextureCoordinate`
  - Enables creation of proper particle materials that read Niagara color/alpha attributes

- **MaterialMCP Operations Trigger "Apply Changes" Dialog** - Fixed in MaterialExpressionService.cpp
  - Issue: Multiple operations triggered Material Editor's "apply changes?" dialog when editor was open, blocking MCP
  - Fix: Added PreEditChange/PostEditChange + MarkPackageDirty before CloseAllEditorsForAsset() in all 5 locations
  - Affected functions: ConnectExpressions, ConnectExpressionsBatch, ConnectToMaterialOutput, DeleteExpression, SetExpressionProperty

- **Material Expression Connections Return Success But Don't Persist** - Fixed in MaterialExpressionService.cpp
  - Issue: `connect_material_expressions` returned `success: true` but connections were lost after reopening the material.
  - Root Cause: Code was using graph-level connections (`MakeLinkTo()`, `TryCreateConnection()`) and syncing in wrong direction (`LinkMaterialExpressionsFromGraph()`). MaterialGraph is transient - rebuilt from expressions on load.
  - Fix: Use UE5's `ConnectExpression()` method and sync graph FROM expressions:
    ```cpp
    SourceExpr->ConnectExpression(TargetInput, Params.SourceOutputIndex);
    Material->MaterialGraph->LinkGraphNodesFromMaterial();  // Expressions → Graph
    ```
  - Key insight: `LinkGraphNodesFromMaterial()` = Expressions → Graph (CORRECT). `LinkMaterialExpressionsFromGraph()` = Graph → Expressions (UNRELIABLE for persistence).

- **NiagaraMCP set_module_input + add_module_to_emitter Combination Causes Compilation Errors** - Fixed in NiagaraModuleService.cpp
  - Issue: Using both tools together caused "ParameterMap traversal" compilation errors
  - Root Cause: `set_module_input` was setting rapid iteration parameters on only ONE script, but Niagara expects them on ALL affected scripts (system spawn/update + emitter scripts)
  - Fix: Implemented equivalent of `FNiagaraStackGraphUtilities::FindAffectedScripts()` (not exported) to collect all affected scripts, then set rapid iteration parameter on ALL of them
  - Now `set_module_input` and `add_module_to_emitter` can be used together without corruption

- **MaterialMCP set_material_expression_property Crashes When Setting DefaultValue on ScalarParameter** - Fixed in MaterialExpressionCreation.cpp
  - Issue: `set_material_expression_property` with `property_name="DefaultValue"` on a ScalarParameter crashed Unreal Editor
  - Error: `EXCEPTION_ACCESS_VIOLATION reading address 0x0000000000000000`
  - Root Cause: The code was calling `PostEditChangeProperty()` on the expression, which triggers `UMaterialExpressionScalarParameter::PostEditChangeProperty()`. This broadcasts `FEditorSupportDelegates::NumericParameterDefaultChanged` delegate, which expects `Expression->Material` to be valid. However, expressions added via `EditorData->ExpressionCollection.AddExpression()` don't have their `Material` member set (only the UObject Outer is set).
  - Fix: Removed the `PreEditChange`/`PostEditChangeProperty` pattern for ScalarParameter and VectorParameter DefaultValue changes. Since `RecompileMaterial()` is called later anyway, these notifications were redundant. Now values are set directly:
    ```cpp
    ScalarParam->DefaultValue = NewValue;  // Direct assignment, no PostEditChangeProperty
    ```
  - Also applies to VectorParameter DefaultValue for consistency

## Notes

- When working on MCP improvements, check this file first
- Add new issues as they are discovered during development
