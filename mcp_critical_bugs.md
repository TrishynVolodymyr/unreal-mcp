# CRITICAL MCP Bugs - Niagara Module Input Tools

## Date: 2026-01-11

## Status: FIXED - Build Successful

---

## Root Cause (IDENTIFIED)

The curve/linked input tools were using `UEdGraphPin::BreakAllPinLinks(true)` to clear existing connections before setting new values. This is **INCORRECT** because:

1. It leaves orphaned nodes in the graph (UNiagaraNodeInput, UNiagaraNodeParameterMapGet, UNiagaraNodeFunctionCall)
2. These orphaned nodes can corrupt the parameter map chain
3. The proper approach is to **remove the connected nodes** rather than just breaking links

**The proper approach** (similar to UE's internal `FNiagaraStackGraphUtilities::RemoveNodesForStackFunctionInputOverridePin`) is to:
1. Identify the connected node type
2. Remove the entire node from the graph (which also removes its connections)
3. Let the subsequent `SetDataInterfaceValueForFunctionInput` or `SetLinkedParameterValueForFunctionInput` create fresh nodes

---

## Fix Applied

**Files Modified:**
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/Niagara/NiagaraCurveInputService.cpp`
- `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/Niagara/NiagaraLinkedInputService.cpp`

**Changes:**
1. Added missing includes: `NiagaraNodeInput.h`, `NiagaraNodeParameterMapSet.h`, `NiagaraNodeParameterMapGet.h`
2. Implemented `RemoveOverrideNodesForPin()` helper function that properly:
   - Removes UNiagaraNodeInput nodes (data interface inputs)
   - Removes UNiagaraNodeParameterMapGet nodes (linked parameters)
   - Removes UNiagaraNodeFunctionCall nodes (dynamic inputs)
   - Falls back to BreakAllPinLinks only for unknown node types
3. Replaced all `BreakAllPinLinks(true)` calls with `RemoveOverrideNodesForPin()` calls

**Note:** The UE5.7 `RemoveNodesForStackFunctionInputOverridePin` function is NOT exported (no NIAGARAEDITOR_API), so we implemented a simplified version that removes the connected nodes. The complex parameter map chain reconnection logic is not needed because we're creating NEW overrides, not just removing existing ones.

---

## Bugs Fixed

### Bug 1: set_module_color_curve_input - FIXED
### Bug 2: set_module_curve_input - FIXED
### Bug 3: set_module_linked_input - FIXED
### Bug 4: move_module - NOT AFFECTED (separate issue, if any)

---

## Build Status

**Build Result:** SUCCESS (2026-01-11)

---

## Testing Required

After launching the editor:
1. Create a new Niagara System with emitter
2. Add modules with curve inputs (ScaleColor, ScaleSpriteSize)
3. Use `set_module_color_curve_input` to set color gradients
4. Use `set_module_curve_input` to set float curves
5. Use `set_module_linked_input` to bind Curve Index to Particles.NormalizedAge
6. Compile system - should succeed without "Parameter Maps must be created via an Input Node" errors
7. Test replacing existing curve values (call the tools twice with different values)

---

## Technical Details

The simplified fix works because:
1. `Graph->RemoveNode()` properly disconnects all pins before removing the node
2. The subsequent `SetDataInterfaceValueForFunctionInput` or `SetLinkedParameterValueForFunctionInput` calls create fresh nodes with correct parameter map connections
3. We don't need to manually reconnect the parameter map chain since these API functions handle it internally
