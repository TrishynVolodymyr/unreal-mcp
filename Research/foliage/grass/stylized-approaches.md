# Stylized Grass Techniques in UE - Girardot Breakdown

**Source:** https://www.youtube.com/watch?v=pVKDfZMffpc
**Author:** Girardot (YouTube)
**Engine:** Unreal Engine 5 (non-Nanite workflow)
**Performance baseline:** ~110 FPS on 10-year-old hardware with full grass system

---

## 1. Modeling Workflow (Blender)

### Grass Blade Creation
- Model simple grass blade meshes in Blender, iterate on pointiness, curvature, thickness, density
- Use top-down orthographic camera as a render canvas to bake textures from 3D meshes
- Carefully unwrap meshes so UV maps can derive gradients via simple materials

### Grass Card Construction
- Create flat cards from blade meshes, UV-unwrap using "project from view" with a quad matching the render region
- Duplicate cards, reset pivot point orientation and scale, tweak so they are not perfectly flat
- Cards should be viewable from most angles (at least partially) to increase perceived density
- Consider two approaches: more compact mesh with fewer cards + more instances, OR bigger mesh with fewer instances -- find the sweet spot

### LOD Creation (Manual, Not Auto-Generated)
- **Always create LODs manually for foliage** -- UE's automated LOD tools cause issues with card-based meshes
- Author creates 3 LODs: LOD0 (full detail), LOD1 (simplified), LOD2 (most simple cards)
- Space out blade meshes in Blender to allow simplest cards for higher LODs

### Data Baking into UVs
- **Pivot points (X, Y):** Baked into a single extra UV channel (UV1). Z is irrelevant since all pivots are at ground level. Memory cost is near zero.
- **Normalized root-to-tip gradient:** Baked in Y of an extra UV channel. Used extensively for blending, SSS falloff, wind falloff, scale control.
- **Per-card pseudo-random value (0-1):** Baked in X per card. Used to randomize wind animation offset per card.
- **Critical:** All LODs must share the same baked UV values.
- Normals left unmodified in Blender -- overridden in the shader directly in engine.

### Texture Baking from 3D Meshes
- **Opacity:** From world normals remap
- **Normal map:** From world normals
- **Root-to-tip gradient:** From texture coordinates
- **Edge gradient:** From texture coordinates
- **Per-blade random grayscale:** Baked per blade
- **Height map:** Black-and-white
- **RGB mask textures:** Combined from gradients/masks for quick roughness/specular creation
- **Distance field texture:** Bi-directional gradient (center-to-edges AND root-to-tip). Created by duplicating meshes, flattening, using shear tool for center-line slope. Used for smooth fade-in/out of blades near camera.
- **Color:** Uses a "splat map" (tiling painted texture) projected in world space rather than per-blade color texture. Created in Krita using painterly brushes in wrap mode for easy tiling.

---

## 2. UE Import & Static Mesh Setup

### Import Settings
- Turn off "Generate Collision" and "Light Map"
- Enable "Combined Meshes"
- Turn off auto-material creation (create manually)

### LOD Setup
- Add LOD1 and LOD2 via right-click > "Add LOD" > import
- Create separate material slots per LOD (cheaper materials on distant LODs)
- Assign each LOD its corresponding material slot

### Mesh Settings
- **Shadow casting: OFF.** Shadow maps lack resolution for small detailed assets like grass. Rendering all grass instances per cascaded shadow map is immensely expensive. Visual benefit is debatable.
- **Screen-space contact shadows:** Fixed cost based on viewport resolution + contact shadow length. Good alternative for grass, though a bit noisy.
- **Stylized grass consensus:** No shadow casting looks more subtle and easier on the eye.
- **Collision: OFF** (default collision mode set appropriately)
- LOD distances: eyeball initially, fine-tune after landscape placement using the LOD debug view

---

## 3. Performance: Transparency vs Geometry Tradeoff

This is the central performance tension for grass rendering:

### The Problem
- Grass is seen in massive quantities and from far away -- vertex count skyrockets
- Vertices are NOT free, even with Nanite
- Very small triangles at distance cause issues with quad overdraw
- Grass materials have complex vertex shaders (wind animation) -- vertex processing cost scales with vertex count

### Transparency (Masked Opacity)
- **Pro:** Fewer vertices, simpler mesh
- **Con:** Overdraw is expensive, especially with many overlapping instances
- **Mitigation:** Depth pre-pass can greatly reduce masked opacity cost, but that pass has its own cost

### Geometry (Detoured Mesh, Reduced Transparency)
- **Pro:** Less overdraw, potentially cheaper pixel shading
- **Con:** More vertices, more vertex shader cost (especially with wind WPO)
- Skipping transparency entirely and relying purely on geometry throws hundreds of thousands to millions of vertices at the GPU

### Recommendation
- **Up close:** Afford more geometry to reduce overdraw efficiently
- **At distance:** Lower vertex count drastically, rely on transparency more
- **Profile and benchmark** with your specific in-game view, distance, and target hardware
- There is always a tipping point -- finding the right balance is project-specific
- **Use full roughness hint:** If material is fully rough, enable "Fully Rough" to save a tiny bit of cost
- **Disable decal response** on grass material if not needed -- makes material slightly cheaper

---

## 4. Instantiation Methods Compared

### Foliage Tool
- Paint instances on any mesh with collision (including landscapes)
- Straightforward, good default choice

### Procedural Foliage Tool
- Complex rules for growth, influence of nearby assets, lifelike behaviors
- Probably overkill for most use cases

### Custom Blueprint (ISM Component)
- Vertical line traces to find valid ground, add instances to Instanced Static Mesh component
- Full control but not as straightforward as foliage tool
- Performance concern with thousands of instances -- C++ may be needed

### PCG (Procedural Content Generation, UE 5.2+)
- Node-based graphs for procedural placement
- Very promising new system

### Landscape Grass Type (Used in This Video)
- Create a Landscape Grass Type asset, assign grass mesh, set density
- In landscape material: add Grass Output node, select grass type, control with landscape layer blend
- Value of 1 = grass everywhere, 0 = none
- Can paint dirt layer to remove grass based on threshold
- **Start/End Cull Distance** settings control draw distance (huge performance impact)
- Can use multiple grass varieties in the same grass type for density layering

---

## 5. Material System

### Base Material Setup
- Two-sided, blend mode = Masked
- Tangent-space normals: OFF (expects world-space normals)
- Set normals to Z unit vector (all pointing up) -- flat stylized look
- Normal map: convert tangent-to-world, lerp with Z unit vector to control strength
- Shading model: **Subsurface** (enables SSS color + opacity pins)
- SSS color: reuse base color with color multiplier for lazy but effective approach

### Runtime Virtual Texture (RVT) Blending
- **Purpose:** Make grass seamlessly blend with landscape at the roots
- Create two RVTs: one for color/normals, one for world height
- Create two RVT volumes, set bounds to landscape
- In landscape material: add RVT output node, wire base color, pixel normals (world space), world height
- **Hack:** Store landscape grass layer mask in the specular channel of the RVT (saves enabling the mask channel which costs 25% more RVT memory)

### Root-to-Tip Blending
- Sample RVT color at roots, blend to grass material color at tips
- Use normalized height from UVs with power node to control gradient curvature
- SSS: force opaque at roots (subsurface color = black), SSS only at tips
- This makes grass perfectly blend with ground at base, show own character at tips

### Per-LOD Material Optimization
- Master foliage material with parameter switches to enable/disable features
- Distant LODs disable: ground projection, view occlusion, normal/roughness/specular maps
- **Warning:** Don't introduce too much visual change between LODs or transition is noticeable

---

## 6. World Position Offset Techniques

### Core Concept: World Position Reconstruction
The most powerful technique in the video. Instead of simple WPO offsets:

1. **Nullify vertex positions:** Subtract world position from itself (offset = 0 - WorldPosition), collapsing all vertices to world origin
2. **Rebuild from scratch** using pre-skinned local position, applying transforms one step at a time
3. This gives complete control over how location, rotation, and scale are applied per-card

### Pivot-Based Card Control
- Use baked pivot points (from UV1) to separate card positions from card geometry
- Subtract pivot from pre-skinned local position -> all cards centered at origin
- Apply per-card scale/rotation while centered
- Add pivot back (transformed local-to-world) to position cards correctly

### Key Transform Behaviors
- **Location:** Applied via pivot point offset (local-to-world transform)
- **Rotation on pivots only:** Cards remain straight/vertical even on slopes (grass grows vertical, looks natural)
- **Scale on pivots only:** Cards spread apart but don't change size
- **Object scale retrieval:** Transform unit vector from mesh-local to world, compute length. For uniform scale, only need one axis (saves 2 square roots)
- **Object rotation retrieval:** Transform unit vectors local-to-world, normalize to remove scale. Use as transform matrix axes.

### WPO Caveats
- CPU is unaware of GPU offset -- collision remains at original position
- Mesh may disappear due to occlusion/frustum culling (increase bounds as workaround, but it's a hack)
- Distance fields are pre-computed, WPO causes self-shadowing issues (use self-shadow offset setting)
- For grass, offsets are usually small enough that these issues rarely arise

---

## 7. Scale-Based Effects via WPO

### Texture-Driven Height Variation
- Sample a texture projected in world space to control per-card scale
- **Critical:** Sample texture using pivot world position (not vertex position) to prevent distortion across vertices on the same card

### Per-Instance Fade (Cull Distance Fading)
- Use `PerInstanceFadeAmount` node to smoothly scale cards to zero at cull distance
- Value goes from 0 (start cull distance) to 1 (end cull distance)
- Apply as scale multiplier for smooth fade-out

### Landscape Layer Sampling for Edge Treatment
- Sample the grass layer from RVT specular channel
- Shrink cards that bleed onto dirt layer
- Tightens the grass/dirt boundary

### RVT Override Trick
- Place invisible-in-main-pass planes within RVT volume on top of landscape
- Those planes write to the RVT (e.g., write 0 to specular channel)
- This shrinks grass at arbitrary locations or overrides color
- Essentially "painting" on the RVT to control grass behavior

---

## 8. Camera Tilt (Top-Down Fix)

### Problem
Looking straight down reveals paper-thin grass cards.

### Solution
- Transform Z unit vector from view space to world space (gets camera's "up" direction in world)
- X and Y components increase when looking down
- Multiply this 2D vector by mesh local height to convert offset into a tilt angle
- Apply as WPO to push top vertices forward in camera space

### Caveats
- It's a hack -- effect can be apparent
- May push vertices inside nearby meshes
- Keep subtle or use alternative approach

---

## 9. View Occlusion (Camera Proximity)

### Problem
Grass blades right in front of camera obscure the view.

### Method A: Dithered Opacity
- Use pixel depth to drive opacity mask value
- Smoothly fade grass near camera
- Small cost from dithered opacity, may not look great

### Method B: Distance Field Texture Fade
- Use the bi-directional gradient texture (distance field)
- Smoothly offset opacity mask based on pixel depth
- Each blade fades in/out smoothly
- Can also drive thickness/scale (seasonal effects: leaves shrink in winter)

### Method C: WPO Camera Avoidance (Author's Preferred)
Material function that pushes vertices away from camera:

1. Compute position delta between vertex and camera position
2. Dot product of delta with itself = squared distance (avoids square root)
3. Build 0-to-1 gradient with offset and divisor (squared values since working with squared distance)
4. Saturate, invert (1-x), apply power falloff
5. Multiply by mesh local Z height (converts to tilt-like offset)
6. **Lateral push:** Dot product of delta with camera X-axis direction, multiply by that direction vector
7. **Forward push:** Camera Z vector (forward in UE view space) transformed to world
8. Combine forward + sideways, discard Z, replace with negative Z (push down)
9. Result: grass pushed forward, sideways, and downward along grass height

**Key optimization:** Compute delta using pivot world positions (not per-vertex) to prevent stretching/collapsing. Entire card gets uniform push.

---

## 10. Wind Animation

### Common Approach (Criticized)
Simple sine-based offset node -- motion is too recognizable, used even in AAA games.

### Pivot-Based Rotation
- Apply rotation using baked pivots
- Looks great but expensive due to trigonometry (sin/cos) on many vertices
- Requires pivot baking

### Spherical Reprojection (Author's Preferred)
Approximates rotation without trig, without requiring pivots:

1. Get vertex's vertical distance to object pivot (local Z position)
2. Add 2D wind offset (X, Y)
3. Compute 3D distance from offset vertex to pivot
4. Compare with original distance -- push vertex to maintain original distance
5. This "spherical reprojection" mimics rotation

**Cost:** Only two square roots. Very cheap.

**Caveat:** Deforms mesh slightly (quad shrinks with large offsets). Keep offset controlled -- barely noticeable for grass.

**Bonus:** The reprojection direction can be reused as a corrected world-space normal (not perfect at pivots but essentially free).

### Wind Texture Setup
- Bake two tiling normal maps in Blender using ocean modifier
- Pack into one texture: R+G = low frequency (large smooth gradients), B+A = high frequency (noisy)
- Project in world space, scroll using Time * speed in X and Y
- **Time offset randomization:** Per-instance (PerInstanceRandomValue) + per-card (baked pseudo-random UV value)
- Control chaotic vs unified wind by adjusting random offset amount
- **Swirl effect:** Offset time along grass length (normalized height UV) -- creates delay along blade length
- Boost base color slightly with wind to emphasize motion visually

---

## 11. Depth Offset (Visibility Trick)

For flowers/plants that get lost within dense grass at distance:
- Compute delta between vertex and camera position
- Divide by distance along camera forward axis
- Multiply by small amount to push vertex towards camera
- **Turn off when rendering shadows** to avoid shadow artifacts

---

## 12. Ground Hugging / Ground Projection

### Problem
Large grass mesh instances on varying slopes cause cards to levitate or clip through ground.

### Solution
1. Sample landscape height from RVT at vertex world position (X, Y)
2. Subtract vertex Z world position = flattening offset
3. Add back the mesh's local Z height (account for object scale)
4. Result: vertical offset that conforms cards to ground surface

### Caveats
- Accuracy depends on RVT resolution (bigger landscape = more approximate)
- Increasing RVT resolution costs more memory
- Landscape LODs can behave weirdly with RVTs -- may need a bias value

---

## 13. Grass Clumping Experiment

### Technique (Author's Experimental)
1. Create globe-looking tiling texture in Krita
2. Use as displacement map in Blender to bake a tiling normal map (X, Y only)
3. Project in engine in world space, applied just before spherical reprojection
4. Offsets vertices to create clump shapes with a fixed direction baked in
5. Spherical reprojection outputs varying normal patterns per clump

### Assessment
- Author admits it's not great / not fully natural
- Kept effect subtle
- Potential improvement: use a density map to arbitrarily move card pivot positions across landscape, with ground projection maintaining contact regardless of offset

---

## 14. Custom Billboard System (Distance LOD)

### Problem
Increasing cull distance with normal grass destroys FPS. The bottleneck is 100% vertex count (not transparency).

### Billboard Mesh Creation (Blender)
- Model 3 large grass clumps
- Detour first clump with a single quad (the billboard)
- UV maps only the first clump (the other two are UV-shifted variants for variation)

### Billboard Material (Transform Matrix Method)

**Basic billboard (faces camera):**
1. Transform mesh vertices from view space to world space
2. If mesh doesn't face -Z by default, apply a transform matrix to reorient (flip Y and Z axes)
3. Result: mesh always faces camera

**Problem:** Clips through landscape on slopes.

**Improved billboard (maintains up axis):**
- Z-axis of matrix = camera forward (Z in view space = forward in UE)
- Y-axis = object's local Z (up) -- NOT camera's Y
- X-axis = cross product of the above two
- Normalize to remove scale from local-to-world transform
- Skip normalizing camera forward (card has no thickness on that axis, saves a square root)

**Final version (hybrid):**
- Bottom vertices: maintain ground contact (no full billboard rotation)
- Top vertices only: use the basic billboard method (face camera)
- Creates some stretching but invisible at distance
- Looks similar to normal grass for smoother transition

### Billboard Variation
- Use per-instance random value to randomly shift UVs by 1/3 or 2/3
- Selects one of 3 baked grass clump variants (like a flipbook)
- Adds significant variation to distant grass

---

## 15. Three-Tier Density System (Final Architecture)

The complete grass system uses **3 separate Landscape Grass Type varieties:**

| Tier | Purpose | Density | Cull Distance | Notes |
|------|---------|---------|---------------|-------|
| **Close** | High density up close | Very high | Short | Fades out at small distance via PerInstanceFadeAmount scale |
| **Medium** | Standard grass | Medium | Medium | Normal grass mesh with 3 LODs, fades via scale |
| **Far (Billboard)** | Draw distance fill | Low-medium | Very large (up to 1km) | Single-quad billboard, extremely cheap |

### Transition System
- Each tier scales cards from 1 to 0 using PerInstanceFadeAmount
- Billboard tier additionally scales cards to 0 when **close** to camera (reverse fade)
- Overlapping fade zones create smooth transitions between tiers
- Medium density slightly reduced to compensate for close-range tier

### Performance Results
- ~110 FPS on 10-year-old hardware
- Grass visible up to ~1 kilometer
- Three separate grass type entries allow independent control of density, jitter, cull distance per tier

---

## 16. Summary of Optimization Techniques

| Technique | Impact | Cost |
|-----------|--------|------|
| Manual LODs (3 levels) | Moderate | Modeling time |
| Shadow casting OFF | Large | None (visual tradeoff) |
| Screen-space contact shadows instead | Good look | Fixed cost based on resolution |
| Per-LOD material switches (disable features at distance) | Moderate | Setup time |
| Fully Rough hint | Tiny | None |
| Disable decal response | Tiny | None |
| Billboard system for far distance | Massive | Material complexity |
| Three-tier density with independent cull distances | Massive | Setup complexity |
| PerInstanceFadeAmount smooth scale-out | Visual quality | Near zero |
| Sample textures at pivot world pos (not vertex) | Prevents artifacts | None |
| Spherical reprojection wind (vs trig rotation) | Micro-optimization | None |
| Skip square roots where possible (squared distance, skip one axis normalization) | Micro | None |
| RVT specular channel hack (avoid mask channel) | 25% RVT memory saved | Potential confusion |

---

## 17. Key Takeaways

1. **Profile first, optimize second.** There is no universally correct balance between transparency and geometry. It depends on camera distance, hardware, and scene complexity.
2. **Vertices are never free.** Even with Nanite. Wind animation vertex shader cost scales linearly with vertex count.
3. **World Position Reconstruction** (nullify + rebuild) is the single most powerful material technique for foliage. It enables per-card scale, rotation, ground projection, clumping, camera avoidance, and billboarding.
4. **Bake data into UVs** (pivots, gradients, random values). Near-zero memory cost, massive flexibility in-engine.
5. **Three-tier grass density** with independent cull distances and smooth scale transitions is the practical architecture that achieves both visual quality and performance.
6. **Billboard grass** at distance is extremely cheap and can extend draw distance to 1km+ without significant FPS cost. The vertex count bottleneck at distance is real and this solves it.
7. **RVT blending** at roots is essential for stylized grass to look integrated with the landscape rather than planted on top.
8. **Manual LODs** for card-based foliage -- never rely on auto-generation.
