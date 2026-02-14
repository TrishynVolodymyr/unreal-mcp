# Niagara Tools - Unreal MCP

This document provides comprehensive information about using Niagara visual effects tools through the Unreal MCP (Model Context Protocol). These tools allow you to create, configure, and manage Niagara particle systems entirely through natural language commands.

## Overview

Niagara tools enable you to:
- Create and duplicate Niagara Systems and Emitters
- Add, remove, enable/disable emitters within systems
- Configure emitter properties (CPU/GPU sim, local space, determinism)
- Add and configure renderers (Sprite, Mesh, Ribbon, Light)
- Search and add modules to emitter stages (Spawn, Update, Event)
- Set module inputs with values, curves, color gradients, and random ranges
- Link module inputs to particle attributes (e.g., NormalizedAge)
- Configure static switches on modules
- Set system-level float, vector, and color parameters
- Add user-exposed parameters for runtime control
- Inspect emitter modules, module inputs, and renderer properties
- Run validation diagnostics on systems
- Spawn Niagara actors in the level
- Compile and save systems

## Natural Language Usage Examples

### Creating Effects

```
"Create a new Niagara System called 'NS_FireExplosion' in /Game/Effects/Fire"

"Create a sprite emitter called 'NE_Sparks' and add it to NS_FireExplosion"

"Duplicate NS_FireExplosion as NS_FireExplosion_Blue"

"Add a ribbon emitter called 'NE_Trail' to the fire system — create it if it doesn't exist"
```

### Configuring Particles

```
"Set the spawn rate to 500 on NS_FireExplosion"

"Add a Gravity Force module to the Update stage of NE_Sparks"

"Set the particle lifetime to random between 1 and 3 seconds"

"Create a color gradient that goes from bright yellow to dark red over the particle lifetime"

"Set the sprite size to random between 5 and 20"

"Switch the Sparks emitter to GPU simulation"
```

### Materials and Rendering

```
"Set the material on the Sparks renderer to M_EmberParticle"

"Add a ribbon renderer to the Trail emitter"

"Get the renderer properties for NE_Sparks"
```

### Inspecting and Debugging

```
"Show me all modules in the NE_Sparks emitter organized by stage"

"Get the inputs for the InitializeParticle module in the Spawn stage"

"Run diagnostics on NS_FireExplosion to check for issues"

"What parameters are exposed on this Niagara system?"
```

## Tool Reference

---

### `create_niagara_system`

Create a new Niagara System — the top-level container for visual effects.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the system (e.g., `"NS_FireExplosion"`) |
| `folder_path` | string | | Folder path for the asset |
| `base_system` | string | | Path to an existing system to duplicate from |
| `auto_activate` | boolean | | Whether to auto-activate when spawned (default: True) |

---

### `duplicate_niagara_system`

Duplicate an existing Niagara System to create a variation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `source_system` | string | ✅ | Path or name of the source system |
| `new_name` | string | ✅ | Name for the duplicate |
| `folder_path` | string | | Folder path for the new asset |

---

### `get_niagara_system_metadata`

Get metadata and configuration from a Niagara System including emitters and parameters.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the Niagara System |

---

### `compile_niagara_system`

Force recompilation and save of a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |

---

### `create_niagara_emitter`

Create a new standalone Niagara Emitter asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Name of the emitter (e.g., `"NE_Sparks"`) |
| `folder_path` | string | | Folder path for the asset |
| `template` | string | | Template emitter path to copy from |
| `emitter_type` | string | | `"Sprite"` (default), `"Mesh"`, `"Ribbon"`, `"Light"` |

---

### `add_emitter_to_system`

Add an emitter to a Niagara System. Can create the emitter if it doesn't exist.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the target system |
| `emitter` | string | ✅ | Path or name of the emitter to add |
| `create_if_missing` | boolean | | Create emitter if it doesn't exist |
| `emitter_folder` | string | | Folder path for creating emitter |
| `emitter_type` | string | | `"Sprite"`, `"Mesh"`, `"Ribbon"`, `"Light"` |

---

### `remove_emitter_from_system`

Remove an emitter from a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `emitter` | string | ✅ | Name of the emitter to remove |

---

### `set_emitter_enabled`

Enable or disable an emitter in a system without removing it.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `emitter` | string | ✅ | Name of the emitter |
| `enabled` | boolean | | True to enable, False to disable |

---

### `set_emitter_property`

Set a property on an emitter (SimTarget, LocalSpace, Determinism, etc.).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `emitter` | string | ✅ | Name of the emitter |
| `property_name` | string | ✅ | `"LocalSpace"`, `"Determinism"`, `"RandomSeed"`, `"SimTarget"`, `"RequiresPersistentIDs"`, `"MaxGPUParticlesSpawnPerFrame"` |
| `property_value` | string | ✅ | Value as string (e.g., `"GPU"`, `"true"`, `"42"`) |

---

### `get_emitter_properties`

Get properties from an emitter (SimTarget, LocalSpace, bounds mode, etc.).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `emitter` | string | ✅ | Name of the emitter |

---

### `set_niagara_float_param` / `set_niagara_vector_param` / `set_niagara_color_param`

Set float, vector, or color parameters on a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `param_name` | string | ✅ | Parameter name (e.g., `"SpawnRate"`) |
| `value` / `x,y,z` / `r,g,b,a` | number | ✅ | Value(s) to set |

---

### `get_niagara_parameters`

Get all user-exposed parameters from a Niagara System with current values.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |

---

### `add_niagara_parameter`

Add a user-exposed parameter to a Niagara System.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `parameter_name` | string | ✅ | Name for the new parameter |
| `parameter_type` | string | ✅ | `"Float"`, `"Int"`, `"Bool"`, `"Vector"`, `"LinearColor"` |
| `default_value` | any | | Default value (native type or string) |
| `scope` | string | | `"user"` (default), `"system"`, `"emitter"` |

---

### `add_renderer_to_emitter`

Add a renderer to an emitter. Ribbon renderers default to MultiPlane for volumetric fire trails.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |
| `emitter` | string | ✅ | Name of the emitter |
| `renderer_type` | string | ✅ | `"Sprite"`, `"Mesh"`, `"Ribbon"`, `"Light"` |

---

### `set_renderer_property`

Set a property on a renderer (Material, Alignment, Shape, etc.).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `renderer_name` | string | ✅ | Renderer name (e.g., `"Renderer"`) |
| `property_name` | string | ✅ | `"Material"`, `"Alignment"`, `"FacingMode"`, `"Shape"`, `"RibbonWidth"`, etc. |
| `property_value` | string | ✅ | Value (asset path for materials, or setting value) |

---

### `get_renderer_properties`

Get all properties and bindings from a renderer.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `renderer_name` | string | | Renderer name (default: `"Renderer"`) |

---

### `search_niagara_modules`

Search available Niagara modules in the asset registry.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `search_query` | string | | Text to search in module names |
| `stage_filter` | string | | `"Spawn"`, `"Update"`, `"Event"`, or `""` for all |
| `max_results` | number | | Maximum results (default: 50) |

---

### `add_module_to_emitter`

Add a module to a specific stage of an emitter.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_path` | string | ✅ | Path to the module script asset |
| `stage` | string | ✅ | `"Spawn"`, `"Update"`, `"Event"`, `"EmitterSpawn"`, `"EmitterUpdate"` |
| `index` | number | | Position in stack (-1 = end) |

---

### `remove_module_from_emitter`

Remove a module from an emitter stage.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Name of the module to remove |
| `stage` | string | ✅ | Stage containing the module |

---

### `set_module_input`

Set an input value on a module and/or enable/disable it.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `input_name` | string | | Input parameter name |
| `value` | string | | Value as string (e.g., `"100"`, `"0,0,500"`) |
| `value_type` | string | | `"float"`, `"vector"`, `"color"`, `"int"`, `"bool"`, `"auto"` |
| `enabled` | boolean | | Enable/disable the module |

---

### `move_module`

Move a module to a new position within its stage for execution order control.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `new_index` | number | ✅ | New position index (0-based) |

---

### `set_module_curve_input`

Set a float curve input on a module (e.g., Scale Over Life).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `input_name` | string | ✅ | Input parameter name |
| `keyframes` | array | ✅ | List of `{time, value}` keyframes (time: 0.0-1.0) |

---

### `set_module_color_curve_input`

Set a color curve (gradient) input on a module.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `input_name` | string | ✅ | Input parameter name |
| `keyframes` | array | ✅ | List of `{time, r, g, b, a}` keyframes |

---

### `set_module_random_input`

Set a random range input on a module for natural variation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `input_name` | string | ✅ | Input parameter name |
| `min_value` | string | ✅ | Minimum value (e.g., `"1.0"` or `"-50,-50,100"`) |
| `max_value` | string | ✅ | Maximum value |

---

### `set_module_linked_input`

Bind a module input to a particle attribute (e.g., `Particles.NormalizedAge`).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `input_name` | string | ✅ | Input parameter name |
| `linked_value` | string | ✅ | Particle attribute (e.g., `"Particles.NormalizedAge"`) |

---

### `set_module_static_switch`

Set a static switch on a Niagara module for compile-time branching.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |
| `switch_name` | string | ✅ | Static switch name |
| `value` | string | ✅ | Display name, internal name, index, or bool |

---

### `get_module_inputs`

Get all inputs for a module with types and current values.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |
| `module_name` | string | ✅ | Module name |
| `stage` | string | ✅ | Stage containing the module |

---

### `get_emitter_modules`

Get all modules in an emitter organized by stage (EmitterSpawn, EmitterUpdate, ParticleSpawn, ParticleUpdate, Event).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the system |
| `emitter_name` | string | ✅ | Emitter name |

---

### `get_niagara_diagnostics`

Run all validation rules against a Niagara System and report issues.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system` | string | ✅ | Path or name of the system |

---

### `spawn_niagara_actor`

Spawn a Niagara System actor in the level.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `system_path` | string | ✅ | Path to the Niagara System asset |
| `actor_name` | string | ✅ | Name for the spawned actor |
| `location` | array | | `[X, Y, Z]` world location (default: `[0, 0, 0]`) |
| `rotation` | array | | `[Pitch, Yaw, Roll]` in degrees |
| `auto_activate` | boolean | | Whether to auto-activate on spawn (default: True) |

## Advanced Usage Patterns

### Building a Complete Fire Effect with Embers and Trails

```
"Create a full fire effect system:
1. Create a Niagara System called 'NS_CampFire' in /Game/Effects/Fire
2. Create a sprite emitter 'NE_Flames' and add it to the system
3. Set spawn rate to 200 and lifetime to random between 0.5 and 1.5 seconds
4. Add a color gradient from bright yellow [1,1,0.2,1] to dark red [0.5,0,0,0] over lifetime
5. Add a Scale Sprite Size module with a curve that grows then shrinks
6. Set the material on the Flames renderer to M_FireParticle
7. Create a second emitter 'NE_Embers' with spawn rate 50 and GPU simulation
8. Add Gravity Force to Embers Update stage and set lifetime to random 2-4 seconds
9. Add a ribbon emitter 'NE_Smoke' with low spawn rate and large sprite size
10. Compile and spawn the system actor in the level at [0, 0, 100]"
```

### Creating a Stylized Magic Projectile

```
"Build a magic projectile effect:
1. Create NS_MagicProjectile with a sprite emitter 'NE_Core' for the glowing center
2. Set NE_Core to local space, spawn rate 30, small sprite size 5-10
3. Add a color gradient cycling through blues and purples
4. Add a ribbon emitter 'NE_Trail' for the trailing ribbon
5. Set ribbon width to a curve that starts at 20 and tapers to 0
6. Set the material on both renderers
7. Add a user-exposed LinearColor parameter 'EffectColor' for runtime tinting
8. Compile the system"
```

### Duplicating and Tweaking Effect Variations

```
"Create a blue fire variant from existing fire:
1. Duplicate NS_CampFire as NS_CampFire_Blue
2. Get the emitter modules for NE_Flames to find the color module
3. Set the color gradient to go from bright cyan [0,0.8,1,1] to deep blue [0,0,0.3,0]
4. Set the emissive tint on NE_Embers to blue
5. Run diagnostics on NS_CampFire_Blue to verify everything is valid
6. Compile the system"
```

## Best Practices for Natural Language Commands

### Specify Stage Names Explicitly
Instead of: *"Add a gravity module to the emitter"*
Use: *"Add a Gravity Force module to the Update stage of NE_Sparks"*

### Use Random Ranges for Natural Variation
Instead of fixed values: *"Set lifetime to 2 seconds"*
Use: *"Set particle lifetime to random between 1.5 and 3.0 seconds"* — this prevents the uniform, artificial look.

### Inspect Modules Before Modifying
Before changing inputs, ask: *"Get the inputs for the InitializeParticle module in the Spawn stage of NE_Flames"* to see available parameter names and current values.

### Search Modules When Unsure of Names
Use: *"Search Niagara modules for gravity"* or *"Search Niagara modules for color"* to discover exact module paths before adding them.

### Run Diagnostics After Complex Changes
After building or modifying a system: *"Run diagnostics on NS_CampFire"* to catch missing connections, invalid modules, or configuration issues before testing in-game.

## Common Use Cases

### Environmental Effects
Creating ambient particle systems for weather (rain, snow, fog), fire (torches, campfires, bonfires), and atmospheric effects (dust motes, fireflies, pollen).

### Weapon and Projectile Effects
Building projectile trails, muzzle flashes, impact sparks, and magical auras using sprite and ribbon emitters with local-space simulation.

### Character VFX
Designing character-attached effects like healing auras, damage indicators, footstep dust, and power-up particles using emitters bound to skeletal meshes.

### Destruction and Impact Effects
Creating one-shot burst effects for explosions, debris, shattering, and impact decal spawners with high initial spawn counts and short lifetimes.

### UI and Feedback Effects
Building screen-space particle effects for collectible pickups, level-up celebrations, and ability cooldown visualizations.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"Emitter not found in system"**: Emitter names inside a system may differ from the asset name. Use `get_niagara_system_metadata` to list all emitters and their actual names within the system.
2. **Module input not applying**: Module input names are case-sensitive and must match exactly. Use `get_module_inputs` to list available inputs for a specific module and stage.
3. **Particles not visible after spawn**: Check that auto_activate is True, the material is assigned with the correct usage flag (sprite/ribbon/mesh), and the emitter is enabled. Use `get_renderer_properties` to verify material assignment.
4. **GPU simulation errors**: Not all modules support GPU simulation. If switching to GPU causes issues, use `get_niagara_diagnostics` to identify incompatible modules and switch them or revert to CPU.
5. **Color gradient not appearing**: Ensure the color curve module is linked to `Particles.NormalizedAge` for the curve index. Use `set_module_linked_input` to bind the curve index input.

## Performance Considerations

- **Choose CPU vs GPU simulation wisely**: GPU simulation handles large particle counts (thousands+) efficiently but limits which modules can be used. For small particle counts (<100), CPU simulation has less overhead.
- **Limit spawn rates**: High spawn rates (1000+) with long lifetimes create large particle pools. Keep alive particle count reasonable — use `get_niagara_diagnostics` to check particle budgets.
- **Use local space when possible**: Local-space emitters are cheaper to update when attached to moving actors since particle positions are relative rather than world-space.
- **Minimize renderer count per emitter**: Each renderer adds a draw call. Combine visual elements into fewer emitters when the particle behavior is similar rather than using many single-purpose emitters.
