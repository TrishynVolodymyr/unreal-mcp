# Grass Optimization Techniques for UE5

Compiled from Epic's Fortnite tech blog, community forums, developer guides.

## 1. MATERIAL TRICKS

### Two-Sided Foliage Shading Model
- Set Shading Model to "Two Sided Foliage", Blend Mode to "Masked", check "Two Sided"
- Gives cheap fake subsurface/translucency on backfaces without actual subsurface scattering cost
- ~5-10% gain compared to Default Lit + actual subsurface

### Eliminate Masking Where Possible (Nanite)
- If using Nanite foliage, add more triangles to match the leaf silhouette exactly and use **Opaque** instead of Masked
- Masked materials force Nanite off its fast software rasterization path
- Epic found that adding geo to remove masking was a net win

### Dithered Opacity for Distance Fade (NOT for LOD transitions on grass)
- Use `PerInstanceFadeAmount` node connected to Opacity Mask. Set Start/End Cull Distances.
- Smooth fade-out instead of pop. Keep the fade band SHORT (500-1000 units)
- NO FPS gain — visual quality trick only, slightly more expensive than hard pop
- **Dithered LOD transitions on grass specifically cause frame rate drops** (documented UE bug). Use ONLY for cull distance fade, NOT for LOD transitions.

### Keep Materials Dead Simple
- Minimum texture samples, no complex math. Avoid translucency entirely.
- Every shader instruction multiplied by instance count

## 2. SHADOW TRICKS

### Disable Shadow Casting + Use Contact Shadows (FORTNITE APPROACH)
- Set `Cast Shadow = false`. On directional light, enable Contact Shadows with intensity ~0.3-0.5
- **This is literally what Fortnite does.** Epic evaluated rendering grass into VSM and decided contact shadows look better AND perform better. Grass shadows in VSM are "expensive and too harsh."
- Huge FPS gain — eliminates all shadow depth rendering for grass
- No long cast shadows from grass — acceptable since grass is tiny

### ShadowCacheInvalidationBehavior = Static (for larger foliage)
- On foliage that DOES cast shadows (bushes, trees), set `ShadowCacheInvalidationBehavior` to `Static`
- Prevents WPO wind animation from invalidating VSM shadow cache every frame
- Self-shadow won't update with wind animation

### Disable Lumen Reflections Tracing for Foliage
- `r.Lumen.Reflections.MaxRoughnessToTraceForFoliage 0`
- Prevents Lumen from ray-tracing reflections against foliage geometry
- Near zero visual impact

### Shadow Proxy Meshes (for trees, not grass)
- Two mesh components: visible mesh (Cast Shadow = false) + hidden low-poly mesh (Cast Hidden Shadow = true)
- Shadow depth pass renders the simple proxy instead of full-detail tree
- Only relevant for non-Nanite trees close to camera

## 3. CULLING TRICKS

### Aggressive Cull Distances
- Grass: **4000-5000** (40-50m)
- Small flowers/rocks: **6000-8000** (60-80m)
- Bushes: **8000-12000** (80-120m)
- Medium trees: **10000-15000** (100-150m)
- Hero trees: **20000+** (200m+)
- Often the single biggest optimization. Can double FPS.

### Smooth Cull Fade
- Start Cull Distance = End - 1000 (e.g., Start=4000, End=5000)
- Use `PerInstanceFadeAmount` in material connected to Opacity Mask

### foliage.DensityScale CVar (Scalability)
- `foliage.DensityScale=0.8` (high quality) down to `0.4` (low quality)
- Values up to 0.5 have minimal visual impact with good FPS gains
- Below 0.4, ground becomes visibly bare

### foliage.LODDistanceScale CVar
- Controls LOD transition distances for foliage. Lower = more aggressive LOD switching.

## 4. WPO (WORLD POSITION OFFSET) TRICKS

### WPO Disable Distance — THE #1 OVERLOOKED SETTING
- Set `World Position Offset Disable Distance` on every foliage mesh/actor that uses WPO wind
- Value in cm (e.g., 5000 = 50m)
- **WPO invalidates VSM shadow cache EVERY FRAME** for all pages containing WPO geometry
- Disabling WPO at distance means shadow pages get cached and reused
- "HUGE performance difference" — direct quote from multiple Epic-affiliated sources
- Set it at or slightly beyond your grass cull distance so it doesn't matter

### Cheap WPO Functions
- Pre-compute wind as much as possible. Use simple sine waves, not complex noise.
- Fortnite uses "largely pre-computed" WPO
- Sine-based pseudo-noise vs actual noise = significant savings

## 5. NANITE vs NON-NANITE FOLIAGE

### When to Use Nanite for Grass
- Nanite is a win for HIGH-POLY foliage
- For classic low-poly masked grass cards, Nanite can be MORE expensive than traditional rendering
- Epic explicitly says "classic lowpoly trees with masked branches material can potentially be more expensive with Nanite than without it"
- **Test both**: we confirmed Nanite OFF = +12 FPS on our grass

### Disable Coarse Page Rendering for Non-Nanite Foliage
- If foliage is NOT Nanite, disable rendering into Coarse Pages
- Huge performance boost for mixed Nanite/non-Nanite scenes

## 6. PCG-SPECIFIC OPTIMIZATIONS

### GPU PCG + FastGeometry (UE 5.7)
- Enable "PCG FastGeo Interop" plugin
- CVar `pcg.RuntimeGeneration.ISM.ComponentlessPrimitives 1`
- Up to 4x FPS improvement over default landscape grass (80.lv benchmark: 15→60 FPS)

### Hierarchical Instance Culling (UE 5.6+)
- Automatic in 5.6+ — culling works on chunks of 64 instances rather than cells
- Significantly improves instance culling for GPU-generated PCG instances

### Avoid Stacking/Overlapping
- Use exclusion zones and density controls to prevent overlapping
- Overlapping grass = massive overdraw with no visual benefit

## 7. DRAW CALL OPTIMIZATION

### HISM is Automatic for Foliage Tool
- UE's Foliage tool automatically uses HISM which batches draw calls and enables cluster-based culling
- Each HISM component creates a bounding volume hierarchy allowing the renderer to skip entire clusters outside the frustum

### Minimize Unique Mesh/Material Combinations
- Use 2-3 grass mesh variants max, all sharing ONE material
- Each unique mesh+material combo = separate draw call batch

### Disable Collision on Grass
- Set Collision to `NoCollision`
- Collision on 100k grass instances is pure CPU waste

## 8. KEY CVARS CHEAT SHEET

| CVar | Value | Effect |
|---|---|---|
| `r.Lumen.Reflections.MaxRoughnessToTraceForFoliage` | `0` | Disable Lumen reflection tracing on foliage |
| `r.ContactShadows` | `1` (with intensity ~0.3) | Use instead of real grass shadows |
| `foliage.DensityScale` | `0.4-0.8` | Global foliage density multiplier |
| `foliage.LODDistanceScale` | `0.5-1.0` | Foliage LOD distance multiplier |
| `r.Shadow.MaxResolution` | `1024` | Cap shadow map resolution |
| `pcg.RuntimeGeneration.ISM.ComponentlessPrimitives` | `1` | Enable FastGeo for PCG (5.7) |

## 9. RECOMMENDED SETTINGS FOR GRASS

- **Material**: Two-Sided Foliage shading model, Masked blend, dead simple shader
- **Cast Shadow**: OFF (use contact shadows on directional light instead)
- **End Cull Distance**: 4000-5000 (40-50m)
- **Start Cull Distance**: 3000-4000 (fade band)
- **WPO Disable Distance**: 5000 (at or beyond cull distance)
- **Collision**: NoCollision
- **LODs**: 3 LODs (6-7 planes -> 2-3 planes -> 1 plane)
- **Dithered LOD Transition**: OFF (causes FPS drops on grass specifically)
- **foliage.DensityScale**: 0.6-0.8 for quality settings

## Sources
- https://medium.com/@shinsoj/notes-on-foliage-in-unreal-5-3522b6eb159f
- https://www.unrealengine.com/en-US/tech-blog/virtual-shadow-maps-in-fortnite-battle-royale-chapter-4
- https://outscal.com/blog/landscape-and-foliage-optimization-unreal-engine-5
- https://80.lv/articles/replacing-unreal-engine-s-grass-with-gpu-pcg-shows-4-times-better-fps
- https://wh0.is/posts/a-look-under-the-hood-at-unreal-engine-landscape-grass-en
- https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-foliage
- https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine
- https://tomlooman.com/unreal-engine-5-7-performance-highlights/
- https://tomlooman.com/unreal-engine-5-6-performance-highlights/
- https://forums.unrealengine.com/t/grass-optimisation-in-unreal-engine-5/1152397
- https://forums.unrealengine.com/t/tall-grass-optimization/156508
- https://medium.com/@GroundZer0/the-hidden-cost-of-unreal-engine-defaults-part-2-603ca04d014b
- https://forums.unrealengine.com/t/dithered-lod-transition-on-grass-drops-frame-rate/404938
- https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pcg-with-gpu-processing-in-unreal-engine
- https://dev.epicgames.com/community/learning/tutorials/oLqa/unreal-engine-smooth-apparition-of-instances-with-dithering
- https://dev.epicgames.com/community/learning/knowledge-base/KP2D/unreal-engine-a-tech-artists-guide-to-pcg
- https://github.com/SabreDartStudios/FoliageShadowImposters
- https://www.artstation.com/artwork/PeR3Dy
- https://medium.com/@thirdspaceinteractive/unreal-engine-5-7-nanite-foliage-a-game-changer-for-real-time-vegetation-c8e9692df3b5
