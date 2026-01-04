# Troubleshooting Reference

Common Niagara issues and solutions.

---

## Effect Not Visible

### Check Rendering

| Issue | Solution |
|-------|----------|
| No renderer added | Add renderer via `add_renderer` |
| Wrong material | Verify material path, check material domain |
| Material not particle-compatible | Use particle material domain or Unlit |
| Blend mode mismatch | Additive needs bright colors, Translucent needs alpha |

### Check Spawning

| Issue | Solution |
|-------|----------|
| SpawnRate = 0 | Set spawn rate > 0 |
| Lifetime = 0 | Set lifetime > 0 |
| SpawnProbability = 0 | Set to 1.0 |
| Emitter disabled | Check Emitter State module |

### Check Transforms

| Issue | Solution |
|-------|----------|
| Spawning at wrong location | Check spawn location module, system transform |
| Particles too small | Increase SpriteSize / MeshScale |
| Particles behind camera | Check spawn location Z value |

### Check System State

| Issue | Solution |
|-------|----------|
| System not compiled | Run `compile_niagara_asset` |
| System not active | Check system spawned with AutoActivate |
| Emitter not added to system | Run `add_emitter_to_system` |

---

## Performance Issues

### Diagnosis Steps

1. **Check particle count** — Use `stat Niagara` in editor
2. **Check GPU vs CPU** — GPU should be default
3. **Check overdraw** — Visualize with Buffer Visualization > Overdraw
4. **Check module complexity** — Simplify update logic

### Common Causes

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| Frame drops on spawn | Too many CPU particles | Switch to GPU or reduce count |
| Gradual slowdown | Particle accumulation | Reduce lifetime, add kill conditions |
| GPU spikes | Complex material | Simplify material, reduce instructions |
| Memory growth | Too many systems active | Pool and reuse, auto-destroy |

### Particle Count Guidelines

```
stat Niagara
```

| Effect Type | Warning | Critical |
|-------------|---------|----------|
| Ambient | > 1000 | > 3000 |
| Gameplay | > 5000 | > 10000 |
| One-shot | Brief spike OK | Settle < 1s |

### Overdraw Checklist

- [ ] Particle size appropriate (not too large)
- [ ] Lifetime not excessive
- [ ] Depth fade enabled for scene intersection
- [ ] Ribbon tessellation minimized
- [ ] No stacked translucent sprites

---

## Module Not Working

### Input Binding Issues

| Issue | Solution |
|-------|----------|
| Input value not applied | Check parameter name matches exactly |
| Curve not interpolating | Verify curve has multiple keys |
| Random not randomizing | Use proper random function syntax |

### Module Order Issues

| Issue | Solution |
|-------|----------|
| Force not affecting particles | Ensure Initialize Particle comes before forces |
| Color not visible | Check Scale Color after initial color set |
| Position wrong | Check spawn location BEFORE velocity modules |

### Parameter Scope Issues

| Issue | Solution |
|-------|----------|
| User parameter not accessible | Ensure parameter exists at correct scope |
| Emitter vs Particle scope | Match parameter scope to where it's used |
| System parameter not visible | Parameters must be exposed at system level |

---

## Compilation Errors

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "Unresolved input" | Module input not connected | Set input value or bind parameter |
| "Type mismatch" | Wrong data type for input | Check expected type (float vs vector) |
| "Missing module" | Referenced module not found | Search and add correct module name |
| "Invalid parameter" | Parameter doesn't exist | Add parameter via `add_niagara_parameter` |

### Resolution Steps

1. Read error message — identifies specific module/input
2. Check module exists — `search_niagara_modules`
3. Verify input types — `get_niagara_metadata` to inspect
4. Re-set problematic inputs — `set_module_input`
5. Recompile — `compile_niagara_asset`

---

## Collision Not Detecting

### Prerequisites

- **CPU simulation required** — Collision does not work on GPU
- **CollisionQuery Data Interface** — Must be added
- **Collision module** — Must be in ParticleUpdate

### Setup Checklist

1. Emitter sim target = `CPUSim`
2. Add CollisionQuery DI:
   ```
   add_data_interface(
     data_interface_type="CollisionQuery",
     data_interface_name="SceneCollision"
   )
   ```
3. Add collision module to ParticleUpdate
4. Set collision response (Kill, Bounce, etc.)

### Performance Warning

Collision is expensive:
- Limit particle count (< 100 ideal)
- Use collision intervals, not every frame
- Consider GPU raymarching for visual-only collision

---

## Data Interface Issues

### Static Mesh DI

| Issue | Solution |
|-------|----------|
| Mesh not found | Verify asset path is correct |
| Sampling wrong location | Check UV/transform settings |
| No normals | Enable normal output on sampler |

### Skeletal Mesh DI

| Issue | Solution |
|-------|----------|
| Not following animation | Ensure skeletal mesh actor is set |
| Wrong bone | Verify bone name matches skeleton |
| Delayed following | Check update frequency |

### Curve DI

| Issue | Solution |
|-------|----------|
| Curve flat | Add keys to curve asset |
| Wrong value range | Check curve min/max |
| Not updating | Ensure curve asset is saved |

---

## Ribbon / Trail Issues

### Visual Issues

| Issue | Solution |
|-------|----------|
| Ribbon twisting | Reduce spawn rate, check facing mode |
| Gaps in ribbon | Increase spawn rate |
| Z-fighting | Enable depth fade or adjust sort priority |

### Performance Issues

| Issue | Solution |
|-------|----------|
| High overdraw | Reduce tessellation, shorten lifetime |
| Too many vertices | Lower TessellationFactor |
| Sorting expensive | Consider disabling sorting if acceptable |

### Ribbon Settings Checklist

```
TessellationMode = Custom
TessellationFactor = 2-4 (minimum needed)
FacingMode = Screen (most compatible)
```

---

## Material Issues

### Common Problems

| Issue | Solution |
|-------|----------|
| Black particles | Check material emissive or base color |
| Pink particles | Material not found, verify path |
| No alpha | Material blend mode must support alpha |
| Too bright | Reduce emissive intensity |

### Material Domain Requirements

| Renderer | Required Domain |
|----------|-----------------|
| Sprite | Surface or Unlit |
| Mesh | Surface |
| Ribbon | Surface or Unlit |

### Blend Mode Guide

| Visual Goal | Blend Mode | Notes |
|-------------|------------|-------|
| Glowing, additive | Additive | Bright colors only |
| Semi-transparent | Translucent | Alpha controls opacity |
| Cutout/masked | Masked | Hard edges |
| Solid | Opaque | No transparency |

---

## Quick Diagnostic Commands

**In-Editor:**
```
stat Niagara           // Particle counts, CPU/GPU time
stat GPU               // GPU timing breakdown
profileGPU             // Detailed GPU profiling
Niagara.DebugDraw 1    // Visual debug info
```

**Verify Asset State:**
```
get_niagara_metadata(asset_path)
// Returns: emitters, modules, parameters, renderers
```

---

## MCP-Specific Issues

### Tool Call Failed

| Error | Solution |
|-------|----------|
| Asset not found | Check path starts with /Game/ |
| Module not found | Use `search_niagara_modules` first |
| Invalid group | Use exact group name: EmitterUpdate, ParticleSpawn, etc. |
| Property not found | Check property name case-sensitivity |

### Workflow Issues

| Issue | Solution |
|-------|----------|
| Changes not visible | Run `compile_niagara_asset` |
| Module added to wrong group | Remove and re-add to correct group |
| Parameter not accessible | Check scope (Emitter vs Particle vs User) |

### Best Practice

After any structural change:
1. `compile_niagara_asset`
2. `spawn_niagara_actor` to test
3. Verify visually in editor
