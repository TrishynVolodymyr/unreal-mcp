# MCP Tool Issues Log - Fire Embers VFX Creation

## Date: 2026-01-11

---

## Issue 1: Cannot Set Emitter CalculateBoundsMode

**Tool**: `set_emitter_property`

**Problem**: Cannot set `CalculateBoundsMode` property on Niagara emitter. The tool only supports: `LocalSpace`, `Determinism`, `RandomSeed`, `SimTarget`, `RequiresPersistentIDs`, `MaxGPUParticlesSpawnPerFrame`

**Error Message**:
```
Unknown emitter property 'CalculateBoundsMode'. Valid properties: LocalSpace, Determinism, RandomSeed, SimTarget, RequiresPersistentIDs, MaxGPUParticlesSpawnPerFrame
```

**Impact**: GPU emitters show warning about dynamic bounds mode. Fixed bounds are required for proper GPU culling/rendering.

**Niagara Diagnostic Warning**:
```
The emitter is GPU and is using dynamic bounds mode.
Please update the Emitter or System properties otherwise bounds may be incorrect.
```

**Workaround**: User must manually set CalculateBoundsMode to "Fixed" and configure FixedBounds in Unreal Editor.

**Suggested Fix**: Add `CalculateBoundsMode` and `FixedBounds` to supported properties in `set_emitter_property` tool.

---

## Issue 2: Cannot Set Material Usage Flags

**Tool**: Material MCP tools (create_material, etc.)

**Problem**: When creating materials for Niagara particles, the "Used with Niagara Sprites" usage flag is not automatically set, and there's no MCP tool to set material usage flags.

**Niagara Diagnostic Warning**:
```
Materials might not render correctly.
Some materials 'M_FireEmber' do not have the correct usage flags set, and will use the default material.
```

**Impact**: Material will display as default material in Niagara until user manually enables usage flag.

**Workaround**: User must manually check "Used with Niagara Sprites" in Material Editor under Usage settings.

**Suggested Fix**:
1. Add `set_material_usage_flags` tool to materialMCP
2. Or auto-detect when material is assigned to Niagara renderer and set flag automatically

---

## Summary

| Issue | Tool | Severity | Workaround Available |
|-------|------|----------|---------------------|
| CalculateBoundsMode not settable | niagaraMCP | Medium | Manual in Editor |
| Material usage flags not settable | materialMCP | Medium | Manual in Editor |

Both issues require manual intervention in Unreal Editor to fully resolve.
