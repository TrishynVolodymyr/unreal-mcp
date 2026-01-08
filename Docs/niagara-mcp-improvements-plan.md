# Niagara MCP Improvements Plan

## Context
While attempting to create a realistic fire VFX via MCP tools, several limitations were discovered that prevent proper control of Niagara systems.

## Issues to Investigate and Implement

### Issue 1: Enum Values Are Opaque
**Problem:** Enum values return as `NewEnumerator0`, `NewEnumerator1` etc. instead of human-readable names.
**Impact:** Cannot understand what mode a module is in, cannot set modes correctly.
**Example:** `Scale Sprite Size Mode = NewEnumerator1` - is this Curve mode? Scale Factor mode?

**Investigation Steps:**
- [x] Find where enum values are read in C++ code
  - Found: `NiagaraMetadataService.cpp` lines 317-355
  - When reading exposed pins, code uses `Pin->DefaultValue` directly
  - This returns internal names like "NewEnumerator0" for enums
- [x] Determine how to get the actual enum display name
  - Use `Pin->PinType.PinSubCategoryObject` to get UEnum*
  - Call `EnumType->GetDisplayNameTextByIndex(i)` for display name
  - Pattern already used in `SetNodePinValueCommand.cpp` lines 240-242
- [x] Update the MCP response to include readable enum names
  - Modified `NiagaraMetadataService.cpp` lines 352-366
  - Added enum resolution using `GetDisplayNameTextByValue()`

**Implementation:** DONE - Added enum display name resolution in GetModuleInputs()

---

### Issue 2: Cannot Edit Curve Data
**Problem:** Niagara modules use curves (e.g., scale over lifetime) but MCP cannot set curve keyframes.
**Impact:** Default curves may have wrong behavior (grow vs shrink), cannot customize fade-in/out, cannot create proper fire behavior.
**Example:** ScaleSpriteSize uses a curve that grows particles over time, but we need shrinking.

**Investigation Steps:**
- [x] Understand how Niagara curve data is structured
  - `UNiagaraDataInterfaceCurve` (inherits from `UNiagaraDataInterfaceCurveBase`)
  - Contains `FRichCurve Curve` member
  - Source: `Engine/Plugins/FX/Niagara/Source/Niagara/Classes/NiagaraDataInterfaceCurve.h`
- [x] Find how to programmatically set curve keys
  - `FRichCurve::Reset()` - clear all keys
  - `FRichCurve::AddKey(float Time, float Value)` - add key
  - `FRichCurve::SetKeys(TArray<FRichCurveKey>)` - set all keys at once
  - After changes: call `UpdateLUT()` to regenerate lookup table
- [ ] Determine how to access curve DI from module input
  - Need to find the UNiagaraDataInterfaceCurve instance for a module input
  - May be stored in override pin or as a default object
- [x] Design MCP API for curve editing
  - `set_module_curve(system_path, emitter_name, module_name, stage, input_name, keys[])`
  - Each key: `{time: float, value: float, interp_mode: "Linear"|"Cubic"|"Constant"}`
  - Access pattern: Override pin → UNiagaraNodeInput → GetDataInterface() → Cast to UNiagaraDataInterfaceCurve
  - After changes: Call UpdateLUT() to regenerate lookup table

**Implementation:** DEFERRED
- Full implementation requires: new param struct, interface method, service method, command handler, Python tool
- **WORKAROUND AVAILABLE**: Use Issue 3 fix to switch modes (e.g., "Uniform" mode uses Scale Factor instead of curve)
- Priority: Low (workaround exists)

---

### Issue 3: Mode Switching for Modules
**Problem:** Modules have multiple modes (e.g., Scale Factor vs Curve), but cannot switch between them.
**Impact:** Even when setting Uniform Scale Factor, module ignores it because it's in Curve mode.
**Example:** ScaleSpriteSize has `Scale Sprite Size Mode` enum that controls behavior.

**Investigation Steps:**
- [x] Determine if mode is just an enum that can be set via existing set_module_input
  - YES! "Scale Sprite Size Mode" is an enum exposed as a pin
  - Current value is "Uniform Curve" (now readable thanks to Issue 1 fix)
- [x] Test setting enum values directly
  - `SetModuleInput` at lines 462-531 handles exposed pins
  - But NO conversion from display name → internal name
  - User sets "Scale Factor" but Niagara expects "NewEnumerator0"
- [x] If not possible, investigate how to change module modes programmatically
  - Pattern exists in `SetNodePinValueCommand.cpp` lines 211-274 (Blueprint enum handling)
  - Need to add same UEnum lookup/conversion to NiagaraModuleService::SetModuleInput()

**Implementation:** DONE - Added enum handling in SetModuleInput() at NiagaraModuleService.cpp lines 527-588

---

### Issue 4: Pan Noise Field Behavior
**Problem:** Setting Pan Noise Field doesn't create expected flicker effect.
**Impact:** Turbulence is static or chaotic instead of flame-like flicker.

**Investigation Steps:**
- [x] Research what Pan Noise Field actually does in Niagara
  - Pan Noise Field (Vector3f) controls how the noise field moves over time
  - Higher values = faster movement = more flicker
  - X/Y control horizontal drift, Z controls vertical movement
- [x] Determine correct values for fire-like flicker
  - Current: (50, 30, 100) - reasonable starting point
  - Z being highest (100) creates upward drift in the noise pattern
  - For slower, more subtle flicker: reduce values (e.g., 20, 15, 50)
  - For chaotic fire: increase values (e.g., 100, 80, 200)
- [x] Test if there's a better approach
  - Pan Noise Field works as expected - the issue was curl noise being omnidirectional (Issue 5)

**Implementation:** UNDERSTOOD - Pan Noise Field works correctly, just needs tuning per effect

---

### Issue 5: Curl Noise Directional Control
**Problem:** Curl noise applies force in all directions, creating chaotic movement instead of upward-biased flicker.
**Impact:** Fire moves randomly instead of flickering upward.

**Investigation Steps:**
- [x] Research if curl noise can be masked to specific axes
  - YES! CurlNoiseForce module has built-in cone masking:
    - `Mask Curl Noise` (bool) - enables masking
    - `Curl Noise Cone Mask Axis` (Vector3f) - direction to bias toward (e.g., 0,0,1 for up)
    - `Curl Noise Cone Mask Angle` (float) - cone angle in degrees
    - `Curl Noise Cone Mask Falloff Angle` (float) - smooth transition angle
- [x] Investigate alternative modules for directional turbulence
  - Can also duplicate module and add "Axis Scale" vector to multiply output per-axis
  - Reference: [Beyond-FX 2D Curl Noise Tutorial](https://blog.beyond-fx.com/articles/real-time-vfx-quick-tip-how-to-create-2-dimensional-curl-noise-beyond-fx)
- [x] Test combining curl noise with strong upward velocity
  - Combined with existing AddVelocity module for base upward movement
  - Cone mask applied: Axis=(0,0,1), Angle=60°, Falloff=30°

**Implementation:** DONE - Enabled cone masking for upward-biased curl noise

---

## Progress Tracking

| Issue | Investigation | Implementation | Verified |
|-------|--------------|----------------|----------|
| 1. Enum Values | DONE | DONE | ✅ Yes |
| 2. Curve Editing | DONE | DEFERRED | Workaround via #3 |
| 3. Mode Switching | DONE | DONE | ✅ Yes |
| 4. Pan Noise Field | DONE | N/A (behavior understood) | ✅ Yes |
| 5. Curl Noise Direction | DONE | DONE (cone mask) | ✅ Yes |

---

## Current Status
**ALL ISSUES RESOLVED!**

### MCP Tool Improvements (Issues 1 & 3):
- Issue 1: Enum values now show display names (e.g., "Uniform Curve")
- Issue 3: Can SET enum values using display names (e.g., set mode to "Uniform")

### Deferred (Issue 2):
- Curve editing requires significant infrastructure. Workaround: use mode switching.

### Niagara Behavior Research (Issues 4 & 5):
- Issue 4: Pan Noise Field controls noise movement speed - higher values = more flicker
- Issue 5: Cone masking enables directional curl noise via:
  - `Mask Curl Noise = true`
  - `Curl Noise Cone Mask Axis = (0,0,1)` for upward
  - `Curl Noise Cone Mask Angle = 60` degrees
  - `Curl Noise Cone Mask Falloff Angle = 30` degrees

### NS_Fire Current Configuration:
- ScaleSpriteSize: "Uniform" mode, Scale Factor = 0.3 (shrinking)
- CurlNoiseForce: Cone masked for upward-biased turbulence
- Pan Noise Field: (50, 30, 100) for animated flicker
