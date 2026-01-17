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

### MaterialMCP compile_material Doesn't Report All Shader Compilation Errors
- **Affected**: `compile_material` in materialMCP
- **Date Discovered**: 2026-01-11
- **Problem**: `compile_material` returns `success: true` with empty `compile_errors` array even when shader compilation fails
- **Example**: Noise expression with 2D input (TexCoord) instead of required 3D input causes shader error, but MCP reports success
- **Actual Error** (visible in UE Output Log):
  ```
  [SM6] Shader debug info dumped to: "E:\code\unreal-mcp\MCPGameProject\Saved\ShaderDebugInfo\PCD3D_SM6\M_Ember_..."
  C:\Program Files\Epic Games\UE_5.7\Engine\Shaders\Private\MaterialTemplate.ush(4567,18):
  Shader FLumenCardPS, Permutation 0, VF FLocalVertexFactory:
  /Engine/Generated/Material.ush:4567:18: error: no matching function for call to 'MaterialExpressionNoise'
  float Local11 = MaterialExpressionNoise( Local10 ,8.00000000f,1.00000000f,0.00000000f,...);
  ```
- **MCP Response** (incorrect):
  ```json
  {
    "success": true,
    "compile_errors": [],
    "has_compile_errors": false
  }
  ```
- **Impact**: AI thinks material compiled successfully when it actually has broken shaders
- **Root Cause**: MCP tool likely only checks `UMaterial::GetCompileErrors()` which returns expression-level errors, not shader compilation errors from the HLSL compiler
- **Workaround**: Always visually verify material preview in Unreal Editor after compile; check Output Log for shader errors
- **Suggested Fix**: Capture shader compilation errors from `FMaterialResource::GetCompileErrors()` or hook into shader compilation result callbacks

---

## Resolved Issues

- **NiagaraMCP get_module_inputs Cannot Read Default UniformRanged Dynamic Input Min/Max Values** - Fixed 2026-01-17
  - Issue: When a module input uses an UniformRanged dynamic input with default/unmodified values (e.g., systems created in Unreal Editor using stock dynamic inputs), the min/max values could not be read because they were baked into the dynamic input script asset.
  - Root Cause: Unreal stores dynamic input values in multiple locations:
    1. Override pins (created when user modifies defaults)
    2. RapidIterationParameters (alternative storage)
    3. Script asset defaults (baked into the dynamic input script graph)
    - Previous implementation only read from locations 1 and 2, missing location 3.
  - Fix: Added APPROACH 4 in `ExtractUniformRangedValues()` that loads the dynamic input script's graph via `DynamicInputNode->FunctionScript->GetLatestSource()` and reads the `UNiagaraScriptVariable` default values for "Minimum" and "Maximum" parameters using `GetDefaultValueData()`.
  - Now `get_module_inputs` returns `random_min` and `random_max` for ALL UniformRanged dynamic inputs, including those with stock/default values.

- **MaterialMCP Created Assets Don't Persist to Disk** - Fixed 2026-01-11
  - Issue: Materials created via `create_material` existed in memory but weren't saved to disk. "Save All" didn't persist them.
  - Root Cause: `CreateMaterial()`, `CreateMaterialInstance()`, and `DuplicateMaterialInstance()` only called `MarkPackageDirty()` but never actually saved the package to disk
  - Fix: Added explicit `UPackage::SavePackage()` calls after asset creation in MaterialService.cpp
  - Note: NiagaraMCP already had proper `SaveAsset()` calls - this was MaterialMCP-specific

- **MaterialMCP Several Expression Types Not Recognized (Noise, ParticleRandom, Length)** - Fixed 2026-01-11
  - Issue: Expression types `Noise`, `ParticleRandom`, and `Length` returned "Unknown expression type" errors
  - Root Cause: These expression classes were not mapped in `GetExpressionClassFromTypeName()`
  - Fix: Added includes for `MaterialExpressionNoise.h`, `MaterialExpressionParticleRandom.h`, `MaterialExpressionLength.h` and registered them in the expression type map
  - Noise expression now supports all configurable properties: Scale, Quality, Levels, OutputMin, OutputMax, LevelScale, Turbulence, Tiling, RepeatSize, NoiseFunction (enum 0-5)

- **MaterialMCP ComponentMask Channel Configuration Not Supported** - Fixed 2026-01-11
  - Issue: ComponentMask R/G/B/A channel selection wasn't working, causing "component mask 0000" compile errors
  - Root Cause: Was actually working - the issue was in how properties were being passed from Python side
  - Verification: Tested with `{"R": true, "G": false, "B": false, "A": false}` - material compiles successfully

- **MaterialMCP RadialGradientExponential Expression Not Supported** - Fixed 2026-01-09
  - Issue: Expression type "RadialGradientExponential" was not recognized
  - Root Cause: RadialGradientExponential is NOT a native expression - it's a Material Function at `/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential`
  - Fix: Added `MaterialFunctionCall` expression type support. Now any Material Function can be used:
    ```python
    add_material_expression(
        material_path="/Game/Materials/M_Ember",
        expression_type="MaterialFunctionCall",
        properties={"function": "/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential"}
    )
    ```
  - Also added `FunctionCall` alias for convenience

- **MaterialMCP ScalarParameter DefaultValue Not Applied** - Fixed 2026-01-09
  - Issue: `DefaultValue` property for ScalarParameter expressions not applied when specified in properties dict
  - Root Cause: Property name case sensitivity - C++ checked for `default_value` (lowercase) but Python sent `DefaultValue` (camelCase)
  - Fix: Now accepts both camelCase and lowercase property names in MaterialExpressionCreation.cpp

- **NiagaraMCP Cannot Set Curve/Gradient Values Over Particle Lifetime** - Fixed 2026-01-09
  - Issue: Cannot set values that change over particle lifetime (fade out, color changes, size curves)
  - Fix: Added `set_module_curve_input` and `set_module_color_curve_input` commands with keyframe support
  - For LinearColor inputs, uses Dynamic Inputs (e.g., "Scale Linear Color by Curve") to wrap curve sampling

- **NiagaraMCP Cannot Set Random Range Values** - Fixed 2026-01-09
  - Issue: Cannot set randomized inputs - particles all had identical values
  - Fix: Added `set_module_random_input` MCP tool that attaches UniformRangedFloat/Vector/Color dynamic input scripts
  - Supports Float, Int, Vector2D, Vector3, Vector4, and LinearColor types

- **NiagaraMCP Module Search Returns Empty for Multi-Word Queries** - Fixed 2026-01-09
  - Issue: `search_niagara_modules("Scale Sprite")` returned 0 results, but single words worked
  - Root Cause: Search used exact substring match for entire query including spaces
  - Fix: Split query into words and match ALL words independently (AND logic)
  - Now `"Scale Sprite"` → 3 results, `"Curl Noise"` → 5 results

- **NiagaraMCP LinearColor Type Cannot Be Set via set_module_input** - Fixed 2026-01-09
  - Issue: Setting LinearColor parameters returned success but value didn't change
  - Root Cause: Was likely a format issue - the original tests used `(R=1.0, G=0.7, B=0.2, A=1.0)` format
  - Fix: Use comma-separated format `1.0,0.5,0.0,1.0` with `value_type="color"`
  - Verified working on Color module in Update stage

- **NiagaraMCP Module Input Type Discovery Missing** - Fixed 2026-01-09
  - Issue: No way to know what type a module input expects before setting it
  - Fix: Added `get_module_inputs` tool that returns input names, types, and current values
  - Example: `get_module_inputs(system, emitter, "Color", "Update")` returns `[{name: "Color", type: "LinearColor"}, ...]`

- **NiagaraMCP add_emitter_to_system create_if_missing Returns Unknown Error** - Fixed 2026-01-09
  - Issue: `create_if_missing=True` always returned "Unknown error" even when emitter creation succeeded
  - Root Cause: MCP framework wraps responses as `{"status": "success", "result": {...}}` but Python code checked for `success` at top level
  - Fix: Updated Python to unwrap MCP response format and check `result.success` or `status == "success"`

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
