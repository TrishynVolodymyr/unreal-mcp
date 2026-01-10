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

**Common Shapes:**
- Box: `BoxSize`, `SurfaceOnly`
- Sphere: `SphereRadius`, `SurfaceOnly`, `HemisphereOnly`
- Cylinder: `CylinderHeight`, `CylinderRadius`
- Torus: `LargeRadius`, `SmallRadius`
- Mesh: `MeshPath`, `SampleMode`

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
| P4 | `add_spawn_location_module` | Missing (use `add_module_to_emitter` for now) |

---

## Current Workaround Limitations

Without these tools, the only options are:
1. Create emitters from scratch with NO template (but then missing essential modules)
2. Manually edit in Unreal Editor and use MCP only for parameters
3. Guess and iterate blindly without verification

---

## Notes for Implementation

### UE Classes to Research:
- `UNiagaraSystem` - System asset
- `UNiagaraEmitter` - Emitter asset
- `FNiagaraEmitterHandle` - Emitter instance in system
- `UNiagaraScript` - Module scripts
- `FNiagaraVariable` - Parameter variables
- `UNiagaraStackViewModel` - Editor stack UI model (may have useful APIs)
- `FNiagaraParameterStore` - Parameter storage
- `UNiagaraSpriteRendererProperties` - Sprite renderer settings
- `UNiagaraRendererProperties` - Base renderer class

### Key Patterns:
- Modules are `UNiagaraScript` assets added to emitter stacks
- Parameters are stored in `FNiagaraParameterStore`
- Renderers are `UNiagaraRendererProperties` subclasses
- Changes require `PostEditChange()` and recompilation
