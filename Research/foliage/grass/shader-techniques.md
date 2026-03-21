# Girardot Realistic Grass Shader — UE5 Technical Breakdown

**Source:** "How I built a realistic grass shader in UE5! Shader driven bend, clump, tilt, wind and more!"
**URL:** https://www.youtube.com/watch?v=yYE1Hfg067I
**Author:** Girardot (YouTube)
**Context:** Part 2 of a 3-part series. Part 1 covers Niagara 2D wind system + tiling wind texture generation. Part 3 covers Blender foliage asset workflow.

---

## Overview

The entire grass system is driven by **World Position Offset (WPO)**. The grass mesh is exported with all cards **straight, undeformed, and facing positive X-axis**. All deformation (bend, rotation, tilt, clumping, wind, camera-facing billboard) is applied entirely in the vertex shader via WPO tricks.

Inspired by Ghost of Tsushima GDC talk. Goal: recreate most features from that tech talk using only WPO, without geometry shaders.

---

## Grass Mesh Setup (Blender)

- Flat cards, all **straight and undeformed**
- All cards **oriented toward positive X-axis** (critical for the bend HLSL to work)
- **UV0**: Standard texture mapping for grass textures
- **UV1 (baked via Data Baker addon)**:
  - Vertical axis: **Normalized height** (0 at base, 1 at tip)
  - Horizontal axis: **Random value per card**
- **Vertex Normals**: Repurposed to store a **random 2D direction per card** (baked via Data Baker Blender addon)
  - Since the shader reconstructs world-space normals from scratch, original vertex normals are irrelevant
  - Free storage channel for per-card random data
- Cards pivots baked into UVs (common technique, separate video on this)

### LOD Consistency for Vertex Normals

- Used a **seeded random stream** so all LODs generate the same random direction for a given card
- Problem: lower LODs delete cards, so component arrays may not be consistent between LODs
- Problem: a single grass card may be made of multiple mesh pieces needing the same normal
- Solution: did the baking in **Blender** (not via UE Action Script) to ensure all LODs share the same data

### Action Script Approach (attempted, abandoned)

- Split mesh by components, assign random 2D normal per component, re-merge
- Failed because: LODs have different component counts, and multi-mesh cards get different normals
- `SplitMeshByComponents` + per-component random normal + `AppendMesh` loop

---

## World Position Offset — Core Technique

### Step 1: Reconstruct World Position from Scratch

```
Offset = (0,0,0) - WorldPosition    // Collapse all vertices to origin
       + VertexLocalPosition         // Add local position back
       - BakedPivots_LocalSpace      // Center cards on their pivots
```

This gives complete control over scale and rotation around pivot points.

### Step 2: Instance Scale

- Assumes **uniform scale** — uses the **Z component** of instance scale
- Transform a vector from local to world space, get its length = instance scale along Z
- Extra parameter for additional scale control

### Step 3: Per-Instance Fade (Distance Culling)

- Uses **PerInstanceFadeAmount** node (0-to-1 value based on instance start/end cull distance settings)
- Smooths out otherwise harsh instance distance culling

### Step 4: Card Spread Control

- Instead of offsetting cards using baked pivots in world space, can use **instance position in world space**
- **Lerp** between pivot-based position and instance position controls how much cards spread apart
- Lerp = 1: cards at their original positions; Lerp = 0: all cards collapsed to instance origin
- Note: reducing spread may mess with **occlusion culling** — increase mesh bounds accordingly

---

## Rotation System — 3x3 Matrix Approach

Instead of stacking trigonometric rotations (expensive, complicated), uses a **3x3 matrix constructed from three orthogonal axes**.

### Random Per-Card Rotation (around Z-axis)

1. Z-axis = world Z (initially)
2. X-axis = random 2D direction stored in **vertex normals**
3. Y-axis = cross product of Z and X
4. Feed into 3x3 matrix multiplication

- Enable **Two Sided** on the material (required since cards may flip)

### Landscape-Aligned Rotation

When instances are oriented to match landscape normals:

1. Get instance Z vector transformed from local to world space = **landscape normal**
2. **Normalize** it (vector transform applies both rotation and scale)
3. Use as new Z-axis
4. Problem: random X-axis (2D) + sloped Z-axis = non-orthogonal axes
5. Solution: **double cross product**:
   - Axis1 = cross(Z, RandomDir)
   - Axis2 = cross(Axis1, Z)
   - Axis3 = Z
6. **Normalize cross products** when input vectors are non-orthogonal
7. Lerp between straight-up and landscape-aligned for artistic control
8. Can **exaggerate** the landscape effect beyond 1.0

---

## Clumping System

### Z-Vector Tilting

The Z-axis of the rotation matrix is **not fixed** — it can be shifted in X and Y to **tilt cards in arbitrary directions**. As long as Z doesn't equal zero, the 3x3 matrix remains well-defined.

### Clump Map Generation Tool

Built as an **Editor Utility Widget** in UE:

- UI uses **SinglePropertyView** widgets pointing to blueprint variables
- Button triggers a draw function
- Creates a **Render Target**, draws brush material hundreds of times with randomized parameters
- Each brush stroke:
  1. Compute 2D position delta between brush position (UV space) and UVs
  2. Safely normalize delta to get **radial direction**
  3. Distort UVs with a **noise node** for organic flow
  4. Mask with a **bilinear sphere mask**
  5. Bilinear mask fades in the effect AND fades it out near center (prevents cards crossing at clump center)
- Brush material is **duplicated and offset 3x** to ensure the clump map **tiles**
- Result: organic, noisy radial direction texture
- Saved as a static texture in current asset browser directory via `CreateStaticTexture`

### Clump Map Sampling

- Sample using **baked pivots in world space** as texture coordinates (not absolute world position)
  - Prevents mesh deformation from two vertices sampling slightly different directions
  - Gets a single clump sample per card
- Add X/Y offset from clump map to the Z-vector before normalization
- **Randomize texture coordinates** by offsetting with the per-card random 2D direction (from vertex normals)
  - Zero offset = spatially coherent clumping
  - More offset = more chaotic/random
  - Cheap to compute
  - Note: randomizing texture coordinates is generally **not GPU cache friendly**

### Clump Position Offset

- The card spread lerp (instance position vs pivot position) can also be driven by a noisy texture
- Ideally would use a **2D distance field** for arbitrary clump center positions (noted as "something to try")

---

## Wind System

### Wind Texture Sampling

- Same Z-vector tilting principle as clumping
- Wind texture provides **velocity** (not just scalar magnitude) — it's a 2D flow direction
- Can randomize wind sampling coordinates same as clump map

### Directional Wind Response (Sail Effect)

- Normalize wind velocity to get **wind direction**
- **Dot product** with card's forward facing direction
- Modulate wind strength by this dot product
- Rationale: flat grass blade acts like a sail, reacts less when perpendicular to wind
- Likely negligible physically, but adds extra randomness cheaply

### Height-Based Wind Fade

- Use the **normalized height gradient** (baked in UV) to fade in wind offset along grass length
- Creates curvature effect
- Individual controls for curvature amount per feature: clump offset, wind offset, bend

### Wind Interaction with Bend

- Wind magnitude **decreases bend amount** — grass straightens in stronger wind
- Wind also **reorients** the tilt direction (via Z-vector normalization) — grass faces wind direction as wind strengthens

---

## Bend System (Resting Pose)

### Mathematical Approach (HLSL Custom Node)

- Bends mesh around the **X-axis** (reason all cards must face +X initially)
- Uses **sine and cosine** — did not measurably impact performance
- Same HLSL code as the "bend modifier" shared on Twitter
- Bend is applied **before** the rotation matrix

### Performance Note on Trig Functions

> "Shaders and GPUs are complicated, nothing scales linearly. At times you may use a sin and not even pay its cost. Occupancy may be low, the GPU may be waiting for a texture fetch and has nothing else to do but math for free while waiting, as long as it's not dependent on what the GPU is waiting for."

### Bend Amount Control

- Driven by a **noise texture** for per-card variation
- Wind X/Y magnitude **decreases** bend amount (grass straightens in wind)

### HLSL Bend Node Outputs

1. **Vertex offset** (the actual bend displacement)
2. **Bend direction vector** — used as starting point for world-space normal reconstruction

---

## Camera-Facing Billboard System

### Goal

Maximize pixel coverage (minimize transparent card edges) while preserving bend, rotation, and random orientation.

### Basic Billboard

1. Transform Z-vector from **view space to world space**
2. Derive XYZ coordinate system from it
3. Problem: full 3D billboard is too apparent

### 2D Billboard (Horizontal Only)

1. Remove Z component from the view-space direction
2. Normalize the resulting 2D vector
3. **Edge case**: when looking straight down, 2D vector is null — use Y-vector from view space instead (check `squareLength ~= 0`)
4. Apply bend effect on top of 2D billboard — mesh "rolls on itself" to face camera

### View Space vs Camera Space

- **View Space**: direction is from whatever renders the geometry (light POV during shadow pass, camera during main pass)
  - Shadows are different but stable
- **Camera Space**: always from camera POV, even during shadow pass
  - Consistent but unstable shadows — shadows disappear when cards are perpendicular to light, cards flicker
- Can use **Shadow Pass Switch** node for separate shadow pass logic

### The Rotation Problem + Solution

Cards must be billboard-rotated BEFORE bend (bend requires X-axis alignment), but rotation is applied AFTER bend. This breaks the billboard effect.

**Solution: Un-rotate the view vector**

- Instead of using the actual 2D view vector, rotate it by the **inverse of the per-card rotation**
- In 2D, the inverse rotation simplifies to using the card's forward vector
- After per-card rotation is applied, the view vector resolves to the actual view direction

### Billboard Impact

- "Without" vs "with" camera-facing: **significant visible difference in perceived grass density**
- Same vertex count, just better pixel coverage
- Not expensive to compute
- Effect is nearly invisible with many grass instances, only apparent on isolated single meshes

---

## Normal Reconstruction

### Why Rebuild from Scratch

1. Vertex normals are repurposed (storing random 2D directions)
2. WPO deformations (bend, tilt, clump, wind) make fixing original normals harder than rebuilding
3. Need world-space normals that account for all deformations

### Two Normal Modes

1. **Upward Normal**: Start from Z-axis, shift with bend — common for stylized rendering, unified lighting
2. **Forward Normal**: Start from card's forward direction, shift with bend — correct for realistic rendering

- The bend HLSL node outputs a **bend direction vector** used as the normal starting point
- Take **absolute value** of Z component (bug fix, works but reason unclear)
- **Lerp** between upward and forward normals for artistic control

### Rotation of Normals

- Apply the **same 3x3 transform matrix** used for card rotation to the reconstructed normal

### Tilt Direction Problem

- Forward normals aligned with the tilt axis result in **no visible change** from the rotation matrix
- **Workaround (cheat)**: Lerp the world normal toward the tilt direction proportional to tilt magnitude
- Ensures wind/clump features affect normals regardless of card orientation

### Normal Map Application (Tangent Space to World Space)

- Standard `TransformVector` (tangent to world) can break because tangents are computed from UVs + vertex normals, and normals are overridden
- **Manual remapping**:
  1. Tangent space Z-axis = reconstructed world normal
  2. Y-axis = cross product of normal and card forward
  3. X-axis = cross product of Y and normal
  4. Transform the tangent-space normal map sample using these three axes
- "Battle tested, haven't noticed issues"

### Artistic Recommendation

- Forward normal is better in broad daylight
- Upward normal is better at sunrise/sunset
- Compromise: mainly forward normal, **slightly shifted toward Z** for best of both worlds

### Vertex Interpolator

- The local position node used in normal calculation is **vertex-only** (not available in pixel shader)
- Must use a **Vertex Interpolator** node to pass data to pixel shader
- This is beneficial anyway — avoids per-pixel computation of expensive WPO math

---

## Shading Model

- **Two-Sided Foliage** shading model
- Provides subsurface scattering for light bleeding through cards
- Subsurface color = reusing base color (could use a separate texture)
- Material Opacity controls transmission amount (0 = fully translucent)
- Believed to automatically set variables for **Lumen** to work with two-sided cards (unconfirmed)

---

## LOD Strategy

### Dithered LOD Transitions

- Uses **dithered LODs** for smoother transitions between LOD levels
- Critical because triangle count must be **drastically reduced at distance** due to vertex shader cost
- Dithered LODs come at a **small extra cost**

### Per-LOD Shader Differences via Material Instances

- Different material instances per LOD level
- Shadow Pass Switch can be **enabled only on distant LODs** (e.g., make distant grass fully opaque in shadow pass for denser shadows)

---

## Opacity & Mip Map Tricks

### Dithered Masked Opacity

- Uses **DitheredMaskedOpacity** (uncommon for grass) — achieves smoother edges and fluffy look
- Problem: mip maps average opacity texture to gray values at distance, making cards semi-transparent

### Mip Map Solutions

1. **Texture settings**: Tweak mip generation options in UE; there's a `PreserveBorder` option that maintains certain values across mip chain — "quite a difference in brightness"
2. **Manual mip maps**:
   - In Photoshop: Layer 0 = full res, duplicate + halve size as "MIP map 1", repeat down to 1x1
   - Export as **DDS** with specific options
   - Import in UE with **"Leave Existing Mips"** setting
   - Can embed fade in/out effects, reduce specular at distance, increase roughness at distance
   - Epic used similar technique for Fortnite wind system

### Shadow Pass Opacity Tricks

Using **Shadow Pass Switch** node:

- **Fully opaque in shadow pass**: Increases shadow coverage, grass looks thicker/denser. Shadows cast from geometry are more apparent though.
- **Apply DitheredMaskedOpacity + Jitter AA** in shadow pass: Fakes light bleeding through grass, weaker shadows. "Super simple yet very effective." (Discovered via Twitter community)
- **Double card size during shadow pass**: Possible via WPO in shadow pass switch — increases shadow coverage.
- **Distance-based**: Enable opaque shadows only on distant LOD material instances — creates denser grass appearance at distance.

---

## Pixel Depth Offset

- Renders a **random value per grass blade** into a texture
- Uses that texture to push pixels in **camera space** via Pixel Depth Offset
- Creates illusion of grass blades at different depths — gives depth to otherwise flat cards
- **Mip map consideration**: At distance, depth values average out. Manually fade out depth in mips to prevent self-shadowing artifacts with dynamic lighting.
- Uncertain how it behaves with **Virtual Shadow Maps** — noted as "something to check"

---

## Landscape Layer Capture (No RVT Alternative)

### Why No Runtime Virtual Textures

- Decided against RVTs for this shader (used them in stylized grass shader previously)
- RVTs add complexity, migration issues, and don't contribute enough visually for realistic grass
- Makes shader harder to migrate to other projects

### Landscape Layer Bake Tool

- Built a tool (Editor Utility Widget) to capture painted landscape layer as a texture
- Select landscape + material layer, click button, generates a texture
- Projected in world space in the grass shader using offset + size parameters
- Used to:
  - **Drive card scale** — smoothly shrink grass bleeding onto dirt layers
  - Create natural-looking layer transitions
- Limitation: **not scalable** for large open worlds, but viable for most use cases

---

## Performance Considerations

### Vertex Shader Cost

- The WPO logic is **extensive** — this grass has a significant vertex shader cost
- "Really important to keep vertex count under control"
- Be **aggressive with LODs**
- Prevent as much **quad overdraw** as possible

### Pixel Shader Cost

- "The pixel shader is actually quite simple and cheap"
- Base color: simple texture
- Roughness + specular: two textures
- Opacity mask, normal, pixel depth offset already computed

### Not Using Nanite or Virtual Shadow Maps

- Both have downsides with WPO (though improving recently)
- Chose traditional shadow maps + non-Nanite meshes as "rock solid"
- Performance "remains quite decent" with large instance counts and long view distance

### Key Performance Takeaways

- Trig functions (sin/cos) in vertex shader may be effectively **free** due to GPU occupancy patterns
- Randomizing texture coordinates is **not GPU cache friendly** (but done anyway for visual benefit)
- Dithered LODs have a small extra cost
- Billboard logic is cheap relative to its density improvement
- Per-card clump sampling (using pivots as UV) avoids per-vertex texture coordinate divergence

---

## Material Pin Assignments Summary

| Material Pin | Source |
|---|---|
| Base Color | Grass texture |
| Roughness | Texture |
| Specular | Texture |
| Normal | Reconstructed world-space normal + tangent-space normal map remapped |
| Opacity Mask | Dithered masked opacity |
| World Position Offset | Full WPO chain (scale, bend, rotation, billboard, clump, wind, tilt) |
| Pixel Depth Offset | Random per-blade depth texture |
| Subsurface Color | Reused base color |

---

## Tools Created

1. **Clump Map Generator** — Editor Utility Widget, generates tiling radial direction textures
2. **Landscape Layer Capture** — Editor Utility Widget, bakes landscape painted layers to textures
3. **Data Baker Blender Addon** — Bakes pivots to UVs, random 2D/3D directions to vertex normals/colors

---

## Key Nodes Referenced

- PerInstanceFadeAmount
- VectorTransform (Local to World, View to World, Camera to World)
- Shadow Pass Switch
- Vertex Interpolator
- DitheredMaskedOpacity (DitherTemporalAA)
- Pixel Depth Offset
- Custom HLSL (bend modifier)
- 3x3 Matrix (constructed from 3 axis vectors)
- Cross product, Dot product, Normalize
- Lerp (extensively used for artistic controls)
- SphereMask (bilinear variant for clump brush)
- Noise node (for clump map brush distortion)
