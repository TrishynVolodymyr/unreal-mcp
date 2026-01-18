# Research: Fire Embers VFX - MCP Tool Capability Analysis

## Summary

Our MCP tools for Niagara and Materials are **mostly capable** of creating realistic fire embers, but have **critical gaps** in material graph creation and some missing convenience features. The core Niagara particle workflow is fully supported.

## Fire Embers VFX Requirements

### Essential Components for Realistic Fire Embers

1. **Niagara System & Emitter** - SUPPORTED
2. **GPU Simulation** - SUPPORTED (via `set_emitter_property` SimTarget)
3. **Spawn Rate Control** - SUPPORTED (via modules)
4. **Random Size Variation** - SUPPORTED (`set_module_random_input`)
5. **Random Lifetime** - SUPPORTED (`set_module_random_input`)
6. **Upward Velocity with Spread** - SUPPORTED (`set_module_random_input` for velocity)
7. **Turbulence/Curl Noise** - SUPPORTED (add Curl Noise Force module)
8. **Color Over Life Gradient** - SUPPORTED (`set_module_color_curve_input`)
9. **Alpha Fade Out** - SUPPORTED (`set_module_color_curve_input` with alpha)
10. **Size Scale Over Life** - SUPPORTED (`set_module_curve_input`)
11. **Fixed Bounds for GPU** - SUPPORTED (`set_emitter_property` CalculateBoundsMode + FixedBounds)
12. **Sprite Renderer with Material** - SUPPORTED (`set_renderer_property`)

### Material Requirements for Fire Embers

1. **Unlit Shading Model** - SUPPORTED
2. **Additive Blend Mode** - SUPPORTED
3. **Used with Niagara Sprites** - SUPPORTED (just added!)
4. **ParticleColor Expression** - SUPPORTED (`add_material_expression`)
5. **Radial Gradient** - SUPPORTED (`add_material_expression` type="RadialGradientExponential")
6. **Multiply Node** - SUPPORTED
7. **Connect to EmissiveColor** - SUPPORTED (`connect_expression_to_material_output`)
8. **Connect to Opacity** - SUPPORTED

---

## Current MCP Tool Capabilities

### Niagara MCP Tools (35+ tools) - COMPREHENSIVE

| Capability | Tool | Status |
|------------|------|--------|
| Create System | `create_niagara_system` | SUPPORTED |
| Create Emitter | `create_niagara_emitter` | SUPPORTED |
| Add Emitter to System | `add_emitter_to_system` | SUPPORTED |
| Set Sim Target (GPU/CPU) | `set_emitter_property` | SUPPORTED |
| Set Bounds Mode (Fixed) | `set_emitter_property` | SUPPORTED |
| Set Fixed Bounds | `set_emitter_property` | SUPPORTED |
| Add Modules | `add_module_to_emitter` | SUPPORTED |
| Set Module Inputs | `set_module_input` | SUPPORTED |
| Random Range Values | `set_module_random_input` | SUPPORTED |
| Float Curves | `set_module_curve_input` | SUPPORTED |
| Color Curves | `set_module_color_curve_input` | SUPPORTED |
| Linked Inputs | `set_module_linked_input` | SUPPORTED |
| Static Switches | `set_module_static_switch` | SUPPORTED |
| Get Module Inputs | `get_module_inputs` | SUPPORTED |
| Get Emitter Modules | `get_emitter_modules` | SUPPORTED |
| Set Renderer Property | `set_renderer_property` | SUPPORTED |
| Get Renderer Properties | `get_renderer_properties` | SUPPORTED |
| Spawn in Level | `spawn_niagara_actor` | SUPPORTED |
| Get Diagnostics | `get_niagara_diagnostics` | SUPPORTED |
| Compile | `compile_niagara_system` | SUPPORTED |

### Material MCP Tools (12 tools) - FUNCTIONAL BUT GAPS

| Capability | Tool | Status |
|------------|------|--------|
| Create Material | `create_material` | SUPPORTED |
| Set Blend Mode | `create_material` param | SUPPORTED |
| Set Shading Model | `create_material` param | SUPPORTED |
| Niagara Usage Flags | `create_material` params | SUPPORTED (just added!) |
| Create Material Instance | `create_material_instance` | SUPPORTED |
| Set Scalar Params | `set_material_scalar_param` | SUPPORTED |
| Set Vector Params | `set_material_vector_param` | SUPPORTED |
| Set Texture Params | `set_material_texture_param` | SUPPORTED |
| Add Expression | `add_material_expression` | SUPPORTED |
| Connect Expressions | `connect_material_expressions` | SUPPORTED |
| Connect to Output | `connect_expression_to_material_output` | SUPPORTED |
| Search Palette | `search_material_palette` | SUPPORTED |

---

## Gap Analysis

### CRITICAL GAPS (Block Realistic Fire Embers)

| Gap | Impact | Priority |
|-----|--------|----------|
| **Material not rendering with usage flags** | Custom materials with `used_with_niagara_sprites=true` don't render - may need `ForceRecompileForRendering()` or explicit save | CRITICAL |

### MODERATE GAPS (Workarounds Available)

| Gap | Impact | Workaround |
|-----|--------|------------|
| No `get_material_expressions` tool | Can't inspect existing material graph nodes | Create from scratch |
| No `delete_material_expression` tool | Can't remove nodes after creation | Recreate material |
| No `set_expression_property` tool | Can't modify expression properties after creation | Set during `add_material_expression` |
| No `compile_material` tool | Materials may not update immediately | Restart editor or use default materials |

### MINOR GAPS (Nice to Have)

| Gap | Impact | Notes |
|-----|--------|-------|
| No material template/preset system | Each material built from scratch | Could duplicate existing materials |
| No `get_material_graph_layout` | Can't visualize material node positions | Not critical for functionality |
| No vector curve input for modules | Color curves work, but no vector3 curves | Use color curve workaround |

---

## Recommendations

### Immediate Fixes Needed

1. **Material Shader Recompilation**
   - Issue: Materials created with usage flags may not compile shaders until editor restart
   - Fix: Add `Material->ForceRecompileForRendering()` and `Material->PostEditChange()` after setting usage flags
   - Location: `MaterialService.cpp` in `CreateMaterial()`

### New Tools to Add (Priority Order)

1. **`compile_material`** - Force material shader compilation
   - Ensures usage flags take effect immediately
   - Essential for Niagara workflow

2. **`get_material_expressions`** - List all expressions in a material graph
   - Enables inspection and modification workflows
   - Useful for debugging

3. **`delete_material_expression`** - Remove expression from graph
   - Enables iterative material editing
   - Prevents orphaned nodes

4. **`set_expression_property`** - Modify expression properties after creation
   - Enables tweaking without recreation
   - Improves iteration speed

---

## Fire Embers Creation Workflow (Current State)

### What WORKS via MCP:

```
1. create_niagara_system("NS_FireEmbers")
2. add_emitter_to_system(system, emitter, create_if_missing=True)
3. set_emitter_property(SimTarget="GPU")
4. set_emitter_property(CalculateBoundsMode="Fixed")
5. set_emitter_property(FixedBounds="-200,-200,0,200,200,500")
6. add_module_to_emitter(SpawnRate, stage="EmitterUpdate")
7. add_module_to_emitter(InitializeParticle, stage="Spawn")
8. set_module_random_input(Lifetime, "1.5", "4.0")
9. set_module_random_input(SpriteSize, "3", "12")
10. set_module_random_input(Velocity, "-30,-30,50", "30,30,150")
11. add_module_to_emitter(GravityForce, stage="Update") [negative gravity]
12. add_module_to_emitter(CurlNoiseForce, stage="Update")
13. add_module_to_emitter(ScaleColor, stage="Update")
14. set_module_color_curve_input(color gradient with alpha fade)
15. set_module_linked_input("Curve Index", "Particles.NormalizedAge")
16. compile_niagara_system()
```

### What PARTIALLY WORKS:

```
17. create_material("M_FireEmber", blend_mode="Additive",
                    shading_model="Unlit", used_with_niagara_sprites=True)
18. add_material_expression("ParticleColor")
19. connect_expression_to_material_output(EmissiveColor)
20. set_renderer_property(Material=...)
```

**Issue**: Material may not render until editor restart due to missing shader recompilation.

### Current Workaround:

Use default Niagara sprite material: `/Niagara/DefaultAssets/DefaultSpriteMaterial`

---

## Conclusion

**For fire embers VFX, MCP tools are 95% capable.** The Niagara side is fully functional. The only critical gap is material shader recompilation - custom materials with Niagara usage flags may not render immediately.

**Recommended action**: Add `ForceRecompileForRendering()` to `MaterialService::CreateMaterial()` to ensure usage flags trigger shader compilation.
