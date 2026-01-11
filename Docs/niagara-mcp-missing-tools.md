# Niagara MCP - Missing Tools Analysis

**Date:** 2026-01-10
**Context:** Attempting to create fire embers VFX via MCP tools
**Problem:** Set values don't appear to take effect, no way to debug/inspect

---

## Critical Issue

When using `set_module_input` and other setters, the values don't visibly change in the effect. Without inspection tools, it's impossible to:
1. Verify if values were actually applied
2. See what modules exist and might be overriding
3. Understand the current state of the emitter

---

## ~~Missing Tools - Priority 1: Inspection/Debug~~ ✅ IMPLEMENTED

### 1.1 `get_emitter_modules` ✅ IMPLEMENTED

Now available as `get_emitter_modules()` in niagaraMCP.

---

### 1.2 `get_module_current_values` ✅ IMPLEMENTED

Now available as `get_module_inputs()` in niagaraMCP - returns current values with types.

---

### 1.3 `get_renderer_properties` ✅ IMPLEMENTED

Now available as `get_renderer_properties()` in niagaraMCP.

---

## ~~Missing Tools - Priority 2: Renderer Configuration~~

### 2.1 ~~`set_sprite_renderer_stretch`~~ ❌ NOT NEEDED

**Status:** Redundant - existing tools cover this functionality.

**Why Not Needed:**
- Velocity-based stretching in Niagara is done via **modules**, not renderer properties
- `UNiagaraSpriteRendererProperties::VelocityScale` does NOT exist in UE5.7
- Use existing tools instead:

**Correct Approach:**
```python
# 1. Set renderer alignment (already supported)
set_renderer_property(system_path, emitter_name, "Renderer", "Alignment", "VelocityAligned")

# 2. Add velocity-based size scaling module
add_module_to_emitter(system_path, emitter_name,
    "/Niagara/Modules/Update/Size/SpriteSizeScaleByVelocity.SpriteSizeScaleByVelocity", "Update")

# 3. Configure stretch amount
set_module_input(system_path, emitter_name, "SpriteSizeScaleByVelocity", "Update",
    "Velocity Threshold", "500", "float")
set_module_input(system_path, emitter_name, "SpriteSizeScaleByVelocity", "Update",
    "Min Scale Factor", "1,1", "vector")
set_module_input(system_path, emitter_name, "SpriteSizeScaleByVelocity", "Update",
    "Max Scale Factor", "1,1.5", "vector")
```

**Available Modules for Velocity Stretching:**
- `/Niagara/Modules/Update/Size/SpriteSizeScaleByVelocity.SpriteSizeScaleByVelocity`
- `/Niagara/Modules/Update/Size/ScaleSpriteSizeBySpeed.ScaleSpriteSizeBySpeed`

---

### 2.2 ~~`set_renderer_binding`~~ ✅ ALREADY SUPPORTED

**Status:** Use existing `set_renderer_property()` - it handles bindings via reflection.

**Example:**
```python
set_renderer_property(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    renderer_name="Renderer",
    property_name="ColorBinding",      # or "SizeBinding", "VelocityBinding", etc.
    property_value="Particles.Color"   # or custom attribute name
)
```

**Available Bindings:** `ColorBinding`, `PositionBinding`, `VelocityBinding`, `SpriteSizeBinding`, `SpriteRotationBinding`, `SubImageIndexBinding`, etc.

---

## ~~Missing Tools - Priority 3: Module Management~~ PARTIALLY IMPLEMENTED

### 3.1 `remove_module_from_emitter` ✅ IMPLEMENTED

Now available as `remove_module_from_emitter()` in niagaraMCP.

---

### 3.2 `set_module_enabled` ✅ IMPLEMENTED (via `set_module_input`)

**Purpose:** Enable/disable a module without removing it

Now available via the `enabled` parameter on `set_module_input()`:

```python
# Disable a module
set_module_input(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_name="Gravity Force",
    stage="Update",
    enabled=False
)

# Enable a module
set_module_input(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_name="Gravity Force",
    stage="Update",
    enabled=True
)
```

---

## ~~Missing Tools - Priority 4: Emitter Properties~~ ✅ IMPLEMENTED

### 4.1 `set_emitter_property` ✅ IMPLEMENTED

Now available as `set_emitter_property()` in niagaraMCP:

```python
# Switch emitter to GPU simulation
set_emitter_property(
    system="NS_FireExplosion",
    emitter="Sparks",
    property_name="SimTarget",
    property_value="GPU"
)

# Enable local space simulation
set_emitter_property(
    system="NS_FireExplosion",
    emitter="Sparks",
    property_name="LocalSpace",
    property_value="true"
)

# Set deterministic seed
set_emitter_property(
    system="NS_FireExplosion",
    emitter="Sparks",
    property_name="RandomSeed",
    property_value="42"
)
```

**Supported Properties:**
- `LocalSpace`: Whether particles simulate in local space (true/false)
- `Determinism`: Whether simulation is deterministic (true/false)
- `RandomSeed`: Seed for deterministic random (integer)
- `SimTarget`: Simulation target - "CPU" or "GPU"
- `RequiresPersistentIDs`: Enable persistent particle IDs (true/false)
- `MaxGPUParticlesSpawnPerFrame`: Max GPU particles per frame (integer)

---

### 4.2 `get_emitter_properties` ✅ IMPLEMENTED

Now available as `get_emitter_properties()` in niagaraMCP:

```python
get_emitter_properties(
    system="NS_FireExplosion",
    emitter="Sparks"
)
# Returns:
# {
#   "success": true,
#   "emitter_name": "Sparks",
#   "properties": {
#     "LocalSpace": false,
#     "Determinism": false,
#     "RandomSeed": 0,
#     "SimTarget": "GPU",
#     "RequiresPersistentIDs": false,
#     "MaxGPUParticlesSpawnPerFrame": 0,
#     "CalculateBoundsMode": "Dynamic"
#   }
# }
```

---

## Missing Tools - Priority 5: Spawn Configuration

### 5.1 `add_spawn_location_module`

**Purpose:** Add spawn shape modules (box, sphere, cylinder, etc.)

**Why Needed:**
- Embers should spawn from an area, not a point
- Different effects need different spawn volumes
- Control initial particle distribution

**Expected Parameters:**
```python
add_spawn_location_module(
    system_path: str,
    emitter_name: str,
    shape_type: str,      # "Box", "Sphere", "Cylinder", "Torus", "Mesh"
    properties: dict      # Shape-specific: {"BoxSize": [100, 100, 50]}
)
```

**Common Shapes (use `get_module_inputs` to see all parameters):**
- Box: `Box Size`, `Surface Only Band Thickness`
- Sphere: `Sphere Radius`, `Hemisphere`
- Cylinder: `Cylinder Height`, `Cylinder Radius`
- Torus: `Large Radius`, `Handle Radius`
- Mesh: `Mesh`, `Sampling Mode`

---

## Summary: Implementation Priority

| Priority | Tool | Status |
|----------|------|--------|
| ~~P0~~ | ~~`get_emitter_modules`~~ | ✅ IMPLEMENTED |
| ~~P0~~ | ~~`get_module_current_values`~~ | ✅ IMPLEMENTED (as `get_module_inputs`) |
| ~~P1~~ | ~~`get_renderer_properties`~~ | ✅ IMPLEMENTED |
| ~~P1~~ | ~~`remove_module_from_emitter`~~ | ✅ IMPLEMENTED |
| ~~P2~~ | ~~`set_sprite_renderer_stretch`~~ | ❌ NOT NEEDED (use modules) |
| ~~P2~~ | ~~`set_renderer_binding`~~ | ✅ ALREADY SUPPORTED (via `set_renderer_property`) |
| ~~P2~~ | ~~`set_module_enabled`~~ | ✅ IMPLEMENTED (via `set_module_input` enabled param) |
| ~~P3~~ | ~~`set_emitter_property`~~ | ✅ IMPLEMENTED |
| ~~P3~~ | ~~`get_emitter_properties`~~ | ✅ IMPLEMENTED |
| P4 | `add_spawn_location_module` | ✅ NOT NEEDED (use `add_module_to_emitter`) |
| ~~P5~~ | ~~Emitter stage module support~~ | ✅ IMPLEMENTED |

---

## Missing Tools - Priority 5: Emitter Stage Support

### 5.1 `add_spawn_location_module` ✅ NOT NEEDED

**Status:** Covered by existing `add_module_to_emitter` + `set_module_input` tools.

**Tested and Working:**
- BoxLocation: `/Niagara/Modules/Spawn/Location/BoxLocation.BoxLocation`
- SphereLocation: `/Niagara/Modules/Spawn/Location/SphereLocation.SphereLocation`
- CylinderLocation: `/Niagara/Modules/Spawn/Location/CylinderLocation.CylinderLocation`
- TorusLocation: `/Niagara/Modules/Spawn/Location/TorusLocation.TorusLocation`
- StaticMeshLocation: `/Niagara/Modules/Spawn/Location/StaticMeshLocation.StaticMeshLocation`

**Example:**
```python
# Add sphere spawn location
add_module_to_emitter(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_path="/Niagara/Modules/Spawn/Location/SphereLocation.SphereLocation",
    stage="Spawn"
)

# Configure radius
set_module_input(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_name="SphereLocation",
    stage="Spawn",
    input_name="Sphere Radius",
    value="150",
    value_type="float"
)
```

---

### 5.2 Emitter Stage Module Support ✅ IMPLEMENTED

**Status:** Full support for EmitterSpawn and EmitterUpdate stages.

**Supported Stages:**
- ✅ `Spawn` / `ParticleSpawn` (ParticleSpawnScript)
- ✅ `Update` / `ParticleUpdate` (ParticleUpdateScript)
- ✅ `Event` (ParticleEventScript)
- ✅ `EmitterSpawn` (EmitterSpawnScript)
- ✅ `EmitterUpdate` (EmitterUpdateScript)

**Tools Supporting Emitter Stages:**
- `add_module_to_emitter` - Add modules to any stage
- `remove_module_from_emitter` - Remove modules from any stage
- `set_module_input` - Set module inputs in any stage
- `get_module_inputs` - Query module inputs in any stage
- `get_emitter_modules` - Returns modules from ALL stages including EmitterSpawn/EmitterUpdate
- `move_module` - Move modules within any stage

**Example - Setting Spawn Rate:**
```python
# Add SpawnRate module to EmitterUpdate stage
add_module_to_emitter(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_path="/Niagara/Modules/Emitter/SpawnRate.SpawnRate",
    stage="EmitterUpdate"
)

# Configure spawn rate
set_module_input(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_name="SpawnRate",
    stage="EmitterUpdate",
    input_name="SpawnRate",
    value="100",
    value_type="float"
)
```

**Common EmitterUpdate Modules:**
- `/Niagara/Modules/Emitter/SpawnRate.SpawnRate` - Continuous spawn rate
- `/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous` - Burst spawn
- `/Niagara/Modules/Emitter/EmitterLifeCycle.EmitterLifeCycle` - Emitter lifecycle control

---

---

## Missing Tools - Priority 6: Module Static Switches & Linked Inputs

**Date:** 2026-01-10
**Context:** Attempting to create torus burst VFX with alpha fading over particle lifetime

### 6.1 `set_module_static_switch` - ✅ IMPLEMENTED

**Purpose:** Set enum/static switch values on modules that control behavior modes

**Now available as `set_module_static_switch()` in niagaraMCP:**
```python
set_module_static_switch(
    system_path="/Game/Effects/NS_Fire",
    emitter_name="NE_Sparks",
    module_name="Scale Color",
    stage="Update",
    switch_name="Scale Color Mode",
    value="Scale Over Life"  # Or "1", or "NewEnumerator1"
)
```

**Features:**
- Accepts display name ("Scale Over Life"), internal name ("NewEnumerator1"), or index ("1")
- Handles enum, bool, and integer static switches
- Triggers recompilation automatically after setting

**ORIGINAL ISSUE:**

**Why Needed:**
- Modules like `ScaleColor` have a "Scale Mode" that switches between direct values vs curve-based
- `get_module_inputs` shows these as "Type" with values like "0", "true", "false"
- Current `set_module_input` only handles float/vector/color/int/bool inputs, not enum selection
- Without this, curve-based fading doesn't work even when curves are configured

**Observed Problem:**
```python
# ScaleColor module inputs show:
{"name": "Scale Mode", "full_name": "Scale Mode", "type": "Type", "value": "0"}
{"name": "ScaleA", "full_name": "ScaleA", "type": "Type", "value": "true"}
{"name": "ScaleRGB", "full_name": "ScaleRGB", "type": "Type", "value": "true"}

# These are STATIC SWITCHES - they determine which code path the module uses
# "Scale Mode" = 0 means direct scaling, not curve-based
# Need to change to curve mode for alpha fading to work
```

**Expected Parameters:**
```python
set_module_static_switch(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str,
    switch_name: str,      # "Scale Mode", "ScaleA", etc.
    switch_value: str      # Enum name or index
)
```

**UE API Reference:**
- `UNiagaraNodeFunctionCall::FindStaticSwitchInputs()`
- `FNiagaraStackGraphUtilities::SetStaticSwitchDefaultValue()`
- Static switches are different from regular inputs - they're compile-time constants

---

### 6.2 `get_module_linked_inputs` - MISSING

**Purpose:** Inspect which module inputs are bound to particle attributes

**Why Needed:**
- After calling `set_module_linked_input`, no way to verify it worked
- `get_module_inputs` shows linked inputs as "[Default/Unset]" even after setting
- Need to see the actual binding (e.g., "Curve Index" → "Particles.NormalizedAge")

**Observed Problem:**
```python
# After set_module_linked_input(... input_name="Curve Index", linked_value="Particles.NormalizedAge")
# get_module_inputs still shows:
{"name": "Curve Index", "value": "[Default/Unset]", "value_mode": "Local"}

# No indication that it's linked to Particles.NormalizedAge
```

**Expected Parameters:**
```python
get_module_linked_inputs(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str
)
# Returns:
# {
#   "linked_inputs": [
#     {"input_name": "Curve Index", "linked_to": "Particles.NormalizedAge", "type": "float"}
#   ],
#   "unlinked_inputs": [...]
# }
```

**UE API Reference:**
- Check the override pin connections on module nodes
- Look for ParameterMapGet nodes connected to inputs
- `FNiagaraStackGraphUtilities::GetStackFunctionInputOverrideNode()`

---

### 6.3 `verify_module_configuration` - MISSING (Debug Tool)

**Purpose:** Comprehensive module state inspection for debugging

**Why Needed:**
- When effects don't work as expected, no way to understand full module state
- Need to see: enabled state, static switch values, input values, linked bindings, curve data  
- Would help diagnose issues like "alpha fading not working"

**Expected Parameters:**
```python
verify_module_configuration(
    system_path: str,
    emitter_name: str,
    module_name: str,
    stage: str
)
# Returns comprehensive state:
# {
#   "module_name": "ScaleColor",
#   "enabled": true,
#   "static_switches": {
#     "Scale Mode": {"value": 0, "options": ["Direct", "Curve", "Both"]},
#     "ScaleA": true,
#     "ScaleRGB": true
#   },
#   "inputs": {
#     "Scale Alpha": {"value": 1.0, "mode": "Local"},
#     "Curve Index": {"value": null, "mode": "Linked", "linked_to": "Particles.NormalizedAge"},
#     "Linear Color Curve": {"mode": "DataInterface", "keyframes": [...]}
#   },
#   "warnings": ["Scale Mode is 0 (Direct) but curve inputs are set - curve won't be used"]
# }
```

---

## Summary

All originally identified missing tools have been implemented. The Niagara MCP toolset now provides complete control over:
- System/Emitter creation and management
- Module operations in ALL stages (Emitter and Particle)
- Parameter configuration
- Renderer properties
- Spawn location shapes

**Remaining Gaps (Priority 6):**
| Tool | Purpose | Impact |
|------|---------|--------|
| `set_module_static_switch` | Set enum/mode values on modules | **HIGH** - Curve-based effects don't work without this |
| `get_module_linked_inputs` | Verify linked input bindings | MEDIUM - Debug/inspection capability |
| `verify_module_configuration` | Full module state dump | MEDIUM - Debug/inspection capability |
