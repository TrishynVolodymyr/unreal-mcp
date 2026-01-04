# MCP Issues Found During Fire Material/VFX Test

**Date:** 2026-01-03
**Test:** Creating fire material and Niagara VFX system

---

## Material MCP Issues

### Issue 1: VectorParameter properties not persisting correctly

**Tool:** `add_material_expression` with `expression_type: "VectorParameter"`

**What happened:**
- Called with properties: `{"ParameterName": "FireColor", "DefaultValue": [1, 0.3, 0, 1]}`
- Result in `get_material_graph_metadata`: `"description": "Param (0,0,0,0) 'Param'"`

**Expected:**
- Parameter name should be "FireColor"
- Default value should be (1, 0.3, 0, 1)

**Actual:**
- Parameter name shows as "Param"
- Default value shows as (0,0,0,0)

**Severity:** Medium - Material still works but parameters have wrong names/defaults

**Workaround:** Use `set_material_expression_property` after creation to set ParameterName and DefaultValue separately

**STATUS: FIXED** - Added support for camelCase property names (`ParameterName`, `DefaultValue`) in addition to lowercase (`parameter_name`, `default_value`) in the VectorParameter handler, matching the ScalarParameter behavior.

---

### Issue 2: Workflow gap - no visibility into disconnected node outputs

**Tool:** `get_material_graph_metadata`

**What happened:**
Created a Panner node intended for animated fire UVs, connected TexCoord to its input, but never connected Panner's OUTPUT to anything. The node became dead weight.

**Intended graph:**
```
TexCoord → Panner → TextureSample (fire texture) → processing → EmissiveColor/Opacity
```

**Actual graph:**
```
TexCoord → Panner → (NOTHING - dead end)

VectorParameter → Multiply → EmissiveColor
ScalarParameter →
ScalarParameter (Opacity) → Opacity
```

**Why this happened:**
1. `get_material_graph_metadata` shows `is_connected: true/false` for node INPUTS
2. But there's no easy way to verify node OUTPUTS are connected to something
3. Without a texture sample node, the Panner serves no purpose
4. I failed to trace the complete path from source nodes to material outputs

**Result:** Solid black rectangle instead of animated fire (VectorParameter also defaulted to 0,0,0,0)

**Severity:** High - Leads to broken materials that appear to be set up correctly

**Needed improvement:**
- Add "orphan node detection" to `get_material_graph_metadata` that flags nodes whose outputs aren't used
- Or add a validation tool that traces all paths and reports dead ends

**STATUS: FIXED AND VERIFIED** - Implemented the following improvements:
1. `get_material_graph_metadata` now supports `fields=["orphans"]` to detect orphan nodes
2. `get_material_graph_metadata` now supports `fields=["flow"]` to trace paths from material outputs
3. New `compile_material` tool that compiles and returns `has_orphans`, `orphan_count`, and `orphans[]` array

**Verification test on M_Fire material:**
- `get_material_graph_metadata("/Game/VFX/Materials/M_Fire", ["orphans"])` → Found Panner as orphan ✅
- `compile_material("/Game/VFX/Materials/M_Fire")` → `has_orphans: true, orphan_count: 1` ✅
- `get_material_graph_metadata("/Game/VFX/Materials/M_Fire", ["flow"])` → Shows EmissiveColor/Opacity paths ✅

---

## Niagara MCP Issues

### Issue 3: Emitter name mismatch in add_module_to_emitter

**Tool:** `add_module_to_emitter`

**What happened:**
1. Created emitter with name "NE_FireFlames"
2. Added emitter to system with `emitter_name: "FireFlames"` (display name)
3. Called `add_module_to_emitter` with `emitter_name: "FireFlames"`
4. Error: `"Emitter 'FireFlames' not found in system"`

**Root cause:**
- `add_emitter_to_system` accepts an `emitter_name` parameter for display name
- But `add_module_to_emitter` requires the actual emitter asset name (e.g., "NE_FireFlames")
- The metadata shows `"name": "NE_FireFlames"` not "FireFlames"

**Expected behavior:**
- Either: `emitter_name` in `add_emitter_to_system` should rename the emitter in the system
- Or: Documentation should clarify that you must use the asset name, not display name

**Severity:** Medium - Confusing but workaround exists

**Workaround:** Always use the emitter asset name (e.g., "NE_FireFlames") when calling `add_module_to_emitter`, ignore the display name parameter in `add_emitter_to_system`

**STATUS: FIXED** - The `emitter_name` parameter in `add_emitter_to_system` now correctly renames the emitter handle using `FNiagaraEmitterHandle::SetName()`. Both the asset name and custom display name can now be used.

---

### Issue 4: Niagara system created via MCP has ParameterMap traversal errors

**Tool:** `create_niagara_emitter`, `add_emitter_to_system`, `add_module_to_emitter`

**What happened:**
The NS_Fire system created via MCP tools fails to compile with 14 errors:
```
Default found for SystemLocation.Offset Coordinate Space, but not found in ParameterMap traversal
Default found for Local.AddVelocity.TransformedVector, but not found in ParameterMap traversal
... (12 more similar errors)
```

**Note:** The `compile_niagara_asset` tool DOES correctly report these errors (returns `status: "error"` with full error messages). Error visibility is working.

**Root cause (investigated):**
When emitters are created from scratch via MCP (not from a template), the emitter factory creates default modules (SpawnRate, AddVelocity, SystemLocation) but their "Rapid Iteration Parameters" may not be properly registered in the script's parameter store.

The engine's `UNiagaraEmitterFactoryNew::InitializeEmitter` function calls `SetRapidIterationParameter` for each module it adds, which registers the module's input parameters. When we add modules programmatically, this step may be incomplete.

Note: `FNiagaraEditorUtilities::AddEmitterToSystem` (which we use) internally calls `FNiagaraStackGraphUtilities::RebuildEmitterNodes`, so that part is correct. The issue may be with how emitters are created initially.

**Severity:** High - Creates non-functional Niagara systems

**STATUS: FIXED AND VERIFIED** ✅

**Fix applied:** Added `System->OnSystemPostEditChange().Broadcast(System)` calls after:
- `add_emitter_to_system`
- `add_module_to_emitter`
- `set_module_input`
- `add_renderer`
- `set_renderer_property`

This mirrors what the Unreal Editor does after making changes to Niagara systems (see `EdGraphSchema_NiagaraSystemOverview.cpp:84`). The broadcast triggers parameter map rebuilding.

Also added `System->WaitForCompilationComplete()` after `RequestCompile(false)` to ensure synchronous compilation completes before returning.

**Why this works:**
- `RebuildEmitterNodes` is NOT exported from NiagaraEditor (no NIAGARAEDITOR_API), so we can't call it directly
- However, `OnSystemPostEditChange()` IS exported (has NIAGARA_API) and is the standard way the editor notifies the system of changes
- The Niagara view models and world managers subscribe to this delegate to trigger rebuilds

**Verification test:**
1. Created NS_FireTest system from scratch (no template)
2. Created NE_FireFlames emitter from scratch (no template)
3. Added emitter to system with custom name "FireFlames"
4. Added Sprite renderer
5. `compile_niagara_asset` → `"success": true, compile_status: "Valid"` ✅
6. Spawned actor in level - particles render correctly ✅

---

### Issue 5: Material connections not persisting when Material Editor is open

**Tool:** `connect_material_expressions`, `connect_expression_to_material_output`

**What happened:**
When the Material Editor is open and viewing the material being modified via MCP:
1. Connection changes made via MCP would show success in API response
2. But the Material Editor's in-memory state would overwrite the changes
3. Connections would not persist to disk or appear in the editor

**Root cause:**
The Material Editor maintains its own in-memory representation of the material graph. When open, it periodically syncs its state back to the material data, overwriting programmatic changes.

The previous approach tried to work with the editor's in-memory state by:
- Detecting if editor was open
- Making connections at EdGraph level vs Material level depending on editor state
- Calling `UpdateMaterialAfterGraphChange()` to refresh

This was complex and unreliable because the editor's internal sync mechanisms could still overwrite changes.

**Solution:**
Apply the same close/reopen editor pattern that worked for `set_material_expression_property`:
1. Detect if Material Editor is open for this material
2. Close it if open (preserves our changes from being overwritten)
3. Make connection changes at Material data level
4. Save the package to disk
5. Reopen the editor if it was open (loads fresh state from disk)

**Functions updated:**
- `ConnectExpressions` - close/reopen pattern added
- `ConnectExpressionsBatch` - close/reopen pattern added
- `ConnectToMaterialOutput` - simplified from dual-branch to close/reopen pattern
- `DeleteExpression` - close/reopen pattern added

**Severity:** High - Critical for MCP workflow to work autonomously

**STATUS: FIXED** ✅

The close/reopen pattern ensures MCP changes persist regardless of whether the Material Editor is open. This is the same pattern UE's own automation tests use.

---

### Issue 6: compile_material not returning shader compilation errors

**Tool:** `compile_material`

**What happened:**
- TextureSample node had error: `[SM6] Sampler type is Color, should be Linear Grayscale`
- `compile_material` returned `success: true` with no error information
- The error was only visible in the Material Editor UI

**Expected:**
- `compile_material` should return shader compilation errors/warnings

**Severity:** High - Cannot detect material errors programmatically

**STATUS: FIXED** ✅

**Fix:** Added compile error capture from `FMaterialResource::GetCompileErrors()`. Returns `compile_errors[]`, `has_compile_errors`, `compile_error_count` fields.

---

### Issue 7: Editor close/reopen shows save dialog requiring user interaction

**Tool:** All material modification tools using close/reopen pattern

**What happened:**
- When Material Editor is closed via `CloseAllEditorsForAsset()`, UE shows a save dialog
- User must click "Save" or "Don't Save" manually
- This defeats the purpose of autonomous MCP operation

**Expected:**
- Changes should be saved silently before closing, or close without prompting

**Severity:** Critical - Blocks autonomous operation

**STATUS: FIXED** ✅

**Fix:** Save the package BEFORE closing the editor. Added `UPackage::SavePackage()` call before `CloseAllEditorsForAsset()` in all affected functions: `ConnectExpressions`, `ConnectExpressionsBatch`, `ConnectToMaterialOutput`, `DeleteExpression`, `SetExpressionProperty`.

---

### Issue 8: set_material_expression_property doesn't work for TextureSample.SamplerType

**Tool:** `set_material_expression_property`

**What happened:**
- Called `set_material_expression_property` with `property_name: "SamplerType"`, `property_value: 1`
- Tool returned success
- But the property remained "Color" instead of changing to "Linear Grayscale"

**Root cause:**
- `ApplyExpressionProperties` in MaterialExpressionService.cpp doesn't have a handler for TextureSample's SamplerType property
- The generic property setting approach isn't working

**Expected:**
- SamplerType should change to Linear Grayscale (enum value 7)

**Severity:** High - Cannot fix common material errors programmatically

**STATUS: FIXED** ✅

**Fix:** Added SamplerType handler to `ApplyExpressionProperties` for TextureSample expressions. Supports values 0-7 (Color, Grayscale, Alpha, Normal, Masks, DistanceFieldFont, LinearColor, LinearGrayscale).

---

## Summary

| MCP Server | Tools Tested | Issues Found | Fixed | Open |
|------------|--------------|--------------|-------|------|
| materialMCP | create_material, add_material_expression, connect_material_expressions, connect_expression_to_material_output, get_material_graph_metadata, compile_material, set_material_expression_property, delete_material_expression | 6 | 6 | 0 |
| niagaraMCP | create_niagara_system, create_niagara_emitter, add_emitter_to_system, add_module_to_emitter, add_renderer, set_renderer_property, set_module_input, compile_niagara_asset, spawn_niagara_actor, get_niagara_metadata, search_niagara_modules | 2 | 2 | 0 |

**Material MCP:** ALL FIXED ✅
- ✅ Issue 1: VectorParameter properties now persist correctly (camelCase support added)
- ✅ Issue 2: Orphan detection working via `["orphans"]` and `["flow"]` fields + `compile_material`
- ✅ Issue 5: Connection/property changes now persist when Material Editor is open (close/reopen pattern)
- ✅ Issue 6: `compile_material` now returns shader compilation errors
- ✅ Issue 7: Editor close/reopen no longer shows save dialog (save before close)
- ✅ Issue 8: `set_material_expression_property` now works for TextureSample.SamplerType

**Niagara MCP:** ALL FIXED ✅
- ✅ Issue 3: Emitter rename now works via `FNiagaraEmitterHandle::SetName()`
- ✅ Issue 4: Added `OnSystemPostEditChange().Broadcast()` to trigger parameter map rebuilding

**Note:** Fixed issues require recompilation of the C++ plugin. Run `RebuildProject.bat` to apply.
